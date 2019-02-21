#include "bootpack.h"

int strcmp(const char *s1, const char *s2){
    for(int i=0;;i++){
        char s, t;
        if(s1[i] == '\0') s = 0;
        else s = s1[i];
        if(s2[i] == '\0') t = 0;
        else t = s2[i];
        if(s > t) return 1;
        if(s < t) return -1;
        if(s == 0 && t == 0) return 0;
    }
}