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
                    printf("opcao -ip\n");
                    for (int i= optind ; i < argc-1 ; i++) {
                        option_ip(argv[optind], argv[i+1], 0, NULL);
                    }
                    break;
                }
                if (i_value[0] == 'c') {
                    printf("opcao -ic\n");
                    option_ic(argv[optind], argc - 3, &argv[optind+1]);
                    break;
                }
                break;
            case 'm':
                printf("opcao -m\n");
                printf("arquivo pretendido: %s\n", optarg);
                printf("arquivo a mover: %s\n", argv[optind]);
                option_m(optarg, argv[optind], argv[optind+1]);
                break;
            case 'x':
                printf("opcao -x\n");
                //argc == 3 quer dizer que não indicou o arquivo, então extrai todos.
                if (argc == 3)
                    option_x(optarg, "NULL", 0);
                else {
                    for (int i= optind ; i < argc ; i++) {
                        option_x(optarg, argv[i], 1);
                    }
                }
                break;
            case 'r':
                printf("opcao -r\n");
                for (int i= optind ; i < argc ; i++)
                    option_r(optarg, argv[i]);
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