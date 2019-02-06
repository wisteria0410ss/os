#include <stdio.h>

int main(){
    FILE *fp = fopen("hankaku.txt", "r");
    if(fp==NULL){
        fprintf(stderr, "load error.\n");
        return 1;
    }

    printf("char hankaku[4096] = {");
    char c[256];
    while(fgets(c, 256, fp)!=NULL){
        if(c[0]=='c'){
            printf("\n");
            continue;
        }

        unsigned char d=0;
        if(c[0]=='.' || c[0]=='*'){
            for(int i=0;i<8;i++){
                d += (c[i]=='*')<<(7-i);
            }
            printf("\t%d,", d);
        }
    }
    printf("\n};\n");
    return 0;
}