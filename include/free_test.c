#include<stdio.h>
int find_index(long *p_long, int len, int n){
    if(len == 0)
        return -1;
    int low = 0, high = len, i, bigger;
    while(low < high){
        i = (low + high) / 2;
        bigger = n - p_long[i]; 
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


int main(int argc, char **argv){
    long longs[] = {1, 4, 7, 9};
    int i = -1;
    printf("1 4 7 9\n");
    while(i < 11){
        printf("%d %d\n", i, find_index(longs, sizeof(longs)/sizeof(long), i));
        i++;
    }
}
