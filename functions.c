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

    // Aqui ele move o "cursor" até o final do arquivo.
    fseek(archive, 0, SEEK_END);
    // Já aqui ele salva nessa variável aonde está o cursor, ou seja, quantos bytes tem o arquivo "archiver.vc"
    long int offset_inicio = ftell(archive);

    //suporte de até 100 arquivos.
    struct diretorio *dir = malloc(sizeof(struct diretorio) * 100);
    int qntd_membros = 0;

    for (int i=0 ; i< num_arquivos ; i++) {
        //Armazena na variável nome o nome do arquivo. Ex: nome = teste.txt
        const char *nome = arquivos[i];
        FILE *f = fopen(nome, "rb");
        if (!f) {
            perror("Erro ao abrir membro");
            continue;
        }

        //preenche a estrutura "st" com os dados do arquivo da variável "nome".
        struct stat st;
        stat(nome, &st);

        //adiciona ao final de archive.
        char buffer[1024];
        size_t lidos;
        //Aqui ele marca em "offset_atual" a posição do cursor, ou seja, aonde o arquivo vai começar a ser inserido.
        long int offset_atual = ftell(archive);

        //ele lê até 1024 bytes e colocar dentro do buffer, e retorna quantos bytes foram lidos.
        //esse loop acaba somente quando todos são lidos.
        while ((lidos = fread(buffer, 1, 1024, f)) > 0)
            //escreve o que acabou de ser lido do buffer para o "archive".
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

    //Escreve o diretório no final do arquivo:
    fwrite(dir, sizeof(struct diretorio), qntd_membros, archive);
    //Escreve a quantidade de membros no archive.vc, ou seja, para saber quantos tem, você deve:
        //fseek(f, -sizeof(int), SEEK_END);  (volta 4 bytes a partir do fim).
        //int total_arquivos;
        //fread(&total_arquivos, sizeof(int), 1, f); (armazena na variável "total_arquivos" a qntd).
    fwrite(&qntd_membros, sizeof(int), 1, archive);

    fclose(archive);
    printf("Arquivos inseridos com sucesso em %s\n", nome_arquivo);
}