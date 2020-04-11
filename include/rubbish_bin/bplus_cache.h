//cache management should be very different from the original one, so I pick it out.
#ifndef BPULS_CACHE_H
#define BPULS_CACHE_H
#include<stdio.h>
#include "bplus_common.h"
typedef struct bplus_cache_manager bplus_cache_manager;
bplus_cache_manager * init_bplus_cache_manager(size_t cache_size, unsigned int least_cache_cn);
void *bplus_get_free_cache(bplus_cache_manager *manager);
void bplus_put_cache(bplus_cache_manager *manager, void *cache);
#endif
