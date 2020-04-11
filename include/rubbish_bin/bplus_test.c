typedef struct my_key{
    int county;
    int person_id;
}id_card;
typedef struct my_data{
    char name[10];
    int age;
}person_info;
#define BPLUS_KEY_TYPE id_card 
#define BPLUS_DATA_TYPE person_info
#include "bplus.h"


int main(int argc, char **argv){
    bplus_tree *tree = init_bplus_tree(10, "liutianyi");
    bplus_tree_info(tree);
}

