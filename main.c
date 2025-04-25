#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "functions.h"

int main(int argc, char **argv) {
    char prox_opcao;
    char *i_value = NULL;
    while ((prox_opcao = getopt(argc, argv, "i:m:x:r:c")) != -1) {
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