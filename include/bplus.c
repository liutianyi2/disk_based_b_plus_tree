#include "bplus.h"
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#define keys(p_node) ((BPLUS_KEY_TYPE *)((char *)p_node + sizeof(bplus_node)))
#define subs(p_node, p_tree) ((off_t *)(keys(p_node) + p_tree->entry_count))
#define datas(p_node, p_tree) ((BPLUS_DATA_TYPE *)(keys(p_node) + p_tree->data_count + 1))
#define INDEX_TRANSFORM(index) (index > 0 ? index : (index == -1 ? 0 : (-index - 2)))
typedef int (*larger_than_fun)(BPLUS_KEY_TYPE *p_a, BPLUS_KEY_TYPE *p_b);

enum {
    LEAF_NODE,
    NON_LEAF_NODE,
};

enum {
    LEFT_SIBLING,
    RIGHT_SIBLING,
};

enum {
    NODE_ATTACH_NODE_ONLY,
    NODE_ATTACH_DATA_ALL,
};

static inline off_t get_next_free_off(bplus_tree *tree){
    off_t next = use_first_free_offset(tree->free_list);
    if(next == OFFSET_NULL){
        next = tree->file_size;
        tree->file_size += BPLUS_PAGE_SIZE;
    }

    return next;
}

static inline bplus_node *get_bplus_node(bplus_tree *tree, int type, int indicator){
    bplus_node* node = bplus_get_free_cache(tree->manager, indicator);
    node->self = OFFSET_NULL;
    node->parent = OFFSET_NULL;
    node->left = OFFSET_NULL;
    node->right = OFFSET_NULL;
    node->children = 0;
    node->type = type;

    return node;
}

//获取内存node，获取文件空洞，再初始化node
static bplus_node* get_bplus_node_free_attach(bplus_tree *tree, int type){
    bplus_node* node = get_bplus_node(tree, type, BPLUS_CACHE_NULL_INDICATOR);
    off_t next = get_next_free_off(tree);
    node->self = next;

    return node;
}

//获取内存node，再映射到文件
static bplus_node* get_bplus_node_file_attach(bplus_tree *tree, off_t off, int is_node_only){
    // NON_LEAF_NODE，在这里无所谓
    bplus_node* node = get_bplus_node(tree, NON_LEAF_NODE, off);
    if(node->header.is_data_in == BPLUS_DATA_NEEDED){
        if(is_node_only == NODE_ATTACH_NODE_ONLY){
            pread(tree->fd, node, sizeof(bplus_node), off);
        }else{
            pread(tree->fd, node, BPLUS_PAGE_SIZE, off);
        }
    }

    return node;
}

//返回正数为确切命中，负数为应该插入的下标
static int find_index(bplus_tree *tree, bplus_node* node, BPLUS_KEY_TYPE *p_key){
    if(node->children == 0)
        return -1;
    int low = 0, high = node->children, i, bigger;
    BPLUS_KEY_TYPE *key_section = keys(node);
    while(low < high){
        i = (low + high) / 2;
        bigger = tree->larger_than(p_key, key_section + i);
        if(bigger == 0){
            return i;
        }else if(bigger < 0){
            high = i;
        }else{
            low = i + 1;
        }
    }
    
    return -low - 1;
}

static inline void node_write_to_file(bplus_tree *tree, bplus_node *node, off_t off){
    //printf("writing: %ld, %d, %d, %d\n", node->self, (keys(node))->county, (keys(node))->person_id), node->parent;
    pwrite(tree->fd, node, sizeof(bplus_node), off);
}

static inline void page_write_to_file(bplus_tree *tree, bplus_node *node, off_t off){
    //printf("writing: %ld, %d, %d\n", node->self, (keys(node))->county, (keys(node))->person_id);
    pwrite(tree->fd, node, BPLUS_PAGE_SIZE, off);
}

static void substitude_first_key_of_parent(bplus_tree *tree, bplus_node *node){
    if(node->parent == OFFSET_NULL)
        return;
    bplus_node* parent = get_bplus_node_file_attach(tree, node->parent, NODE_ATTACH_DATA_ALL);
    //assert(memcmp(keys(parent), keys(node) + 1, sizeof(BPLUS_KEY_TYPE)) == 0);
    *(keys(parent)) = *(keys(node));
    page_write_to_file(tree, parent, parent->self);
    substitude_first_key_of_parent(tree, parent);
    bplus_put_cache(tree->manager, parent);
}

