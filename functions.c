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

        //preenchendo as informações do arquivo no membro do diretório.
        struct membro entrada;

        //calculando o tamanho do arquivo.
        fseek(f, 0, SEEK_END);
        entrada.tam_original = ftell(f);

        strncpy(entrada.nome, nome, 100);
        entrada.uid = getuid();
        entrada.tam_disco = st.st_size;
        entrada.data_modif = st.st_mtime;
        entrada.ordem = dir.qntd_de_membros + 1;
        entrada.offset = offset_atual;

        fclose(f);

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

void option_m(const char *nome_arquivo, char *arquivo_mover) {
    FILE *archive = fopen(nome_arquivo, "r+b");
    if (!archive) {
        perror("Erro ao abrir o arquivo.\n");
        return;
    }

    printf("Procurando pelo arquivo: %s\n", arquivo_mover);

    //descobrindo o tamanho do archive.vc:
    fseek(archive, 0, SEEK_END);
    long int tam_arquivo = ftell(archive);

    //descobrindo a quantidade de membros em archive.vc:
    int qntd_arquivos;
    fseek(archive, -sizeof(int), SEEK_END);
    fread(&qntd_arquivos, sizeof(int), 1, archive);

    //descobindo aonde começa o diretório:
    //calculando o tamanho total do diretório.
    long int tam_dir = sizeof(struct membro) * qntd_arquivos + sizeof(int);
    //aonde começa o diretório no archive:
    long int inicio_dir = tam_arquivo - tam_dir;
    
    //lendo e guardando os membros do diretório:
    fseek(archive, inicio_dir, SEEK_SET);
    struct membro *membros = malloc(sizeof(struct membro) * qntd_arquivos);
    if (!membros) {
        fprintf(stderr, "Erro ao alocar memória para membros.\n");
        fclose(archive);
        return;
    }

    fread(membros, sizeof(struct membro), qntd_arquivos, archive);

    // encontrando e salvando o indice do arquivo_mover:
    int mover_idx = -1;
    for (int i = 0; i < qntd_arquivos; i++) {
        //essa função "strcmp" compara duas strings e retorna 0 quando são iguais.
        if (strcmp(membros[i].nome, arquivo_mover) == 0) {
            mover_idx = i;
            break;
        }
    }
    if (mover_idx == -1) {
        printf("Arquivo a mover não encontrado no diretório.\n");
        free(membros);
        fclose(archive);
        return;
    }

    //salvando o arquivo_mover em um buffer:
    char *buffer_mover = malloc(membros[mover_idx].tam_disco);
    if (!buffer_mover) {
        fprintf(stderr, "Erro ao alocar memória para buffer mover.\n");
        fclose(archive);
        return;
    }
    fseek(archive, membros[mover_idx].offset, SEEK_SET);
    fread(buffer_mover, 1, membros[mover_idx].tam_disco, archive);

//----------------------------------------------------------------------------------------------------------//
    //O formato para mover será o seguinte:
    // vinac <opção> <archive> membro1 membrox;
    //caso "membrox" não exista a movimentação deve ir para o início;
    //caso "membrox" exista a movimentação deve ir para logo depois de membrox
    //Atenção: a forma como estou implementando está identificando os arquivos e o target pelo nome.

    //movendo os arquivos que estão atrás para frente "membro_mover.tam_disco":
    for (int i=mover_idx-1 ; i>=0 ; i--) {
        char *temp = malloc(membros[i].tam_disco);
        if (!buffer_mover) {
            fprintf(stderr, "Erro ao alocar memória para temp.\n");
            fclose(archive);
            return;
        }
        fseek(archive, membros[i].offset, SEEK_SET);
        fread(temp, 1, membros[i].tam_disco, archive);

        membros[i].offset += membros[mover_idx].tam_disco;
        fseek(archive, membros[i].offset, SEEK_SET);
        fwrite(temp, 1, membros[i].tam_disco, archive);
        free(temp);
    }

    //escrevendo o arquivo_mover no início:
    membros[mover_idx].offset = 0;
    fseek(archive, 0, SEEK_SET);
    fwrite(buffer_mover, 1, membros[mover_idx].tam_disco, archive);
    free(buffer_mover);

    // Atualizando o novo diretório:
    //aqui eu atualizo o diretório copiado
    struct membro mover_membro = membros[mover_idx];
    for (int i = mover_idx; i > 0; i--) {
        membros[i] = membros[i - 1];
    }
    //colocando o membro movido na primeira posição do meu diretório:
    membros[0] = mover_membro;
    //apagando o diretório antigo e escrevendo o atualizado:
    ftruncate(fileno(archive), inicio_dir);
    fseek(archive, 0, SEEK_END);
    fwrite(membros, sizeof(struct membro), qntd_arquivos, archive);
    fwrite(&qntd_arquivos, sizeof(int), 1, archive);

    free(membros);
    fclose(archive);

    printf("Arquivo %s movido para o início com sucesso.\n", arquivo_mover);

}