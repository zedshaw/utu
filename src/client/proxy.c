#include "proxy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int Proxy_listener_send(Proxy *conn, Node *header, Node *body);

void Proxy_destroy(Proxy *conn)
{
  assert_mem(conn);

  close(conn->client.write_fd);
  close(conn->client.read_fd);
  bdestroy(conn->client.name);
  bdestroy(conn->client.key);

  close(conn->host.fd);
  bdestroy(conn->host.name);
  bdestroy(conn->host.port);
  bdestroy(conn->host.key);
  bdestroy(conn->host.host);

  // sometimes the state isn't correct, so let the system clean for now
  // if(conn->state) CryptState_destroy(conn->state);
}

void Proxy_dump_invalid_state(Proxy *conn)
{
  bstring given_fp = CryptState_fingerprint_bstr(conn->host_given_key);
  bstring expect_fp = CryptState_fingerprint_bstr(conn->host.key);

  Proxy_error(conn, UTU_ERR_PEER_VERIFY, bformat("Invalid cryptographic state attempting to connect to Hub at:\n"
      "\t%s:%s with expected name %s (given %s)\n"
      "With expected key vs given key (fingerprints):\n"
      "\tEXPECT: %s\n"
      "\tGIVEN : %s\n"
      "You most likely shouldn't talk to this Hub unless you gave bad keys.\n",
      bdata(conn->host.host),
      bdata(conn->host.port),
      bdata(conn->host.name),
      bdata(conn->state->them.name),
      bdata(expect_fp),
      bdata(given_fp)));
}


int Proxy_confirm_key(CryptState *state)
{
  assert_mem(state);
  assert_mem(state->data);

  Proxy *conn = (Proxy *)state->data;
  conn->host_given_key = CryptState_export_key(state, CRYPT_THEIR_KEY, PK_PUBLIC);

  if(conn->host.key && !biseq(conn->host_given_key, conn->host.key)) {
    fail("Expected Hub public key does not match given Hub public key from the host.");
  }

  log(INFO, "Hub identity confirmed and keys exchanged.  Keep them safe.");
  return 1;

  on_fail(Proxy_dump_invalid_state(conn); return 0);
}



int Proxy_final_verify(Proxy *conn)
{
  assert_mem(conn);
  assert_mem(conn->state);

  log(INFO, "Verifying final established crypto between peers.");

  check_err(conn->host.name, INVALID_HUB, "No name given for Hub after connect.");
  check_err(conn->client.name, INVALID_CLIENT, "No client name configured, how'd you do that?");
  check_err(biseq(conn->host.name, conn->state->them.name), PEER_VERIFY, "Expected Hub name and given Hub name do not match.");
  check_err(biseq(conn->client.name, conn->state->me.name), INVALID_CLIENT, "Final client name and given client name do not match.");
  check_err(biseq(conn->host.key, conn->host_given_key), PEER_VERIFY, "Expected and given Hub keys do not match.");

  bstring host_fp = CryptState_fingerprint_key(conn->state, CRYPT_THEIR_KEY, PK_PUBLIC);
  bstring client_pub_fp = CryptState_fingerprint_key(conn->state, CRYPT_MY_KEY, PK_PUBLIC);
  bstring client_pub = CryptState_export_key(conn->state, CRYPT_MY_KEY, PK_PUBLIC);

  bstring client_prv_fp = CryptState_fingerprint_key(conn->state, CRYPT_MY_KEY, PK_PRIVATE);
  bstring client_prv = CryptState_export_key(conn->state, CRYPT_MY_KEY, PK_PRIVATE);

  Node *host_info = Node_cons("[s[bsww", conn->state->them.name, conn->host_given_key, host_fp, "public", "their");
  check_err(host_info, STACKISH, "Failed to construct valid host name:key response.");

  Node *my_info = Node_cons("[s[bsw[bsww", conn->state->me.name, client_prv, client_prv_fp, "private", client_pub, client_pub_fp, "public", "my");
  check_err(my_info, STACKISH, "Failed to construct valid client name:key response.");

  check_err(Proxy_listener_send(conn, host_info, my_info), LISTENER_IO, "Failed to write initial connect response to listener.");

  log(INFO, "Peering cryptography validated.  Hub should be OK.");
  return 1;
  on_fail(Proxy_dump_invalid_state(conn); return 0);
}



int Proxy_hub_send(Proxy *conn, Node *header, Node *body)
{
  bstring hdata = NULL;
  Node *msg = NULL;
  int rc = 0;
  assert_mem(header);  assert_mem(body); assert_mem(conn);

  hdata = Node_bstr(header, 1);

  msg = CryptState_encrypt_node(conn->state, &conn->state->them.skey, hdata, body);
  check_then(msg, "failed to encrypt payload", rc = -1);

  rc = write_header_node(conn->host.fd, hdata, msg);
  check_err(rc > 0, HUB_IO, "Failed to write encrypted message.");

  ensure(if(msg) Node_destroy(msg);  
      if(hdata) bdestroy(hdata);
      Node_destroy(header);
      Node_destroy(body);
      return rc > 0);
}