static void simple_add_to_leaf(bplus_tree *tree, bplus_node* leaf, BPLUS_KEY_TYPE *p_key, BPLUS_DATA_TYPE *p_data, int index){
    if(index < leaf->children){
        memmove(datas(leaf, tree) + index + 1, datas(leaf, tree) + index, (leaf->children - index) * sizeof(BPLUS_DATA_TYPE));
        memmove(keys(leaf) + index + 1, keys(leaf) + index, (leaf->children - index) * sizeof(BPLUS_KEY_TYPE));
    }
    //printf("############:%d\n", index);
    //printf("#####%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
    //结构体赋值
    *(datas(leaf, tree) + index) = *p_data;
    //printf("#####%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
    *(keys(leaf) + index) = *p_key;
    //printf("%ld %ld\n", (char *)(keys(leaf) + index), (char *)(datas(leaf, tree)));
    //printf("#####%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
    leaf->children++;

    // 如果叶子结点的第一个结点插入，那肯定是最左下角的，整棵树最小的
    //printf("#####%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
    if(index == 0)
        substitude_first_key_of_parent(tree, leaf);
    //printf("#####%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
}

static void simple_add_to_non_leaf(bplus_tree *tree, bplus_node* node, BPLUS_KEY_TYPE *p_key, off_t off, int index){
    if(index < node->children){
        memmove(subs(node, tree) + index + 1, subs(node, tree) + index, (node->children - index) * sizeof(off_t));
        memmove(keys(node) + index + 1, keys(node) + index, (node->children - index) * sizeof(BPLUS_KEY_TYPE));
    }
    //结构体赋值
    *(subs(node, tree) + index) = off;
    *(keys(node) + index) = *p_key;
    node->children++;
}

static void parent_adjustment_for_new_right(bplus_tree *tree, bplus_node *left, bplus_node *right){
    bplus_node *parent;
    int index;
    // 已经是root
    if(left->parent == OFFSET_NULL){
        parent = get_bplus_node_free_attach(tree, LEAF_NODE);
        simple_add_to_non_leaf(tree, parent, keys(left), left->self, 0);
        simple_add_to_non_leaf(tree, parent, keys(right), right->self, 1);
        left->parent = parent->self;
        right->parent = parent->self;
    }else{
        parent = get_bplus_node_file_attach(tree, left->parent, NODE_ATTACH_DATA_ALL);
    }
}

// 本层对新加入的右节点做调整，需要写盘的有左节点，右节点，原右节点；
static void same_level_adjustment_for_new_right(bplus_tree *tree, bplus_node *left, bplus_node *right){
    right->parent = left->parent;
    right->left = left->self;
    right->right = left->right;
    left->right = right->self;
    if(right->right != OFFSET_NULL){
        bplus_node *ori_right = get_bplus_node_file_attach(tree, right->right, NODE_ATTACH_NODE_ONLY);
        ori_right->left = right->self;
        node_write_to_file(tree, ori_right, right->right);
        bplus_put_cache(tree->manager, ori_right);
    }
}

static void non_leaf_children_parent_adjustment(bplus_tree *tree, bplus_node *node){
    bplus_node *child;
    for(int i = 0; i < node->children; i++){
        child = get_bplus_node_file_attach(tree, *(subs(node, tree) + i), NODE_ATTACH_NODE_ONLY);
        child->parent = node->self;
        node_write_to_file(tree, child, child->self);
        bplus_put_cache(tree->manager, child);
    }
}

