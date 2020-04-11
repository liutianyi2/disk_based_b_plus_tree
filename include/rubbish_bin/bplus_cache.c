//original realization
//this part has a lot of optimization to be done
#include "bplus_cache.h"
#include<stdlib.h>
#include<assert.h>

typedef struct bplus_cache_manager{
    size_t cache_size;
    unsigned int cache_cn;
    int *used;
    char *buf;
}bplus_cache_manager;

bplus_cache_manager * init_bplus_cache_manager(size_t cache_size, unsigned int least_cache_cn){
    bplus_cache_manager *p_manager = malloc(sizeof(bplus_cache_manager));
    assert(p_manager != NULL);
    cache_size = BPLUS_ROUND_UP(cache_size, sizeof(long));
    p_manager->buf = malloc(cache_size * least_cache_cn);
    assert(p_manager->buf != NULL);
    p_manager->used = calloc(least_cache_cn, sizeof(int));
    assert(p_manager->used != NULL);

    p_manager->cache_size = cache_size;
    p_manager->cache_cn = least_cache_cn;

    return p_manager;
}

void *bplus_get_free_cache(bplus_cache_manager *p_manager){
    int i = 0;
    while(i < p_manager->cache_cn){
        if(*(p_manager->used + i) == 0)
            break;
        i++;
    }
    assert(i != p_manager->cache_cn);
    *(p_manager->used + i) = 1;
#ifdef BPLUS_DEBUG
    printf("using %d\n", i);
#endif

    void *ret = (void *)(p_manager->buf + i * p_manager->cache_size);
    //printf("%lld\n%lld\n", (char *)p_manager->buf - (char*)NULL, (char *)(ret) - (char*)NULL);
    return ret;
}

void bplus_put_cache(bplus_cache_manager *manager, void *cache){
    int i = ((char *)cache - (char *)(manager->buf)) / manager->cache_size;
#ifdef BPLUS_DEBUG
    printf("returning %d\n", i);
#endif
    manager->used[i] = 0;
}