Node *Proxy_hub_recv(Proxy *conn, Node **header)
{
  Node *msg = NULL, *packet = NULL;
  bstring hdata = NULL;
  assert_mem(header); assert_mem(conn); 

  packet = readnodes(conn->host.fd, header);
  check_err(packet && *header, HUB_IO, "Failed to read hub message.");

  hdata = Node_bstr(*header, 1);
  check_err(hdata, STACKISH, "Failed to convert stackish header from hub into a string for decrypt.");

  msg = CryptState_decrypt_node(conn->state, &conn->state->me.skey, hdata, packet);
  check_err(msg, CRYPTO, "Failed to decrypt payload from hub.");
  
  ensure(Node_destroy(packet); bdestroy(hdata); return msg);
}


int Proxy_listener_send(Proxy *conn, Node *header, Node *body)
{
  assert_mem(header); assert_mem(conn);

  int rc = writenodes(conn->client.write_fd, header, body);
  check_err(rc > 0, LISTENER_IO, "Failed to write data to client listener.");

  Node_destroy(body); Node_destroy(header);

  ensure(return rc > 0);
}


Node *Proxy_listener_recv(Proxy *conn, Node **header) 
{
  Node *body = readnodes(conn->client.read_fd, header);
  return body;
}



#define FD_SWAP(fd1, fds1, fd2, fds2) FD_SET(fd1, fds1); FD_CLR(fd2, fds2) 

#define PROXY_HANDLE_EVENT(side, readfd, readfn, writefd, writefn)\
  if(FD_ISSET(readfd, &readmask)) {\
    body[side] = readfn(conn, &header[side]);\
    check_err(header[side] && body[side], IO, "message read failure");\
    FD_SWAP(writefd, &writeset, readfd, &readset);\
  }\
\
  if(FD_ISSET(writefd, &writemask)) {\
    check_err(writefn(conn, header[side], body[side]), IO, "message write failure");\
    FD_SWAP(readfd, &readset, writefd, &writeset);\
  }


enum { HUB=0, LISTENER=1 };

int Proxy_event_loop(Proxy *conn)
{
  // find out the maxfd possible
  int maxfd = conn->host.fd < conn->client.read_fd ? conn->client.read_fd : conn->host.fd;
  maxfd = maxfd < conn->client.write_fd ? conn->client.write_fd : maxfd;

  int rc = 0;
  fd_set readmask, readset;
  fd_set writemask, writeset;
  Node *header[2] = {NULL, NULL};
  Node *body[2] = {NULL, NULL};

  assert_mem(conn);

  FD_ZERO(&readset);
  FD_ZERO(&writeset);

  FD_SET(conn->host.fd, &readset);
  FD_SET(conn->client.read_fd, &readset);

  while(1) {
    readmask = readset;
    writemask = writeset;

    rc = select(maxfd+1, &readmask, &writemask, NULL, NULL);
    check_err(rc != 0, IO, "Failed to get an event from select call.");
    check_err(rc > 0, IO, "Error running select event loop.");

    PROXY_HANDLE_EVENT(HUB, conn->host.fd, Proxy_hub_recv, conn->client.write_fd, Proxy_listener_send);
    PROXY_HANDLE_EVENT(LISTENER, conn->client.read_fd, Proxy_listener_recv, conn->host.fd, Proxy_hub_send);
  }

  return 1;
  on_fail(return 0);
}


int Proxy_connect(Proxy *conn) 
{
  Node *rhdr = NULL, *rmsg = NULL;
  Node *imsg = NULL;
  bstring header = NULL;
  int rc = 0;

  assert_mem(conn);

  CryptState_init();

  // create the peer crypto we need
  conn->state = CryptState_create(conn->client.name, conn->client.key);
  check_err(conn->state, CRYPTO, "Failed to initialize cryptography subsystem.");
  conn->state->data = conn;

  log(INFO, "Connecting to %s:%s as %s expecting server named %s", 
      bdata(conn->host.host), bdata(conn->host.port),
      bdata(conn->client.name), bdata(conn->host.name));

  // connect to the hub
  conn->host.fd = tcp_client((char *)bdata(conn->host.host), (char *)bdata(conn->host.port));
  check_err(conn->host.fd >= 0, NET, "Failed to connect to Hub.");

  rmsg = readnodes(conn->host.fd, &rhdr);
  check_err(!rmsg && rhdr, PEER_ESTABLISH, "Incorrectly formatted initial message from Hub.");

  // generate first response
  imsg = CryptState_initiator_start(conn->state, rhdr, &header, Proxy_confirm_key);
  Node_destroy(rhdr); rhdr = NULL;
  check_err(imsg, PEER_ESTABLISH, "Failed to construct response to Hub's first message.");

  rc = write_header_node(conn->host.fd, header, imsg);
  Node_destroy(imsg); imsg = NULL;
  bdestroy(header); header = NULL;
  check_err(rc > 0, PEER_ESTABLISH, "Failed sending response to Hub's first message.");

  rmsg = readnodes(conn->host.fd, &rhdr);
  check_err(rmsg && rhdr, PEER_ESTABLISH, "Invalid response from hub.");
  header = Node_bstr(rhdr, 1); Node_destroy(rhdr); rhdr = NULL;

  rc = CryptState_initiator_process_response(conn->state, header, rmsg);
  check_err(rc, PEER_ESTABLISH, "Failed to process receiver 2nd message.");
  Node_destroy(rmsg); bdestroy(header); header = NULL; rmsg = NULL;

  imsg = CryptState_initiator_send_final(conn->state, &header);
  check_err(imsg, PEER_ESTABLISH, "Failed to generate final message.");

  rc = write_header_node(conn->host.fd, header, imsg);
  check_err(rc > 0, PEER_ESTABLISH, "Failed to write final message to hub.");

  check_err(Proxy_final_verify(conn), PEER_VERIFY, "Final verify of Hub info failed.");

  // remember, rc>0 is how we make sure that everything went ok during processing
  ensure(if(imsg) Node_destroy(imsg);
      if(rmsg) Node_destroy(rmsg);
      if(rhdr) Node_destroy(rhdr);
      if(header) bdestroy(header);
      return rc > 0);
}


