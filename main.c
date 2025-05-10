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
                    for (int i= optind ; i < argc-1 ; i++) {
                        option_ip(argv[optind], argv[i+1], 0, NULL);
                    }
                    printf("Arquivos inseridos com sucesso em %s\n", argv[optind]);
                    break;
                }
                if (i_value[0] == 'c') {
                    option_ic(argv[optind], argc - 3, &argv[optind+1]);
                    break;
                }
                break;
            case 'm':
                option_m(optarg, argv[optind], argv[optind+1]);
                break;
            case 'x':
                if (argc == 3)
                    option_x(optarg, "NULL", 0);
                else {
                    for (int i= optind ; i < argc ; i++) {
                        option_x(optarg, argv[i], 1);
                    }
                }
                break;
            case 'r':
                for (int i= optind ; i < argc ; i++)
                    option_r(optarg, argv[i]);
                break;
            case 'c':
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