//bplus_free_list_head use_free_offset for outside use
//we can only get node from the head of the queue


#ifndef BPLUS_LIST_H
#define BPLUS_LIST_H
#include "bplus_common.h"
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
typedef struct bplus_free_node{
    off_t offset;
    struct bplus_free_node *next;
    struct bplus_free_node *pre;
}bplus_free_node;

typedef struct bplus_free_list_head{
    bplus_free_node *first;
}bplus_free_list_head;

static inline void remove_node_from_list(bplus_free_node *p_node, bplus_free_list_head *p_head){
    if(p_node->next){
        p_node->next->pre = p_node->pre;
        p_node->pre->next = p_node->next;
    }
    if(p_head->first == p_node){
        if(p_node->next != p_node){
            p_head->first = p_node->next;
        }else{
            p_head->first = NULL;
        }
    }
}

off_t use_first_free_offset(bplus_free_list_head *p_head){
    bplus_free_node *p_free_node = p_head->first;
    if(p_free_node == NULL)
        return OFFSET_NULL;

    off_t res = p_free_node->offset;
    printf("removing %lld\n", res);
    remove_node_from_list(p_free_node, p_head);
    free(p_free_node);

    return res;
}

enum {
    FREE_OFFSET_HEAD,
    FREE_OFFSET_TAIL,
};

void add_free_offset(bplus_free_list_head *p_head, off_t offset, int position){
    bplus_free_node *p_free_node = malloc(sizeof(bplus_free_node));
    assert(p_free_node != NULL);
    p_free_node->offset = offset;
    p_free_node->next = p_head->first;
    if(p_head->first){
        p_free_node->pre = p_head->first->pre;
        p_free_node->next = p_head->first;
        p_head->first->pre->next = p_free_node;
        p_head->first->pre = p_free_node;
    }else{
        p_free_node->next = p_free_node;
        p_free_node->pre = p_free_node;
    }
    if(position == FREE_OFFSET_HEAD || !p_head->first){
        p_head->first = p_free_node;
    }
}

bplus_free_list_head *get_free_list_head(){
    bplus_free_list_head *temp_head = malloc(sizeof(bplus_free_list_head));
    assert(temp_head != NULL);
    temp_head->first = NULL;

    return temp_head;
}

bplus_free_list_head *init_build_free_list_head(unsigned int node_count, off_t *offs, int position){
    bplus_free_list_head *temp_head = get_free_list_head();
    while(node_count-- > 0)
        add_free_offset(temp_head, *(offs++), position);

    return temp_head;
}

void free_list_traverse(bplus_free_list_head *p_head){
    if(p_head == NULL){
        printf("p_head null\n");
    }else if(p_head->first == NULL){
        printf("free list empty\n");
    }else{
        bplus_free_node *p_free_node = p_head->first;
        do{
            printf("%lld ", p_free_node->offset);
            p_free_node = p_free_node->next;
        }while(p_free_node != p_head->first);
    }
}

#endif