int Proxy_listen(Proxy *conn, ProxyClientType type, bstring name)
{
  switch(type) {
    case PROXY_CLIENT_DOMAIN:
      check(name, "DOMAIN client IO specified but NULL given for the name.");
      log(INFO, "Attaching to listener server socket on %s.", bdata(name));
      conn->client.read_fd = conn->client.write_fd =  unix_client(name);
      break;
    case PROXY_CLIENT_FIFO:
      check(name, "FIFO client IO specified but NULL given for the name.");
      bstring in_file = bformat("%s.in", bdata(name));
      bstring out_file = bformat("%s.out", bdata(name));
      check(!mkfifo((const char *)in_file->data, 0600), "Failed to create input fifo.");
      check(!mkfifo((const char *)out_file->data, 0600), "Failed to create output fifo.");

      log(INFO, "Opening %s for the read FIFO end.", bdata(in_file));
      conn->client.read_fd = open((const char *)in_file->data, O_RDONLY);
      check(conn->client.read_fd >= 0, "Failed to open read end of FIFO.");

      log(INFO, "Opening %s for the write FIFO end.", bdata(out_file));
      conn->client.write_fd = open((const char *)out_file->data, O_WRONLY);
      check(conn->client.write_fd >= 0, "Failed to open write end of FIFO.");

      bdestroy(in_file);
      bdestroy(out_file);
      break;
    case PROXY_CLIENT_STDIO:
      log(INFO, "Listening on stdin/stdout.");
      conn->client.read_fd = 0;
      conn->client.write_fd = 1;
      break;
    case PROXY_CLIENT_NONE:
      fail("You did not set a client connection type.");
      break;
    default:
      fail("Invalid proxy client type given.");
  }

  check_err(conn->client.read_fd >= 0, LISTENER_IO, "Failed to start listener read side.");
  check_err(conn->client.write_fd >= 0, LISTENER_IO, "Failed to start listener write side.");

  dbg("Client listening on read_fd %d and write_fd %d", conn->client.read_fd, conn->client.write_fd);

  return 1;
  on_fail(return 0);
}


int Proxy_configure(Proxy *conn)
{
  int rc = 0;
  Node *host = NULL;
  Node *client = NULL;

  // read the configuration stackish from the client
  log(INFO, "waiting for initial config");
  host = readnodes(conn->client.read_fd, &client);
  check_err(host && client, CONFIG, "Received an invalid host and client configuration.");

  rc = Node_decons(host, 1, "[bsssw", 
      &conn->host.key, &conn->host.name, 
      &conn->host.port, &conn->host.host, "host");
  check_err(rc, CONFIG, "Invalid configuration format for host: [sssbw");

  rc = Node_decons(client, 1, "[bsw", &conn->client.key, &conn->client.name, "client");
  check_err(rc, CONFIG, "Invalid configuration format for client: [sbw");

  // set any keys that are 0 length to NULL so they are generated
  if(blength(conn->host.key) == 0) {
    log(WARN, "No expected host key given, will accept anything.");
    bdestroy(conn->host.key);
    conn->host.key = NULL;
  }

  if(blength(conn->client.key) == 0) {
    log(WARN, "No client key given, will generate one.");
    bdestroy(conn->client.key);
    conn->client.key = NULL;
  }

  ensure(if(host) Node_destroy(host); 
      if(client) Node_destroy(client); 
      return rc);
}

void Proxy_error(Proxy *conn, UtuMendicantErrorCode code, bstring message)
{
  Node *err = Node_cons("[nbw", (uint64_t)code, message, "err");
  Node_dump(err, ' ', 1);
  // don't use Proxy_listener_send since that could be causing a write error
  // just try writing, and ignore the write if it failed
  writenodes(conn->client.write_fd, err, NULL);
  Node_destroy(err);
}
