#ifndef BPLUS_H
#define BPLUS_H
#define BPLUS_TREE_CACHE_CN 10
#define BPLUS_PAGE_SIZE (1024 * 4)
#include "bplus_list.h"
#include "bplus_cache.h"
#ifndef BPLUS_KEY_TYPE
#define BPLUS_KEY_TYPE long
#endif
#ifndef BPLUS_DATA_TYPE
#define BPLUS_DATA_TYPE long
#endif
#define MAX_PATH_LEN 200

typedef struct bplus_node{
    off_t self;
    off_t parent;
    off_t left;
    off_t right;
    int type;
    int children;
}bplus_node;

typedef struct bplus_tree{
    size_t value_size;
    size_t page_size;
    int entry_count;
    int data_count;
    off_t root;
    bplus_free_list_head *free_list;
    bplus_cache_manager *manager;
    char path[MAX_PATH_LEN];
    int fd;
}bplus_tree;

bplus_tree *init_bplus_tree(unsigned int hint_cache_size, const char *name);

void bplus_tree_info(const bplus_tree *p_tree);

#endif
