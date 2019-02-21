#include "bootpack.h"

int strcmp(const char *s1, const char *s2){
    for(int i=0;;i++){
        if(s1[i] > s2[i]) return 1;
        if(s1[i] < s2[i]) return -1;
        if(s1[i] == 0 && s2[i] == 0) return 0;
    }
}

int starts_with(const char *s, const char *prefix){
    for(int i=0;prefix[i]!=0;i++){
        if(s[i] != prefix[i]) return 0;
    }
    return 1;
}