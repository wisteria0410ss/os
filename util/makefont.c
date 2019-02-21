#include <stdio.h>

int main(int argc, char **argv){
    FILE *fp = fopen(argv[1], "r");
    if(fp==NULL){
        fprintf(stderr, "load error.\n");
        return 1;
    }

    printf("char *get_fontdata(){\n\tstatic char hankaku[4096] = {");
    char c[256];
    while(fgets(c, 256, fp)!=NULL){
        if(c[0]=='c'){
            printf("\n\t");
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
    printf("\n\t};\n\treturn hankaku;\n}\n");
    return 0;
}