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

    struct diretorio dir;
    dir.qntd_de_membros = 0;
    dir.elemento = NULL;

    //o arquivo já existe, vamos verificar se há diretório no final:
    fseek(archive, 0, SEEK_END);
    long int tam_arquivo = ftell(archive);

    if (tam_arquivo >= sizeof(int)) {
        int qntd_antiga;
        // Lê quantidade de membros antigos
        fseek(archive, -sizeof(int), SEEK_END);
        fread(&qntd_antiga, sizeof(int), 1, archive);

        if (qntd_antiga > 0) {
            //calculando o tamanho total do diretório antigo.
            long int tam_dir_antigo = sizeof(struct membro) * qntd_antiga + sizeof(int);
            //onde começa o diretório no archive:
            long int inicio_dir = tam_arquivo - tam_dir_antigo;
            
            // Lê os membros antigos
            fseek(archive, inicio_dir, SEEK_SET);
            struct membro *anteriores = malloc(sizeof(struct membro) * qntd_antiga);
            fread(anteriores, sizeof(struct membro), qntd_antiga, archive);

            // Atualiza o diretório atual com os antigos
            dir.qntd_de_membros = qntd_antiga;
            dir.elemento = malloc(sizeof(struct membro) * qntd_antiga);
            memcpy(dir.elemento, anteriores, sizeof(struct membro) * qntd_antiga);
            free(anteriores);

            // Remove o diretório antigo
            ftruncate(fileno(archive), inicio_dir);
            fseek(archive, 0, SEEK_END);
        }
    }

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

        struct membro entrada;
        strncpy(entrada.nome, nome, 100);
        entrada.uid = getuid();
        entrada.tam_original = st.st_size;
        entrada.tam_disco = st.st_mtime;
        entrada.data_modif = st.st_mtime;
        entrada.ordem = dir.qntd_de_membros + 1;
        entrada.offset = offset_atual;

        dir.qntd_de_membros++;
        dir.elemento = realloc(dir.elemento, sizeof(struct membro) * dir.qntd_de_membros);
        dir.elemento[dir.qntd_de_membros - 1] = entrada;
    }

    //Escreve primeiramente os membros;
    fwrite(dir.elemento, sizeof(struct membro), dir.qntd_de_membros, archive);
    //Depois a qntd de membros;
    fwrite(&dir.qntd_de_membros, sizeof(int), 1, archive);

    fclose(archive);
    printf("Arquivos inseridos com sucesso em %s\n", nome_arquivo);
}

void option_c(const char *nome_arquivo) {
    FILE *archive = fopen(nome_arquivo, "r+b");
    if (!archive) {
        perror("Erro ao abrir o arquivo.\n");
        return;
    }

    //verificando se há algo no archive:
    fseek(archive, 0, SEEK_END);
    long int tam_arquivo = ftell(archive);
    if (tam_arquivo < sizeof(int)) {
        printf("Sem arquivos no archive.vc!\n");
        return;
    }

    //descobrindo quantos arquivos há em archive.vc:
    int qntd_arquivos;
    fseek(archive, -sizeof(int), SEEK_END);
    fread(&qntd_arquivos, sizeof(int), 1, archive);

    //calculando o tamanho total do diretório antigo.
    long int tam_dir = sizeof(struct membro) * qntd_arquivos + sizeof(int);
    //onde começa o diretório no archive:
    long int inicio_dir = tam_arquivo - tam_dir;
    fseek(archive, inicio_dir, SEEK_SET);

    // aloca memória para ler o os membros do diretório:
    struct membro *membros = malloc(sizeof(struct membro) * qntd_arquivos);
    if (!membros) {
        perror("Erro ao alocar memória para membros\n");
        fclose(archive);
        return;
    }

    // lê os membros do diretório e coloca em "membros":
    fread(membros, sizeof(struct membro), qntd_arquivos, archive);

    if (qntd_arquivos > 0) {
        for (int i=0 ; i<qntd_arquivos ; i++) {
            printf("nome:%s, uid:%d, tamanho_original: %ld, tamanho_em_disco: %ld, data_modificação: %s",
            membros[i].nome,
            membros[i].uid,
            membros[i].tam_original,
            membros[i].tam_disco,
            ctime(&membros[i].data_modif));
        }
    }

    free(membros);
    fclose(archive);
}