static void non_leaf_split(bplus_tree *tree, bplus_node *left){
    if(left->children < tree->entry_count)
        return;
    bplus_node *right = get_bplus_node_free_attach(tree, NON_LEAF_NODE), *parent;
    right->children = left->children / 2;
    left->children -= right->children;
    memmove(keys(right), keys(left) + left->children, sizeof(BPLUS_KEY_TYPE) * right->children);
    memmove(subs(right, tree), subs(left, tree) + left->children, sizeof(off_t) * right->children);
    same_level_adjustment_for_new_right(tree, left, right);
    non_leaf_children_parent_adjustment(tree, right);

    // 当前left就是根结点，写完之后发现竟然貌似和叶子结点分裂步骤一样
    if(left->parent == OFFSET_NULL){
        parent = get_bplus_node_free_attach(tree, NON_LEAF_NODE);
        parent->children = 2;
        *keys(parent) = *keys(left);
        *(keys(parent) + 1) = *keys(right);
        *subs(parent, tree) = left->self;
        *(subs(parent, tree) + 1) = right->self;
        left->parent = parent->self;
        right->parent = parent->self;
        tree->root = parent->self;
        tree->level++;

        page_write_to_file(tree, parent, parent->self);
        page_write_to_file(tree, right, right->self);
        bplus_put_cache(tree->manager, parent);
        bplus_put_cache(tree->manager, right);
    }else{
        parent = get_bplus_node_file_attach(tree, left->parent, NODE_ATTACH_DATA_ALL);
        int index = find_index(tree, parent, keys(right));
        // assert(index < 0);
        index = -index - 1;
        if(index < parent->children){
            memmove(keys(parent) + index + 1, keys(parent) + index, sizeof(BPLUS_KEY_TYPE) * (parent->children - index));
            memmove(subs(parent, tree) + index + 1, subs(parent, tree) + index, sizeof(off_t) * (parent->children - index));
        }
        parent->children++;
        *(keys(parent) + index) = *keys(right);
        *(subs(parent, tree) + index) = right->self;
        right->parent = parent->self;
        // 往上层迭代非叶子结点
        non_leaf_split(tree, parent);
        page_write_to_file(tree, parent, parent->self);
        page_write_to_file(tree, right, right->self);
        bplus_put_cache(tree->manager, parent);
        bplus_put_cache(tree->manager, right);
    }
}

static void leaf_split(bplus_tree *tree, bplus_node *left){
    if(left->children < tree->data_count)
        return;
    // 分裂出来的永远是右子节点
    bplus_node *right = get_bplus_node_free_attach(tree, LEAF_NODE), *parent;
    //printf("inininin: %d\n", tree->root);
    right->children = left->children / 2;
    left->children -= right->children;
    //printf("%d %d\n", left->children, right->children);
    //printf("%d:%d %d\n", left->children, (keys(left) + left->children)->county, (keys(left) + left->children)->person_id);
    memmove(keys(right), keys(left) + left->children, sizeof(BPLUS_KEY_TYPE) * right->children);
    memmove(datas(right, tree), datas(left, tree) + left->children, sizeof(BPLUS_DATA_TYPE) * right->children);
    //printf("%d %d\n", keys(left)->county, keys(left)->person_id);
    //printf("%s %d\n", datas(left, tree)->name, datas(left, tree)->age);
    //printf("%d %d\n", keys(right)->county, keys(right)->person_id);
    same_level_adjustment_for_new_right(tree, left, right);

    // 上层结构
    // 以前还没有非叶子结点
    if(left->parent == OFFSET_NULL){
        parent = get_bplus_node_free_attach(tree, NON_LEAF_NODE);
        parent->children = 2;
        *keys(parent) = *keys(left);
        *(keys(parent) + 1) = *keys(right);
        *subs(parent, tree) = left->self;
        *(subs(parent, tree) + 1) = right->self;
        left->parent = parent->self;
        right->parent = parent->self;
        tree->root = parent->self;
        tree->level++;
        //printf("inininin: %d, %d, %d\n", tree->root, left->self, right->self);
        //assert(parent->type == NON_LEAF_NODE);
        //assert(parent->self == 168);

        page_write_to_file(tree, parent, parent->self);
        page_write_to_file(tree, right, right->self);
        bplus_put_cache(tree->manager, parent);
        bplus_put_cache(tree->manager, right);
    }else{
        parent = get_bplus_node_file_attach(tree, left->parent, NODE_ATTACH_DATA_ALL);
        int index = find_index(tree, parent, keys(right));
        // assert(index < 0);
        index = -index - 1;
        if(index < parent->children){
            memmove(keys(parent) + index + 1, keys(parent) + index, sizeof(BPLUS_KEY_TYPE) * (parent->children - index));
            memmove(subs(parent, tree) + index + 1, subs(parent, tree) + index, sizeof(off_t) * (parent->children - index));
        }
        parent->children++;
        *(keys(parent) + index) = *keys(right);
        *(subs(parent, tree) + index) = right->self;
        right->parent = parent->self;
        // 往上层迭代非叶子结点
        // 必须在之前写防止bug, cual es muy dificil para mi a encontrarlo
        page_write_to_file(tree, right, right->self);
        non_leaf_split(tree, parent);
        page_write_to_file(tree, parent, parent->self);
        bplus_put_cache(tree->manager, parent);
        bplus_put_cache(tree->manager, right);
    }
}

