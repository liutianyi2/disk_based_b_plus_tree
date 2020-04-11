#ifndef BPLUS_LIST_H
#define BPLUS_LIST_H
#include "bplus_common.h"
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
typedef struct bplus_free_node bplus_free_node;

typedef struct bplus_free_list_head bplus_free_list_head;

off_t use_first_free_offset(bplus_free_list_head *p_head);

enum {
    FREE_OFFSET_HEAD,
    FREE_OFFSET_TAIL,
};

void add_free_offset(bplus_free_list_head *p_head, off_t offset, int position);

bplus_free_list_head *get_free_list_head();

bplus_free_list_head *init_build_free_list_head(unsigned int node_count, off_t *offs, int position);

void free_list_traverse(bplus_free_list_head *p_head);

#endif
