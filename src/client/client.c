/*
 * Utu -- Saving The Internet With Hate
 *
 * Copyright (c) Zed A. Shaw 2005 (zedshaw@zedshaw.com)
 *
 * This file is modifiable/redistributable under the terms of the GNU
 * General Public License.
 *
 * You should have recieved a copy of the GNU General Public License along
 * with this program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 0211-1307, USA.
 */

#include <getopt.h>
#include "proxy.h"

void usage(const char *msg)
{
  if(msg) fprintf(stderr, "ERROR: %s\n", msg);

  printf("usage: [-h] [-f fifo | -i | -d domain] [-s | -v] -m hate_max -e err.log\n");

  exit(1);
}


/**
 * Called by main to do the one off validation or generation
 * of signatures.  It takes a single [validate or [sign
 * header, and a body of what to process.  It then processes
 * the body and returns the same header and the results.
 * The return from validation includes the key that was
 * used to validate (taken from the signature payload).
 *
 * It *always* returns something, even if that something
 * is an error message in the standard format.
 */
void client_signature_process(Proxy *conn)
{
  Node *hdr = NULL;
  Node *body = Proxy_listener_recv(conn,  &hdr);
  Node *result = NULL;
  int sig_stat=0;
  ecc_key pkey;
  bstring name = NULL, key = NULL;

  CryptState_init();

  check_err(hdr, CONFIG, "Must give a [sign or [validate node.");
  check_err(body, CONFIG, "Must give a body to sign.");

  // header determines whether to sign or validate
  // we don't give a shit about memory leaks since we exit right away
  if(biseq(hdr->name, bfromcstr("validate"))) {

    check_err(Node_decons(hdr, 0, "[sw", &name, "validate"), STACKISH, "Invalid validate node.");
    result = CryptState_verify_signature(&pkey, name, body, &sig_stat);
    check_err(sig_stat, CRYPTO, "Signature did not validate for given node.");

    // add the extracted key onto the validate result
    bstring res_key = CryptState_ecc_key_bstr(&pkey, PK_PUBLIC);
    check_err(res_key, CRYPTO, "Invalid ECC public key given in signature.");
    Node_new_blob(hdr, res_key);

  } else if(biseq(hdr->name, bfromcstr("sign"))) {

    check_err(Node_decons(hdr, 0, "[bsw", &key, &name, "sign"), STACKISH, "Invalid sign node.");
    check_err(CryptState_bstr_ecc_key(&pkey, key), CRYPTO, "Failed to import ECC private key.");
    result = CryptState_sign_node(&pkey, name, body);

  } else {
    check_err(0, CONFIG, "Header should be [sign or [validate, nothing else.");
  }

  check_err(result, CRYPTO, "Failed to process requested sign/validate.");
  Proxy_listener_send(conn, hdr, result);

  exit(0);
  on_fail(fflush(stderr);  exit(1));
}


int main( int argc, char **argv )
{
  int rc = 0;
  bstring io_name = NULL;
  int io_type = 0;
  int sign_only = 0;
  unsigned int max_hate_level = 24;
  Proxy conn;
  memset(&conn, 0, sizeof(conn));

  while((rc = getopt(argc, argv, "hf:id:sm:e:c:")) != -1) {
    switch(rc) {
      case 'h': usage(NULL);
                break;
      case 'f': io_type = PROXY_CLIENT_FIFO;
                io_name = bfromcstr(optarg); 
                break;
      case 'i': io_type = PROXY_CLIENT_STDIO;
                break;
      case 'd': io_type = PROXY_CLIENT_DOMAIN; 
                io_name = bfromcstr(optarg);
                break;
      case 's': sign_only = 1;
                break;
      case 'm': max_hate_level = atoi(optarg);
                break;
      case 'e': freopen(optarg, "a", stderr);
                setvbuf(stderr, (char *)NULL, _IONBF, 0);
                break;
    }
  }

  if(io_type == PROXY_CLIENT_NONE) {
    usage("Specify either -f, -d, or -i style client IO.");
  }

  if(io_type != PROXY_CLIENT_STDIO && !io_name) {
    usage("You must specify a name for FIFO and DOMAIN client IO types.");
  }

  if(max_hate_level > 64) {
    usage("You specified an insane max hate level. (Must be less than 65 for 2^64)");
  }

  log(INFO, "Enforcing secure umask.");
  umask(S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

  rc = Proxy_listen(&conn, io_type, io_name);
  check(rc, "Failed to listen on local domain socket.");

  if(sign_only) client_signature_process(&conn);

  rc = Proxy_configure(&conn);
  check(rc, "Failed to configure mendicant proxy.");

  rc = Proxy_connect(&conn);
  check(rc, "Failed to connect to remote hub.");

  rc = Proxy_event_loop(&conn);
  check(rc, "Event loop aborted with failure.");

  rc = 0;
  ensure(Proxy_destroy(&conn); return rc);
}