static int bplus_add_to_leaf(bplus_tree *tree, bplus_node *leaf, BPLUS_KEY_TYPE *p_key, BPLUS_DATA_TYPE *p_data, const int *index_advised){
    //printf("adding: %d %d\n", p_key->county, p_key->person_id);
    int index, ret;
    if(index_advised == NULL){
        index = find_index(tree, leaf, p_key);
    }else{
        index = *index_advised;
    }
    //printf("%d\n", index);
    if(index >= 0)
        return BPLUS_ADD_ALREADY_HAS;
    //不涉及合并分裂
    if(leaf->children < tree->data_count){
        simple_add_to_leaf(tree, leaf, p_key, p_data, -index - 1);
        page_write_to_file(tree, leaf, leaf->self);
        ret = BPLUS_ADD_SUCCESS;
    }else{
        //实现预留了一个的空间
        //printf("%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
        simple_add_to_leaf(tree, leaf, p_key, p_data, -index - 1);
        //printf("%s %d\n", datas(leaf, tree)->name, datas(leaf, tree)->age);
        leaf_split(tree, leaf);
        page_write_to_file(tree, leaf, leaf->self);
        ret = BPLUS_ADD_SUCCESS;
    }

    return ret;
}

// 值、结果参数调用
static int find_key_from_root(bplus_tree *tree, BPLUS_KEY_TYPE *p_key, bplus_node **res_node, int *res_index){
    off_t off = tree->root;
    bplus_node *node = get_bplus_node_file_attach(tree, off, NODE_ATTACH_DATA_ALL);
    int index;
    while(node->type != LEAF_NODE){
        index = find_index(tree, node, p_key);
        //printf("haha: %d\n", index);
        if(index < 0)
            index = INDEX_TRANSFORM(index);
        off = *(subs(node, tree) + index);
        // 释放
        bplus_put_cache(tree->manager, node);
        node = get_bplus_node_file_attach(tree, off, NODE_ATTACH_DATA_ALL);
    }

    index = find_index(tree, node, p_key);
    *res_node = node;
    *res_index = index;

    if(index >= 0){
        return BPLUS_ADD_ALREADY_HAS;
    }else{
        return BPLUS_ADD_INDEX_FOUND;
    }
}

int bplus_add_key_value(bplus_tree *tree, BPLUS_KEY_TYPE *p_key, BPLUS_DATA_TYPE *p_data){
    bplus_node *leaf;
    int res;
    int index;

    //第一个叶子结点
    if(tree->root == OFFSET_NULL){
        leaf = get_bplus_node_free_attach(tree, LEAF_NODE);
        res = bplus_add_to_leaf(tree, leaf, p_key, p_data, NULL);
        tree->root = leaf->self;
        tree->level = 1;
        bplus_put_cache(tree->manager, leaf);
        return res;
    }

    res = find_key_from_root(tree, p_key, &leaf, &index);
    if(res == BPLUS_ADD_ALREADY_HAS){
        bplus_put_cache(tree->manager, leaf);
        return BPLUS_ADD_ALREADY_HAS;
    }
    
    bplus_add_to_leaf(tree, leaf, p_key, p_data, &index);
    bplus_put_cache(tree->manager, leaf);

    return BPLUS_ADD_SUCCESS;
}

// 替换第一个记录被改变后的父结点key
static void substitude_certain_key_of_parent(bplus_tree *tree, bplus_node *node, BPLUS_KEY_TYPE *p_ori){
    if(node->parent == OFFSET_NULL)
        return;
    bplus_node* parent = get_bplus_node_file_attach(tree, node->parent, NODE_ATTACH_DATA_ALL);
    int index = find_index(tree, parent, p_ori);
    assert(index >= 0);
    //assert(memcmp(keys(parent), keys(node) + 1, sizeof(BPLUS_KEY_TYPE)) == 0);
    *(keys(parent) + index) = *(keys(node));
    page_write_to_file(tree, parent, parent->self);
    if(index == 0)
        substitude_certain_key_of_parent(tree, parent, p_ori);
    bplus_put_cache(tree->manager, parent);
}

