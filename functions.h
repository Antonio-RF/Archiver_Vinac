#ifndef __FUNCTIONS__
#define __FUNCTIONS__
#include <unistd.h>
#include <time.h>

struct diretorio {
    char nome[100];
    uid_t uid;
    long int tam_original;
    long int tam_disco;
    time_t data_modif;
    int ordem;
    long int offset;    
};

void option_ip(const char *nome_arquivo, int num_arquivos, char **arquivos);


#endif