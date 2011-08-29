#include "io.h"


#define error(X,Y,Z) abort()

#define NTIMERS 25

/* readn - read exactly n bytes */
int readn(int fd, char *bp, size_t len)
{
  int cnt;
  int rc;

  assert_mem(bp);
  assert(fd >= 0 && "invalid socket on readn");
  assert(len > 0 && "attempt to read 0");

  cnt = len;
  while ( cnt > 0 )
  {
    rc = read(fd, bp, cnt);

    if ( rc < 0 )                           /* read error ? */
    {
      if ( errno == EINTR )   /* interrupted? */
        continue;                       /* restart the read */
      return -1;                              /* return error */
    }
    if ( rc == 0 )                          /* EOF? */
      return len - cnt;               /* return short count */
    bp += rc;
    cnt -= rc;
  }

  assert(errno == 0 && "damn errno leak");
  return len;
}


bstring readframe(int fd)
{
  u_int16_t reclen = 0;
  int rc;
  bstring result = NULL;

  assert(fd >= 0 && "invalid socket on readframe");

  /* Retrieve the length of the record */
  rc = readn(fd, ( char * )&reclen, sizeof(reclen));
  check(rc == sizeof(reclen), "failed to read record length");

  /* convert from network order */
  reclen = ntohs( reclen );
  result = bfromcstralloc(reclen+1, "");
  assert_mem(result);

  /* read that much exactly */
  rc = readn( fd, (char *)result->data, reclen );
  check_then(rc == reclen, "invalid record length from listener", bdestroy(result); result = NULL);

  /* make sure the size really is enforced */
  bsetsize(result, reclen);


  /* all done */
  ensure(return result);
}

Node *readnodes(int fd, Node **header)
{
  Node *body = NULL;
  *header = NULL;
  size_t from = 0;
  
  assert_mem(header);
  assert(fd >= 0 && "invalid socket");

  bstring data = readframe(fd);
  check(data, "failed to read framing record");

  *header = Node_parse_seq(data, &from);
  check(*header, "failed to read header node");

  body = Node_parse_seq(data, &from);
  
  ensure(bdestroy(data); return body);
}


int writenodes(int fd, Node *header, Node *body) 
{
  int rc = 0;
  bstring data = bfromcstr("");

  assert_mem(header);
  assert(fd >= 0 && "file descriptor invalid");

  Node_catbstr(data, header, ' ', 1);

  rc = write_header_node(fd, data, body);
  check(rc, "failed writing both nodes");

  ensure(bdestroy(data); return rc);
}

int write_header_node(int fd, bstring header, Node *body) 
{
  int rc = 0;
  struct iovec iov[2];
  u_int16_t reclen = 0;

  assert_mem(header);
  assert(fd >= 0 && "file descriptor invalid");

  if(body) Node_catbstr(header, body, ' ', 1);
  reclen = htons(blength(header));

  iov[0].iov_len = sizeof(reclen);
  iov[0].iov_base = &reclen;
  iov[1].iov_len = blength(header);
  iov[1].iov_base = bdata(header);

  rc = writev(fd, iov, 2);
  check(rc > 0, "write failure");

  ensure(return rc);
}

int set_address( char *hname, char *sname, struct sockaddr_in *sap, char *protocol )
{
	struct servent *sp;
	struct hostent *hp;
	char *endptr;
	short port;

  assert_mem(hname);
  assert_mem(sname);
  assert_mem(sap);
  assert_mem(protocol);

	bzero( sap, sizeof( *sap ) );
	sap->sin_family = AF_INET;
	if ( hname != NULL )
	{
		if ( !inet_aton( hname, &sap->sin_addr ) )
		{
			hp = gethostbyname( hname );
			check(hp != NULL, "unknown host" );
			sap->sin_addr = *( struct in_addr * )hp->h_addr;
		}
	}
	else
		sap->sin_addr.s_addr = htonl( INADDR_ANY );
	port = strtol( sname, &endptr, 0 );
	if ( *endptr == '\0' )
		sap->sin_port = htons( port );
	else
	{
		sp = getservbyname( sname, protocol );
		check( sp != NULL, "unknown service");
		sap->sin_port = sp->s_port;
	}

  return 1;
  on_fail(return 0);
}

/* tcp_client - set up for a TCP client */
int tcp_client( char *hname, char *sname )
{
	struct sockaddr_in peer;
	int s;

  assert_mem(hname);
  assert_mem(sname);

	check(set_address( hname, sname, &peer, "tcp" ), "failed to set address");
	s = socket(AF_INET, SOCK_STREAM, 0);
	check(s >= 0, "socket call failed");

	check(!connect(s, (struct sockaddr *)&peer, sizeof(peer)), "connect failed");

	return s;

  on_fail(return -1);
}


int unix_client(bstring hname)
{
	struct sockaddr_un local = { .sun_family = AF_UNIX, .sun_path = {0} };
	int s = 0;

  assert_mem(hname);

  check((unsigned int)blength(hname) <= sizeof(local) - sizeof(local.sun_family), "name too long");

  memcpy(local.sun_path, hname->data, blength(hname));

	s = socket(AF_UNIX, SOCK_STREAM, 0);
  check(isvalidsock(s), "socket call failed");

  // set it to nonblocking here

	check(connect( s, ( struct sockaddr * )&local, sizeof( local ) ) == 0, "connect failed");

  return s;
  on_fail(return -1);
}

