/*
 * Myriad -- Coroutine Based Network Simplification Library
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


#ifndef myriad_defend_h
#define myriad_defend_h

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


#ifdef MYRIAD_LOG_SYSLOG
#include <syslog.h>

#ifndef NDEBUG
#define dbg(M, ...) syslog(LOG_DAEMON, "[DEBUG %s:%d] " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define dbg(M, ...)
#endif

#define log(L, M, ...) syslog(LOG_DAEMON, "[" #L "] " M "\n", ##__VA_ARGS__)

#else

#ifndef NDEBUG
#define dbg(M, ...) fprintf(stderr, "[DEBUG %s:%d] " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define dbg(M, ...)
#endif

#define log(L, M, ...) fprintf(stderr, "[" #L "] " M "\n", ##__VA_ARGS__)

#endif

#define fail(M) log(FAIL, "(%s:%d) %s  {err: %s}", __FILE__, __LINE__, M, strerror(errno)); errno=0; goto _check_fail;
#define check(T,M) if(!(T) || errno) { fail(M) };
#define check_then(T,M,C) if(!(T) || errno) { C; fail(M) };
#define check_errno(T,E) check_then(T, "check " #T "failed, errno set " # E, errno = E)
#define on_fail(C) _check_fail: C; 
#define ensure(C) _check_fail: C;
#define trace() log(TRACE, ">> %s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
#define precond(T) check(T, "FAIL pre-condition: " #T)
#define postcond(T) check(T, "FAIL post-condition: " #T)
#define sentinel() log(FATAL, "(%s:%d) sentinel reached!", __FILE__, __LINE__)
#define assert_not(V,N) assert((V) != (N) && "" # V " cannot be " # N);
#define assert_mem(V) assert((V) != NULL && "memory alloc failure");
#endif
