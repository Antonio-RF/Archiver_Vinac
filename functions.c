#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include "functions.h"

void option_ip(const char *nome_arquivo, int num_arquivos, char **arquivos) {
    printf("Arquivo archive: %s\n", nome_arquivo);
    printf("Quantidade de arquivos a adicionar: %d\n", num_arquivos);
    for (int i = 0; i < num_arquivos; i++) {
        printf("-> Arquivo[%d] = %s\n", i, arquivos[i]);
    }
    
    //"nome_arquivo" vai ser aonde você quer adicionar.
    //no caso, como a entrada vai ser ./vinac -ip archive.vc texto1.txt texto2.txt ...
    // o "nome_arquivo" será "archive.vc"
    FILE *archive = fopen(nome_arquivo, "r+b");

    //se não existir ainda, cria.
    if (!archive) {
        archive = fopen(nome_arquivo, "w+b");
        if (!archive) {
            perror("Erro ao abrir archive.vc\n");
            exit(1);
        }
    }

    // Pula para o fim do arquivo para pegar o tamanho atual (para calcular offset).
    fseek(archive, 0, SEEK_END);
    long int offset_inicio = ftell(archive);

    //suporte de até 100 arquivos.
    struct diretorio *dir = malloc(sizeof(struct diretorio) * 100);
    int qntd_membros = 0;

    for (int i=0 ; i< num_arquivos ; i++) {
        const char *nome = arquivos[i];
        FILE *f = fopen(nome, "rb");
        if (!f) {
            perror("Erro ao abrir membro");
            continue;
        }

        //informações do arquivo.
        struct stat st;
        stat(nome, &st);

        //adiciona ao final de archive.
        char buffer[1024];
        size_t lidos;
        long int offset_atual = ftell(archive);

        while ((lidos = fread(buffer, 1, 1024, f)) > 0)
            fwrite(buffer, 1, lidos, archive);

        fclose(f);

        struct diretorio entrada;
        strncpy(entrada.nome, nome, 100);
        entrada.uid = getuid();
        entrada.tam_original = st.st_size;
        entrada.tam_disco = st.st_mtime;
        entrada.data_modif = st.st_mtime;
        entrada.ordem = qntd_membros + 1;
        entrada.offset = offset_atual;

        dir[qntd_membros++] = entrada;
    }

    //Escreve o diretório no final do arquivo
    fwrite(dir, sizeof(struct diretorio), qntd_membros, archive);
    fwrite(&qntd_membros, sizeof(int), 1, archive);

    fclose(archive);
    printf("Arquivos inseridos com sucesso em %s\n", nome_arquivo);

}