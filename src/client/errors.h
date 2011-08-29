#ifndef utu_mendicant_errors_h
#define utu_mendicant_errors_h

typedef enum { 
  UTU_ERR_INVALID_HUB=400,
  UTU_ERR_INVALID_CLIENT,
  UTU_ERR_STACKISH,
  UTU_ERR_HUB_IO,
  UTU_ERR_LISTENER_IO,
  UTU_ERR_IO,
  UTU_ERR_CRYPTO,
  UTU_ERR_NET,
  UTU_ERR_PEER_ESTABLISH,
  UTU_ERR_PEER_VERIFY,
  UTU_ERR_CONFIG,
} UtuMendicantErrorCode;

/**
 * Like the normal check macro but handles sending stackish formatted
 * errors back to the listener.
 */
#define check_err(T,N,M) if(!(T)) { Proxy_error(conn, UTU_ERR_##N, bfromcstr(M)); check(T,M); }

#endif
