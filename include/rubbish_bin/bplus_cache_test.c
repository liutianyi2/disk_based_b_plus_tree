#include "bplus_cache.h"


int main(int argc, char **argv){
    bplus_cache_manager *manager = init_bplus_cache_manager(1000, 10);
    void *pointers[10];
    int i = 10;
    while(i > 0){
        pointers[i - 1] = bplus_get_free_cache(manager);
        i--;
    }
    i = 5;
    while(i > 0){
        bplus_put_cache(manager, *(pointers + (i - 1) * 2));
        i--;
    }
    i = 5;
    while(i > 0){
        pointers[i - 1] = bplus_get_free_cache(manager);
        i--;
    }
}