void simple_remove_from_leaf(bplus_tree *tree, bplus_node *leaf, int index){
    if(index < (leaf->children - 1)){
        memmove(keys(leaf) + index, keys(leaf) + index + 1, (leaf->children - index - 1) * sizeof(BPLUS_KEY_TYPE));
        memmove(datas(leaf, tree) + index, datas(leaf, tree) + index + 1, (leaf->children - index - 1) * sizeof(BPLUS_DATA_TYPE));
    }
    leaf->children--;
    page_write_to_file(tree, leaf, leaf->self);
}

void non_leaf_lend_from_siblings(bplus_tree *tree, bplus_node *node, bplus_node **sibling, int *p_side){
    bplus_node* sib;
    *sibling = NULL;
    if(node->left != OFFSET_NULL){
        sib = get_bplus_node_file_attach(tree, node->left, NODE_ATTACH_DATA_ALL);
        if(sib->children >= ((tree->entry_count / 2) + 1)){
            *sibling = sib;
            *p_side = LEFT_SIBLING; 
            return;
        }
        bplus_put_cache(tree->manager, sib);
    }

    if(node->right != OFFSET_NULL){
        sib = get_bplus_node_file_attach(tree, node->right, NODE_ATTACH_DATA_ALL);
        if(sib->children >= ((tree->entry_count / 2) + 1)){
            *sibling = sib;
            *p_side = RIGHT_SIBLING; 
            return;
        }
        bplus_put_cache(tree->manager, sib);
    }
}

// 凑点行数，之后可能要微调
void leaf_lend_from_siblings(bplus_tree *tree, bplus_node *node, bplus_node **sibling, int *p_side){
    bplus_node* sib;
    *sibling = NULL;
    if(node->left != OFFSET_NULL){
        sib = get_bplus_node_file_attach(tree, node->left, NODE_ATTACH_DATA_ALL);
        if(sib->children >= ((tree->data_count / 2) + 1)){
            *sibling = sib;
            *p_side = LEFT_SIBLING; 
            return;
        }
        bplus_put_cache(tree->manager, sib);
    }

    if(node->right != OFFSET_NULL){
        sib = get_bplus_node_file_attach(tree, node->right, NODE_ATTACH_DATA_ALL);
        if(sib->children >= ((tree->data_count / 2) + 1)){
            *sibling = sib;
            *p_side = RIGHT_SIBLING; 
            return;
        }
        bplus_put_cache(tree->manager, sib);
    }
}

void do_lend_from_sibling(bplus_tree *tree, bplus_node *node, bplus_node *sibling, int side, int index){
    BPLUS_KEY_TYPE ori_node_key, ori_sibling_key;
    BPLUS_DATA_TYPE ori_sibling_data, ori_node_data;
    off_t ori_node_off, ori_sibling_off;
    // 为了逻辑清晰，写点重复代码，以后再删
    if(node->type == LEAF_NODE){
        ori_node_key = *keys(node);
        ori_node_data = *datas(node, tree);
        if(side == LEFT_SIBLING){
            ori_sibling_key = *(keys(sibling) + sibling->children - 1);
            ori_sibling_data = *(datas(sibling, tree) + sibling->children - 1);
            sibling->children--;
            // 先插入到第一个记录
            memmove(keys(node) + 1, keys(node), node->children * sizeof(BPLUS_KEY_TYPE));
            memmove(datas(node, tree) + 1, datas(node, tree), node->children * sizeof(BPLUS_DATA_TYPE));
            *keys(node) = ori_sibling_key;
            *datas(node, tree) = ori_sibling_data;
            // 更新父结点key，顺序不用变
            substitude_certain_key_of_parent(tree, node, &ori_node_key);
            // 删除结点，当前被替换点为index+1
            if(index < (node->children - 1)){
                memmove(keys(node) + index + 1, keys(node) + index + 2, (node->children - index - 1) * sizeof(BPLUS_KEY_TYPE));
                memmove(datas(node, tree) + index + 1, datas(node, tree) + index + 2, (node->children - index - 1) * sizeof(BPLUS_DATA_TYPE));
            }
        }else{
            ori_sibling_key = *keys(sibling);
            ori_sibling_data = *datas(sibling, tree);
            memmove(keys(sibling), keys(sibling) + 1, (sibling->children - 1) * sizeof(BPLUS_KEY_TYPE));
            memmove(datas(sibling, tree), datas(sibling, tree) + 1, (sibling->children - 1) * sizeof(BPLUS_DATA_TYPE));
            sibling->children--;
            substitude_certain_key_of_parent(tree, sibling, &ori_sibling_key);

            // 添加到本结点最后一个
            *(keys(node) + node->children) = ori_sibling_key;
            *(datas(node, tree) + node->children) = ori_sibling_data;
            memmove(keys(node) + index, keys(node) + index + 1, (node->children - index) * sizeof(BPLUS_KEY_TYPE));
            memmove(datas(node, tree) + index, datas(node, tree) + index + 1, (node->children - index) * sizeof(BPLUS_DATA_TYPE));
            if(index == 0)
                substitude_certain_key_of_parent(tree, node, &ori_node_key);
        }
    }
    // 统一处理
    page_write_to_file(tree, sibling, sibling->self);
    page_write_to_file(tree, node, node->self);
    bplus_put_cache(tree->manager, sibling);
}

