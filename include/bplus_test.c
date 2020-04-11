#include "bplus.h"

int id_card_bigger(id_card *a, id_card *b){
    //printf("%d %d\n", a->county, b->county);
    //printf("%d %d\n", a->person_id, b->person_id);
    if(a->county > b->county){
        return 1;
    }else if(a->county < b->county){
        return -1;
    }else{
        if(a->person_id > b->person_id){
            return 1;
        }else if(a->person_id < b->person_id){
            return -1;
        }
    }
    return 0;
}

void my_print(id_card *ic, person_info *pi){
    if(pi != NULL){
        printf("key: %d | %d, data: %s | %d ### ", ic->county, ic->person_id, pi->name, pi->age);
    }else{
        printf("key: %d | %d ### ", ic->county, ic->person_id);
    }
}

void add_multiple(bplus_tree *tree, int county, int count){
    int i;
    id_card ic;
    ic.county = county;
    person_info pi = {"trival", 39};
    for(i = 0; i < count; i++){
        //printf("inininin\n");
        ic.person_id = i;
        bplus_add_key_value(tree, &ic, &pi);
    }
}

// 1024 * 0.2 页大小作为测试
int main(int argc, char **argv){
    bplus_tree *tree = init_bplus_tree(10, "liutianyi", id_card_bigger);
    bplus_tree_info(tree);
    id_card ic;
    person_info pi;
    int res;

    // 1
    ic.county = 1;
    ic.person_id = 1;
    strcpy(pi.name, "primera");
    pi.age = 10;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 重复
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 顺序插入 2
    ic.county = 1;
    ic.person_id = 2;
    strcpy(pi.name, "secunda");
    pi.age = 11;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 删除 2
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);

    // 继续删除
    ic.person_id = 1;
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);

    // 重新插入两个
    ic.county = 1;
    ic.person_id = 1;
    strcpy(pi.name, "primera");
    pi.age = 10;
    bplus_add_key_value(tree, &ic, &pi);
    ic.county = 1;
    ic.person_id = 2;
    strcpy(pi.name, "secunda");
    pi.age = 11;
    bplus_add_key_value(tree, &ic, &pi);
    // 删除第一个
    ic.person_id = 1;
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    //第一个插回去
    ic.county = 1;
    ic.person_id = 1;
    strcpy(pi.name, "primera");
    pi.age = 10;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 乱序插入 3
    ic.county = 0;
    ic.person_id = 0;
    strcpy(pi.name, "dios");
    pi.age = 100;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 乱序插入4
    ic.county = 0;
    ic.person_id = 1;
    strcpy(pi.name, "monstro");
    pi.age = 99;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 正序插入5，叶子结点分裂
    ic.county = 2;
    ic.person_id = 1;
    strcpy(pi.name, "dos");
    pi.age = 60;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // trival ones，再次导致结点分裂
    add_multiple(tree, 3, 3);
    //bplus_tree_traverse(tree, my_print);

    // 中间再次重复插入，失败
    ic.county = 1;
    ic.person_id = 2;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 插入，成功
    ic.person_id = 3;
    strcpy(pi.name, "dentro");
    pi.age = 18;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 删除，从记录数够的结点
    ic.county = 1; 
    ic.person_id = 3; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    // 加回去
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);
    // 删除第一个记录
    ic.county = 1; 
    ic.person_id = 2; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    // 加回去
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);
    // 继续删，给左兄弟借
    ic.county = 3; 
    ic.person_id = 1; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    // 先加继续删，给右兄弟借
    add_multiple(tree, 40, 4);
    //bplus_tree_traverse(tree, my_print);
    ic.county = 40; 
    ic.person_id = 0; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    ic.county = 3; 
    ic.person_id = 2; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);

    // 再次插入，结点分裂
    ic.person_id = 4;
    strcpy(pi.name, "dentro dos");
    pi.age = 19;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 最左侧插入
    ic.county = -1;
    ic.person_id = 0;
    strcpy(pi.name, "invisible");
    pi.age = -1;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);

    // 插入直到插满根结点
    add_multiple(tree, 4, 12);
    //bplus_tree_traverse(tree, my_print);

    // 最左侧再次插入
    ic.county = -2;
    ic.person_id = 0;
    strcpy(pi.name, "invisible -2");
    pi.age = -2;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);
    ic.county = -2;
    ic.person_id = 1;
    bplus_add_key_value(tree, &ic, &pi);
    ic.county = -2;
    ic.person_id = 2;
    bplus_add_key_value(tree, &ic, &pi);
    //bplus_tree_traverse(tree, my_print);
    // 删除检验
    ic.county = 4; 
    ic.person_id = 10; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    ic.county = 4; 
    ic.person_id = 11; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    ic.county = 4; 
    ic.person_id = 9; 
    bplus_remove_key(tree, &ic);
    //bplus_tree_traverse(tree, my_print);
    ic.county = 40; 
    ic.person_id = 2; 
    bplus_remove_key(tree, &ic);
    bplus_tree_traverse(tree, my_print);
    ic.county = 40; 
    ic.person_id = 1; 
    bplus_remove_key(tree, &ic);
    bplus_tree_traverse(tree, my_print);
    exit(0);

    // 遍历
    id_card ic_buf[200];
    person_info pi_buf[200];
    size_t buf_size = 200;
    id_card small, big;
    small.county = -1;
    small.person_id = 0;
    big.county = 3;
    big.person_id = 1;
    bplus_range_search(tree, &small, &big, ic_buf, pi_buf, &buf_size);
    printf("buf_size: %zu\n", buf_size);
    for(int i = 0;i < buf_size;i++){
        printf("%d %d\n", (ic_buf + i)->county, (ic_buf + i)->person_id);
        printf("%s %d\n", (pi_buf + i)->name, (pi_buf + i)->age);
    }
}

