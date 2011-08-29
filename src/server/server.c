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

#include "hub/hub.h"
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

bstring pid_file_name = NULL;


int redirect_out_logs(const char *log_file)
{
  stdout = freopen(log_file, "a", stdout);
  check(stdout, "Failed to divert stdout to requested log file.");
  check(!setvbuf(stderr, (char *)NULL, _IONBF, 0), "Failed to make stdout unbuffered.");

  stderr = freopen(log_file, "a", stderr);
  check(stderr, "Failed to divert stderr to requested log file.");
  check(!setvbuf(stderr, (char *)NULL, _IONBF, 0), "Failed tomake stderr unbuffered.");

  check(!fclose(stdin), "Failed to close stdin.");

  return 1;
  on_fail(return 0);
}

bstring load_key_file(const char *file)
{
  bstring key = NULL;
  FILE *key_file = fopen(file, "r");
  check(key_file, "failed to open given key file (put -m before -k to make a new key)");

  log(INFO, "reading key from %s", file);

  key = bread((bNread)fread, key_file);
  check(key, "failed to read key from key file");

  ensure(if(key_file) fclose(key_file); return key);
}

int create_key_file(const char *file, bstring key)
{
  if(!access(file, F_OK) || errno != ENOENT) {
    fail("Requested key file to already exists. Will not make a new one (drop -m or remove the file).");
  } else {
    errno = 0;
  }

  FILE *key_file = fopen(file, "w+");
  check(key_file, "failed to open given key file to create it");

  log(INFO, "Creating key file %s", file);

  check(fwrite(key->data, (size_t)blength(key), 1, key_file) > 0, "failed to write key to file");

  fclose(key_file);

  return 1;
  on_fail(return 0);
}

void usage(const char *message) 
{
  if(message) {
    printf("ERROR: %s\n", message);
  }

  printf("USAGE: utuserver -a addr -p port -n name [-d chroot] [-k keyfile] [-m] [-u uid -g gid] [-l server.log]\n");
}

void remove_pid_atexit()
{
  if(pid_file_name) unlink((const char *)pid_file_name->data);
}

int write_pid(bstring addr, bstring port, bstring name)
{
  pid_file_name = bformat("/var/run/utuserver-%s-%s-%s.pid", bdata(addr), bdata(port), bdata(name));
  int rc = 0;

  FILE *pid_file = fopen((const char *)bdata(pid_file_name), "w");

  check(pid_file, "failed to open pid file");
  log(INFO, "PID file written to %s with PID %d", bdata(pid_file_name), getpid());
  check(fprintf(pid_file, "%d", getpid()) > 0, "failed to write pid to pid file");
  check(atexit(remove_pid_atexit) == 0, "Cannot set cleanup function at exit.");

  rc = 1;

  ensure(if(pid_file) fclose(pid_file); return rc);
}

int main (int argc, char *argv[])
{
  bstring addr = NULL, port = NULL, name = NULL, key = NULL;
  int uid = 0, gid = 0;
  int daemonize = 0;
  int make_new_key = 0;
  const char *key_file = "utuserver.key";
  const char *chroot = "/var/run/utu";
  int rc = 0;

  uid = geteuid();
  gid = getegid();

  while((rc = getopt(argc, argv, "ha:p:n:k:d:g:u:m:l:")) != -1) {
    switch(rc) {
      case 'h':
        usage(NULL);
        return 0;
      case 'a':
        addr = bfromcstr(optarg);
        break;
      case 'p':
        port = bfromcstr(optarg);
        break;
      case 'n':
        name = bfromcstr(optarg);
        break;
      case 'd':
        daemonize = 1;
        chroot = optarg;
        break;
      case 'k':
        if(!make_new_key) {
          key = load_key_file(optarg);
        } else {
          key = NULL;
          key_file = optarg;
        }
        break;
      case 'u':
        uid = atoi(optarg);
        break;
      case 'm':
        make_new_key = 1;
        break;
      case 'g':
        gid = atoi(optarg);
        break;
      case 'l':
        check(redirect_out_logs(optarg), "Failed to create log file.");
        break;
      default:
        usage("invalid arguments");
        return 1;
    }
  }

  if(!addr || !port || !name) {
    usage("you need -a, -p, AND -n options");
    return 1;
  }

  Hub_init(argv[0]);

  if(daemonize) {
    log(INFO, "Daemonizing into chroot %s with uid:gid == %d:%d", chroot, uid, gid);
    check(daemon(0,1) != -1, "failed to daemonize");
    check(write_pid(addr, port, name), "failed to write pid file");
    check(server_chroot_drop_priv((char *)chroot, uid, gid), "failed to chroot");
  }

  Hub *hub = Hub_create(addr, port, name, key);
  check(hub, "Failed to initialize Utu Hub for operations.");

  if(!key && make_new_key) {
    check(create_key_file(key_file, hub->key), "Failed to create key file");
  }

  Hub_listen(hub);

  Hub_destroy(hub);

  return 0;
  on_fail(return 1);
}
