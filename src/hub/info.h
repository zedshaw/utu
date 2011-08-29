#ifndef utu_hub_info_h
#define utu_hub_info_h

#include <myriad/bstring/bstrlib.h>

bstring Info_list(bstring path, bstring *error);

bstring Info_get(bstring path, bstring *error);

bstring Info_put(bstring path, bstring data);

int Info_delete(bstring path);

bstring Info_tag(bstring path, bstring *error);

#endif
