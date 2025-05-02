#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "functions.h"

int main(int argc, char **argv) {
    char prox_opcao;
    char *i_value = NULL;
    while ((prox_opcao = getopt(argc, argv, "i:m:x:r:c")) != -1) {
        for (int i = 0; i < argc; i++) {
            printf("argv[%d] = %s\n", i, argv[i]);
        }
        switch(prox_opcao) {
            case 'i':
                i_value = optarg;
                if (i_value[0] == 'p') {
                    printf("opcao -ip\n")   ;
                    // nome_arquivo: archive.vc ; num_arquivos: tira "./vinac" "-ip" e "archive.vc" ;
                    // arquivos: ponteiro de ponteiros a partir.
                    option_ip(argv[optind], argc - 3, &argv[optind+1]);
                    break;
                }
                if (i_value[0] == 'c') {
                    printf("opcao -ic\n");
                    break;
                }
                break;
            case 'm':
                printf("opcao -m\n");
                if (optind >= argc) {
                    fprintf(stderr, "Erro: esperado nome do arquivo a mover após o nome do arquivo.\n");
                    break;
                }
                if ((optind + 1) > argc) {
                    fprintf(stderr, "Erro: argumento insuficiente após -m.\n");
                    break;
                }
                printf("arquivo pretendido: %s\n", optarg);
                printf("arquivo a mover: %s\n", argv[optind]);
                option_m(optarg, argv[optind]);
                break;
            case 'x':
                printf("opcao -x\n");
                break;
            case 'r':
                printf("opcao -r\n");
                break;
            case 'c':
                printf("opcao -c\n");
                if (optind < argc) {
                    option_c(argv[optind]);
                } else {
                    fprintf(stderr, "Erro: esperado nome do arquivo após -c\n");
                }
                break;
            default:
                printf("Fim, thanks!\n");
                break;
        }
    }

}