// 保证不会是第一个子entry，所以无须再递归调整上层
static void removed_children_parent_adjustment(bplus_tree *tree, bplus_node *parent, int index){
    if(index != (parent->children - 1)){
        memmove(keys(parent) + index, keys(parent) + index + 1, (parent->children - index - 1) * sizeof(BPLUS_KEY_TYPE));
        memmove(subs(parent, tree) + index, subs(parent, tree) + index + 1, parent->children * sizeof(BPLUS_DATA_TYPE));
    }
    parent->children--;
    page_write_to_file(tree, parent, parent->self);
}

static void same_level_adjustment_for_removed_right(bplus_tree *tree, bplus_node *left, bplus_node *right){
    bplus_node *new_right = NULL;
    left->right = right->right;
    if(right->right != OFFSET_NULL){
        new_right = get_bplus_node_file_attach(tree, right->right, NODE_ATTACH_NODE_ONLY);
        new_right->left = left->self;
        node_write_to_file(tree, new_right, new_right->self);
        bplus_put_cache(tree->manager, new_right);
    }
    page_write_to_file(tree, left, left->self);
}

static void child_adapted_by_sibling(bplus_tree *tree, bplus_node *node, bplus_node *sibling){
    bplus_node *child;
    for(int i = 0;i < node->children;i++){
        child = get_bplus_node_file_attach(tree, *(subs(node, tree) + i), NODE_ATTACH_NODE_ONLY);
        child->parent = sibling->self;
        node_write_to_file(tree, child, child->self);
        bplus_put_cache(tree->manager, child);
    }
}

void sibling_merge(bplus_tree *tree, bplus_node *node){
    if(node->type == NON_LEAF_NODE && node->children >= (tree->entry_count) / 2){
        return;
    }

    // 根结点至少有两个子女，害我在这儿想了半天
    if(node->self == tree->root){
        if(node->children == 1){
            bplus_node *child = get_bplus_node_file_attach(tree, *subs(node, tree), NODE_ATTACH_DATA_ALL); 
            tree->root = child->self;
            child->parent = OFFSET_NULL;
            node_write_to_file(tree, child, child->self);
            bplus_put_cache(tree->manager, child);
            add_free_offset(tree->free_list, node->self, FREE_OFFSET_HEAD);
        }
        return;
    }

    bplus_node *sibling, *parent, *temp;
    parent = get_bplus_node_file_attach(tree, node->parent, NODE_ATTACH_DATA_ALL);
    int index = find_index(tree, parent, keys(node));
    //printf("haha: %d\n", index);
    //printf("parent: %d %d\n", (keys(parent) + index)->county, (keys(parent) + index)->person_id);
    off_t off = node->self;
    // 保证在同一父结点合并
    // 统一为右结点并到左结点，即node合并到sibling
    if(index == 0){
        temp = get_bplus_node_file_attach(tree, node->right, NODE_ATTACH_DATA_ALL);
        sibling = node;
        node = temp;
        index = 1;
    }else{
        // 默认左结点
        sibling = get_bplus_node_file_attach(tree, node->left, NODE_ATTACH_DATA_ALL);
    }
    // node合并到sibling
    //printf("jiji: %d %d\n", keys(node)->county, keys(node)->person_id);
    if(node->type == LEAF_NODE){
        memmove(keys(sibling) + sibling->children, keys(node), node->children * sizeof(BPLUS_KEY_TYPE));
        memmove(datas(sibling, tree) + sibling->children, datas(node, tree), node->children * sizeof(BPLUS_DATA_TYPE));
    }else{
        child_adapted_by_sibling(tree, node, sibling);
        memmove(keys(sibling) + sibling->children, keys(node), node->children * sizeof(BPLUS_KEY_TYPE));
        memmove(subs(sibling, tree) + sibling->children, subs(node, tree), node->children * sizeof(off_t));
    }
    //printf("after jiji: %d %d\n", (keys(sibling) + 2)->county, (keys(sibling) + 2)->person_id);
    sibling->children += node->children;
    same_level_adjustment_for_removed_right(tree, sibling, node);
    removed_children_parent_adjustment(tree, parent, index);
    // 彻底地回收结点　
    add_free_offset(tree->free_list, off, FREE_OFFSET_HEAD);
    bplus_put_cache(tree->manager, sibling);

    // 父结点非叶子结点递归合并
    sibling_merge(tree, parent);
    bplus_put_cache(tree->manager, parent);
}

