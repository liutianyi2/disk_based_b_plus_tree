#include "bplus_list.h"


int main(int argc, char **argv){
    bplus_free_list_head *free_nodes = get_free_list_head();
    int i = 1;
    while(i <= 10){
        add_free_offset(free_nodes, i, FREE_OFFSET_HEAD);
        i++;
    }
    printf("test one begins, add 10\n");
    free_list_traverse(free_nodes);
    printf("\ntest one done\n");

    printf("test two, using 5 heads\n");
    i = 5;
    while(i-- > 0){
        printf("%lld ", use_first_free_offset(free_nodes));
    }
    printf("\nremaining:");
    free_list_traverse(free_nodes);
    printf("test two done\n");
    printf("test three add 100, 101 to tail\n");
    add_free_offset(free_nodes, 100, FREE_OFFSET_TAIL);
    add_free_offset(free_nodes, 101, FREE_OFFSET_TAIL);
    free_list_traverse(free_nodes);
    printf("test three done\n");
}

