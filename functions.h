#ifndef __FUNCTIONS__
#define __FUNCTIONS__
#include <unistd.h>
#include <time.h>
#include <sys/types.h>


struct membro {
    char nome[100];
    uid_t uid;
    long int tam_original;
    long int tam_disco;
    time_t data_modif;
    int ordem;
    long int offset;    
};

struct diretorio {
    int qntd_de_membros;
    struct membro *elemento;
};

struct informacoes_comprimido {
    long int tam_original;
    long int tam_disco;
};

void option_ip(const char *nome_arquivo, char *arquivo, int controle, struct informacoes_comprimido *x);

void option_c(const char *nome_arquivo);

void option_m(const char *nome_arquivo, char *arquivo_mover, char *arquivo_destino);

void option_r(const char *nome_arquivo, char *arquivo_remover);

void option_ic(const char *nome_arquivo, int num_arquivos, char **arquivos);

void option_x(char *nome_arquivo, char *arquivo_extrair, int controle);

#endif