// 函数内所有的sibling都在调用函数内释放
void bplus_remove_from_leaf(bplus_tree *tree, bplus_node *leaf, int index){
    BPLUS_KEY_TYPE ori_key;
    if(leaf->parent == OFFSET_NULL){
        simple_remove_from_leaf(tree, leaf, index);
        if(leaf->children == 0)
            tree->root = OFFSET_NULL;
        return;
    }
    if(leaf->children >= ((tree->data_count / 2) + 1)){
        // 直接删除
        if(index > 0){
            // 需要平移数据
            simple_remove_from_leaf(tree, leaf, index);
        }else{
            // 还是平移数据
            // 保留第一个key
            ori_key = *(keys(leaf));
            simple_remove_from_leaf(tree, leaf, index);
            substitude_certain_key_of_parent(tree, leaf, &ori_key);
        }
    }else{
        // 需要借entry，或者合并结点
        // 能借吗？给谁借？
        bplus_node *sib;
        int side;
        leaf_lend_from_siblings(tree, leaf, &sib, &side);
        if(sib != NULL){
            // 能借！！！
            do_lend_from_sibling(tree, leaf, sib, side, index);
        }else{
            // 合并，整个项目最麻烦的代码
            // 先删除，当前b+树除了结点数其他都符合 
            ori_key = *(keys(leaf));
            simple_remove_from_leaf(tree, leaf, index);
            substitude_certain_key_of_parent(tree, leaf, &ori_key);
            //printf("haha: %d %d\n", keys(leaf)->county, keys(leaf)->person_id);
            sibling_merge(tree, leaf);
        }
    }
}

int bplus_remove_key(bplus_tree *tree, BPLUS_KEY_TYPE *p_key){
    printf("removing: %d %d\n", p_key->county, p_key->person_id);
    bplus_node *leaf;
    int index;
    if(tree->root == OFFSET_NULL)
        return BPLUS_DELETE_NOT_FOUND;

    find_key_from_root(tree, p_key, &leaf, &index);
    if(index < 0)
        return BPLUS_DELETE_NOT_FOUND;

    bplus_remove_from_leaf(tree, leaf, index);
    //printf("haha: %d %d\n", keys(leaf)->county, keys(leaf)->person_id);
    bplus_put_cache(tree->manager, leaf);

    return BPLUS_DELETE_SUCCESS;
}

void bplus_range_search(bplus_tree *tree, BPLUS_KEY_TYPE *p_key_small, BPLUS_KEY_TYPE *p_key_big, BPLUS_KEY_TYPE *key_buf, BPLUS_DATA_TYPE *data_buf, size_t *buf_size){
    bplus_node *leaf;
    int i = 0, res, index;
    off_t right;

    if(tree->root == OFFSET_NULL){
        *buf_size = 0;
        return;
    }
    res = find_key_from_root(tree, p_key_small, &leaf, &index);
    if(index < 0)
        index = -index - 1;
    int is_end = 0;
    while(1){
        while(index < leaf->children){
            if(tree->larger_than(p_key_big, keys(leaf) + index) < 0){
                is_end = 1;
                break;
            }
            *(key_buf + i) = *(keys(leaf) + index);
            *(data_buf + i) = *(datas(leaf, tree) + index);
            i++;
            if(i >= *buf_size)
                break;
            index++;
        }
        if(is_end)
            break;
        if(leaf->right != OFFSET_NULL){
            right = leaf->right;
            bplus_put_cache(tree->manager, leaf);
            leaf = get_bplus_node_file_attach(tree, right, NODE_ATTACH_DATA_ALL);
        }else{
            break;
        }
        index = 0;
    }
    *buf_size = i;
    bplus_put_cache(tree->manager, leaf);
}

