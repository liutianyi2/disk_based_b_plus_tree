#ifndef BPLUS_H
#define BPLUS_H
#define BPLUS_TREE_CACHE_CN 10
//改小便于测试
#define BPLUS_PAGE_SIZE (1024 * 0.2)
#include "bplus_list.h"
#include "bplus_cache.h"
#ifndef BPLUS_KEY_TYPE
#define BPLUS_KEY_TYPE bplus_key_type
#endif
#ifndef BPLUS_DATA_TYPE
#define BPLUS_DATA_TYPE bplus_data_type
#endif
#define MAX_PATH_LEN 200
typedef int (*bplus_larger_than_fun)(BPLUS_KEY_TYPE *p_a, BPLUS_KEY_TYPE *p_b);

typedef void (*print_key_data)(BPLUS_KEY_TYPE *p_key, BPLUS_DATA_TYPE *p_data);

typedef struct bplus_node{
    //cache复用，暂不使用
    bplus_cache_header header;
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
    off_t file_size;
    int level;
    bplus_free_list_head *free_list;
    bplus_cache_manager *manager;
    bplus_larger_than_fun larger_than;
    char path[MAX_PATH_LEN];
    int fd;
}bplus_tree;

bplus_tree *init_bplus_tree(unsigned int hint_cache_size, const char *name, bplus_larger_than_fun p_fun);

void bplus_tree_info(const bplus_tree *p_tree);

int bplus_add_key_value(bplus_tree *tree, BPLUS_KEY_TYPE *p_key, BPLUS_DATA_TYPE *p_data);

enum {
    BPLUS_ADD_SUCCESS,
    BPLUS_ADD_ALREADY_HAS,
    BPLUS_ADD_INDEX_FOUND,
};

enum {
    BPLUS_DELETE_NOT_FOUND,
    BPLUS_DELETE_SUCCESS,
};

void bplus_tree_traverse(bplus_tree *tree, print_key_data p_fun);

void bplus_range_search(bplus_tree *tree, BPLUS_KEY_TYPE *p_key_small, BPLUS_KEY_TYPE *p_key_big, BPLUS_KEY_TYPE *key_buf, BPLUS_DATA_TYPE *data_buf, size_t *buf_size);

int bplus_remove_key(bplus_tree *tree, BPLUS_KEY_TYPE *p_key);

#endif
