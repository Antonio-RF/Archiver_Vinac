#include <stdio.h>
#include <unistd.h>
#include "functions.h"

int main(int argc, char **argv) {
    char prox_opcao;
    char *i_value = NULL;
    while ((prox_opcao = getopt(argc, argv, "i:m:x:r:c")) != -1) {
        switch(prox_opcao) {
            case 'i':
                i_value = optarg;
                if (i_value[0] == 'p') {
                    printf("opcao -ip\n");
                    printf("argv: %s\n", argv[optind]);
                    option_ip(argv[optind], argc - (optind) + 1, &argv[optind+1]);
                    break;
                }
                if (i_value[0] == 'c') {
                    printf("opcao -ic\n");
                    break;
                }
            case 'm':
                printf("opcao -m\n");
                break;
            case 'x':
                printf("opcao -x\n");
                break;
            case 'r':
                printf("opcao -r\n");
                break;
            case 'c':
                printf("opcao -c\n");
                break;
            default:
                printf("Fim, thanks!\n");
                break;
        }
    }

}