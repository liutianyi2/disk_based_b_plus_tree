//cache management should be very different from the original one, so I pick it out.
#ifndef BPULS_CACHE_H
#define BPULS_CACHE_H
#include<stdio.h>
#include "bplus_common.h"
typedef struct bplus_cache_manager bplus_cache_manager;
bplus_cache_manager * init_bplus_cache_manager(size_t cache_size, unsigned int least_cache_cn);
//之后缓存的管理将是重点，会增加参数
void *bplus_get_free_cache(bplus_cache_manager *manager, long indicator);
void bplus_put_cache(bplus_cache_manager *manager, void *cache);

enum {
    BPLUS_DATA_IN,
    BPLUS_DATA_NEEDED,
};
typedef struct bplus_cache_header{
    int is_data_in;
    long indicator;
}bplus_cache_header;

#define BPLUS_CACHE_NULL_INDICATOR -1

#endif
