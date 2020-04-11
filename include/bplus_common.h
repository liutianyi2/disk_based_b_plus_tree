#ifndef BPLUS_COMMON_H
#define BPLUS_COMMON_H
#define OFFSET_NULL 0xdeadbeef
#define BPLUS_ROUND_UP(ori, r) ((ori & (r - 1)) ? ((ori & ~(r - 1)) + r): ori)
#include<stdio.h>
#include<errno.h>
#include<string.h>
static inline void bplus_print_error(){
    printf("%s\n", strerror(errno));
}

typedef struct id_card{
    int county;
    int person_id;
}bplus_key_type, id_card;

typedef struct person_info{
    char name[15];
    int age;
}bplus_data_type, person_info;

#endif
