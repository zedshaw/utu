#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "info.h"
#include <myriad/defend.h>

bstring Info_list(bstring path, bstring *error)
{
  bstring listing = NULL;
  DIR *dir = NULL;
  struct dirent *dptr = NULL;
  *error = NULL;

  dir = opendir((char *)path->data);
  check_then(dir, "Failed to open requested directory for info request.", *error = bfromcstr(strerror(errno)));

  
  for(listing = bfromcstr(""); (dptr = readdir(dir)) != NULL; bconchar(listing, '\n')) {
    bcatcstr(listing, dptr->d_name);
  }

  if(blength(listing) > 64*1024 - 200) {
    *error = bfromcstr("Requested listing is larger than allowed message size.");
    fail("Return results for directory listing were too large.");
  }

  ensure(if(dir) closedir(dir); return listing);
}

bstring Info_tag(bstring path, bstring *error)
{
  struct stat fst;
  bstring tag = NULL;
  *error = NULL;

  check_then(stat((char *)path->data, &fst) == 0, "Failed to stat info request file.", *error = bfromcstr(strerror(errno)));

  tag = bformat("%d-%d-%d", fst.st_ino, fst.st_size, fst.st_mtime);

  ensure(return tag);
}

bstring Info_get(bstring path, bstring *error)
{
  FILE *in = NULL;
  *error = NULL;
  bstring data = NULL;

  dbg("Serving info file %s", bdata(path));
  in = fopen((char *)bdata(path), "rb");
  check_then(in, "Failed to open requested file.", *error = bfromcstr(strerror(errno)));

  data = bread((bNread)fread, in);
  check_then(data, "Failed to read requested file.", *error = bfromcstr(strerror(errno)));

  ensure(if(in) fclose(in); return data);
}

bstring Info_put(bstring path, bstring data)
{
  return NULL;
}

int Info_delete(bstring path)
{
  return 1;
}

