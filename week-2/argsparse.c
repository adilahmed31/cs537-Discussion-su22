#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char* fname = NULL;
char* word = NULL;

void parse_args(int argc, char* argv[]){
        int opt;
        while ((opt = getopt(argc, argv, "Vhf:")) != -1){
                switch(opt){
                        case 'V':
                                printf("V argument provided!\n");
                                exit(0);
                        case 'h':
                                printf("h argument provided!\n");
                                exit(0);
                        case 'f':
                                fname = optarg;
                                printf("f argument provided with value %s\n", fname);
                                break;
                        default:
                                printf("Invalid arguments!\n");
                                exit(1);
                }
        }
        if (optind != argc - 1){
                printf("Invalid arguments!\n");
                exit(1);
        }
        else{
                printf("Filename is %s and word is %s\n", fname, argv[optind]);
        }
}

int main(int argc, char* argv[]){
        parse_args(argc, argv);
}