void bplus_tree_traverse(bplus_tree *tree, print_key_data p_fun){
    if(tree->root == OFFSET_NULL){
        printf("tree root empty\n");
        return;
    }
    off_t all_offs[1000];
    int i, j = 0, n = 0;
    bplus_node *node = get_bplus_node_file_attach(tree, tree->root, NODE_ATTACH_DATA_ALL);
    //printf("jfoei: %d, %d, %d\n", node->children, node->self, tree->root);
    BPLUS_KEY_TYPE *p_key;
    BPLUS_DATA_TYPE *p_data;
    printf("root node:\n");
    while(node != NULL){
        if(node->type == LEAF_NODE){
            printf("a leaf node\n");
            p_key = keys(node);
            p_data = datas(node, tree);
            for(i = 0; i < node->children; i++){
                //printf("%d %d\n", p_key->county, p_key->person_id);
                p_fun(p_key + i, p_data + i);
            }
            printf("\n\n");
        }else{
            printf("a non leaf node\n");
            p_key = keys(node);
            off_t *p_off = subs(node, tree);
            for(i = 0; i < node->children; i++){
                p_fun(p_key + i, NULL);
                all_offs[n++] = *(p_off + i);
            }
            printf("\n\n");
        }
        if(j < n){
            bplus_put_cache(tree->manager, node);
            node = get_bplus_node_file_attach(tree, all_offs[j++], NODE_ATTACH_DATA_ALL);
        }else{
            node = NULL;
        }
    }
}

bplus_tree *init_bplus_tree(unsigned int hint_cache_size, const char *name, bplus_larger_than_fun p_fun){
    bplus_tree *tree = malloc(sizeof(bplus_tree));
    assert(tree != NULL);
    tree->value_size = sizeof(BPLUS_DATA_TYPE);
    tree->page_size = BPLUS_PAGE_SIZE;
    tree->root = OFFSET_NULL;
    tree->free_list = get_free_list_head();
    tree->manager = init_bplus_cache_manager(tree->page_size, hint_cache_size);
    char *res = getcwd(tree->path, MAX_PATH_LEN - 50);
    assert(res != NULL);
    strcpy(tree->path + strlen(tree->path), "/../data/");
    res = strncpy(tree->path + strlen(tree->path), name, MAX_PATH_LEN - strlen(tree->path) - 10);
    assert(res != NULL);
    strcpy(tree->path + strlen(tree->path), ".bplus");
    //printf("%s\n", tree->path);
    tree->fd = open(tree->path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    //bplus_print_error();
    assert(tree->fd >= 0);
    tree->larger_than = p_fun;
    tree->level = 0;

    // 暂不考虑对齐
    // 均-1处理，从而便于先插入再分裂，以免对于新插入点位置作过于繁琐的分类
    tree->entry_count = (tree->page_size - sizeof(bplus_node)) / (sizeof(off_t) + sizeof(BPLUS_KEY_TYPE)) - 1;
    tree->data_count = (tree->page_size - sizeof(bplus_node)) / (tree->value_size + sizeof(BPLUS_KEY_TYPE)) - 1;
    tree->file_size = 0;

    return tree;
}

void bplus_tree_info(const bplus_tree *p_tree){
    printf("tree info##############\n");
    printf("value size: %lu\n", p_tree->value_size);
    printf("page size: %lu\n", p_tree->page_size);
    printf("entry count: %d\n", p_tree->entry_count);
    printf("data count: %d\n", p_tree->data_count);
    printf("root: %lld\n", p_tree->root);
    printf("path:%s\n", p_tree->path);
    printf("fd:%d\n", p_tree->fd);
    printf("tree info##############\n\n");
}

