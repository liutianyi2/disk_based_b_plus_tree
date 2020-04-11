#include "bplus.h"
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#define keys(p_node) ((BPLUS_KEY_TYPE *)((char *)p_node + sizeof(bplus_node)))
#define subs(p_node, p_tree) ((off_t *)(keys(p_node) + p_tree->entry_count))
#define datas(p_node) ((BPLUS_DATA_TYPE *)((char *)p_node + sizeof(bplus_node)))

enum {
    LEAF_NODE,
    NON_LEAF_NODE,
};

bplus_tree *init_bplus_tree(unsigned int hint_cache_size, const char *name){
    bplus_tree *tree = malloc(sizeof(bplus_tree));
    assert(tree != NULL);
    tree->value_size = sizeof(BPLUS_DATA_TYPE);
    tree->page_size = BPLUS_PAGE_SIZE;
    tree->root = OFFSET_NULL;
    tree->free_list = get_free_list_head();
    tree->manager = init_bplus_cache_manager(tree->page_size, hint_cache_size);
    char *res = getcwd(tree->path, MAX_PATH_LEN - 50);
    assert(res != NULL);
    strcpy(tree->path + strlen(tree->path), "../data/");
    res = strncpy(tree->path + strlen(tree->path), name, MAX_PATH_LEN - strlen(tree->path) - 10);
    assert(res != NULL);
    strcpy(tree->path + strlen(tree->path), ".bplus");
    tree->fd = open(tree->path, O_RDWR | O_CREAT);
    assert(tree->fd >= 0);

    // 暂不考虑对齐
    tree->entry_count = (tree->page_size - sizeof(bplus_node)) / (sizeof(off_t) + sizeof(BPLUS_KEY_TYPE));
    tree->data_count = (tree->page_size - sizeof(bplus_node)) / tree->value_size;

    return tree;
}

void bplus_tree_info(const bplus_tree *p_tree){
    printf("value size: %lu\n", p_tree->value_size);
    printf("page size: %lu\n", p_tree->page_size);
    printf("entry count: %d\n", p_tree->entry_count);
    printf("data count: %d\n", p_tree->data_count);
    printf("root: %lld\n", p_tree->root);
    printf("path:%s\n", p_tree->path);
    printf("fd:%d\n", p_tree->fd);
}

