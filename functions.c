#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    
#include <unistd.h>  
#include <sys/stat.h>
#include "functions.h"
#include "lz.h"
#include "secundary.h"

//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO INSERIR NÃO COMPRIMIDO:
void option_ip(const char *nome_arquivo, char *arquivo, int controle, struct informacoes_comprimido *x) {
    printf("Arquivo archive: %s\n", nome_arquivo);
    printf("Arquivo a ser inserido: %s\n", arquivo);

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

    //conferindo se há algo no archive.vc:
    if ((unsigned long)tam_arquivo >= sizeof(int)) {
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

    //Armazena na variável nome o nome do arquivo. Ex: nome = teste.txt
    char *nome = arquivo;
    FILE *f = fopen(nome, "rb");
    if (!f) {
        perror("Erro ao abrir membro");
        return;
    }

    //preenche a estrutura "st" com os dados do arquivo da variável "nome".
    struct stat st;
    stat(nome, &st);
//--------------------------------------------------------------------
    //conferindo se o arquivo já existe no meu diretório:
    int guarda_i = confere_existe(dir, nome);

    //guardando o offset de aonde eu vou adicionar meu diretório ao final
    //a depender dos casos:
    //-1 = vai para o final (caso em que o arquivo a inserir é maior ou inserção normal).
    //0 = vai para o final do último arquivo do archive.
    // Primeiro, move os dados reais no arquivo (trás para frente)
    int offset_final_diretorio = -1;

    //se existir, vou pegar o tamanho do arquivo que já está no diretório e o tamanho desse que vou inserir.
    //tirar essa diferença e, se for diferente de 0, colocar todos que estão à frente "diferença" para frente ou para trás.
    if (guarda_i != -1) {
        fseek(f, 0, SEEK_END);
        int tam_arq_inserir = ftell(f);
        int tam_arq_antigo = dir.elemento[guarda_i].tam_disco;
        int diferenca_tam = tam_arq_inserir - tam_arq_antigo;
        printf("diferença de tam: %d\n", diferenca_tam);

        if (diferenca_tam != 0) {
            if (diferenca_tam > 0) {
                for (int j = dir.qntd_de_membros - 1; j > guarda_i; j--) {
                    struct membro *m = &dir.elemento[j];

                    // Posição atual dos dados (antes do deslocamento)
                    long int pos_atual = m->offset;
                    long int novo_offset = pos_atual + diferenca_tam;

                    // Aloca buffer e lê os dados na posição atual
                    char *buffer = malloc(m->tam_disco);
                    fseek(archive, pos_atual, SEEK_SET);

                    // Escreve na nova posição (após o deslocamento)
                    fseek(archive, novo_offset, SEEK_SET);

                    free(buffer);

                    // Atualiza o offset na estrutura
                    m->offset = novo_offset;
                }
            }
            else {
                //só para tratar o caso de ser o último membro:
                struct membro *k = &dir.elemento[guarda_i];
                offset_final_diretorio = k->offset+k->tam_disco+diferenca_tam;
                printf("offset_final_diretório: %d\n", offset_final_diretorio);
                for (int j = guarda_i+1; j < dir.qntd_de_membros; j++) {
                    struct membro *m = &dir.elemento[j];

                    // Posição atual dos dados (antes do deslocamento)
                    long int pos_atual = m->offset;
                    long int novo_offset = pos_atual + diferenca_tam;

                    // Aloca buffer e lê os dados na posição atual
                    char *buffer = malloc(m->tam_disco);
                    fseek(archive, pos_atual, SEEK_SET);
                    fread(buffer, 1, m->tam_disco, archive);

                    // Escreve na nova posição (após o deslocamento)
                    fseek(archive, novo_offset, SEEK_SET);
                    fwrite(buffer, 1, m->tam_disco, archive);

                    free(buffer);

                    // Atualiza o offset na estrutura
                    m->offset = novo_offset;
                    offset_final_diretorio = m->offset+m->tam_disco;
                }
            }

            //escrevendo o conteúdo do arquivo novo no offset do antigo:
            fseek(f, 0, SEEK_SET);
            fseek(archive, dir.elemento[guarda_i].offset, SEEK_SET);
            char buffer[1024];
            size_t lidos;
            while ((lidos = fread(buffer, 1, sizeof(buffer), f)) > 0)
                fwrite(buffer, 1, lidos, archive);

            dir.elemento[guarda_i].tam_disco = tam_arq_inserir;

            fclose(f);
        }
        if (diferenca_tam == 0) {
            //escrevendo o conteúdo do arquivo novo no offset do antigo:
            fseek(f, 0, SEEK_SET);
            fseek(archive, dir.elemento[guarda_i].offset, SEEK_SET);
            char buffer[1024];
            size_t lidos;
            while ((lidos = fread(buffer, 1, sizeof(buffer), f)) > 0)
                fwrite(buffer, 1, lidos, archive);

            fclose(f);
        }
    }
//--------------------------------------------------------------------
    //aqui é se não tem membro igual (caso normal):
    else {
        //adiciona ao archive.
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
        // Só preenche os tamanhos se controle for 0 (não comprimido)
        if (controle == 0) {
            entrada.tam_original = ftell(f);   // Tamanho original do arquivo
            entrada.tam_disco = st.st_size;    // Tamanho no disco do arquivo
        }
        else {
            entrada.tam_original = x->tam_original;
            entrada.tam_disco = x->tam_disco;  
        }


        strncpy(entrada.nome, nome, 100);
        entrada.nome[99] = '\0';
        entrada.uid = getuid();
        entrada.data_modif = st.st_mtime;
        entrada.ordem = dir.qntd_de_membros + 1;
        entrada.offset = offset_atual;

        dir.qntd_de_membros++;
        dir.elemento = realloc(dir.elemento, sizeof(struct membro) * dir.qntd_de_membros);

        dir.elemento[dir.qntd_de_membros - 1] = entrada;
        fclose(f);
    }

    //importantíssimo:
    if (offset_final_diretorio == -1)
        fseek(archive, 0, SEEK_END);
    else 
        fseek(archive, offset_final_diretorio, SEEK_SET);

    //Escreve primeiramente os membros;
    fwrite(dir.elemento, sizeof(struct membro), dir.qntd_de_membros, archive);
    //Depois a qntd de membros;
    fwrite(&dir.qntd_de_membros, sizeof(int), 1, archive);

    // apagando tudo que há depois do meu diretório (se há alguma coisa)
    long pos = ftell(archive);
    if (pos == -1L) {
        perror("ftell");
        // Trate o erro, por exemplo, com exit ou return
    } else {
        // Trunca o arquivo exatamente onde o cursor está agora
        if (ftruncate(fileno(archive), pos) == -1) {
            perror("ftruncate");
            // Trate o erro se necessário
        }
    }

    fclose(archive);
    printf("Arquivos inseridos com sucesso em %s\n", nome_arquivo);
}

//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO INSERIR COMPRIMIDO:
void option_ic(const char *nome_arquivo, int num_arquivos, char **arquivos) {
    //criando um vetor para comprimir os arquivos:
    char **arquivos_a_comprimir = malloc(num_arquivos * sizeof(char *));
    struct informacoes_comprimido *info = malloc(num_arquivos *sizeof(struct informacoes_comprimido));
    if (!arquivos_a_comprimir || !info) {
        perror("Erro ao alocar memória");
        free(arquivos_a_comprimir);
        free(info);
        return;
    }

    for (int i = 0; i < num_arquivos; i++) {
        char *nome = arquivos[i];
        FILE *f = fopen(nome, "rb");
        if (!f) {
            perror("Erro ao abrir membro");
            arquivos_a_comprimir[i] = NULL;
            continue;
        }

        // Descobrindo tamanho do arquivo
        fseek(f, 0, SEEK_END);
        long tam = ftell(f);
        rewind(f);

        // Aloca memória e lê todo o conteúdo do arquivo
        char *conteudo = malloc(tam);
        if (!conteudo) {
            perror("Erro ao alocar memória para leitura");
            fclose(f);
            arquivos_a_comprimir[i] = NULL;
            continue;
        }

        fread(conteudo, 1, tam, f);
        fclose(f);

        // Comprime o conteúdo
        unsigned char *comprimido = malloc(tam);
        // Realiza compressão
        int tam_comprimido = LZ_Compress((unsigned char *)conteudo, comprimido, (unsigned int)tam);
        free(conteudo);
        
        if (tam_comprimido <= 0 || !comprimido) {
            printf("Erro ao comprimir %s\n", arquivos[i]);
            free(comprimido);
            arquivos_a_comprimir[i] = NULL;
            continue;
        }

        //colocando as informações do tamanho original e tamanho em disco.
        struct stat st;
        stat(nome, &st);
        info[i].tam_original = st.st_size;
        info[i].tam_disco = tam_comprimido;

        //aqui eu confiro se o arquivo comprimido é maior que o original ou não.
        //se for maior, eu insiro o arquivo original, senão eu insiro o arquivo comprimido.
        if (tam_comprimido < tam) {
            FILE *out1 = fopen(nome, "wb");
            if (!out1) {
                perror("Erro ao sobrescrever o arquivo original com o conteúdo comprimido");
                free(comprimido);
                arquivos_a_comprimir[i] = NULL;
                continue;
            }

            fwrite(comprimido, 1, tam_comprimido, out1);
            fclose(out1);
            free(comprimido);

            arquivos_a_comprimir[i] = strdup(nome);
        }
        else {
            info[i].tam_disco = info[i].tam_original;
            free(comprimido);
            arquivos_a_comprimir[i] = strdup(nome);
        }
    }

    // Remove os arquivos temporários
    for (int i = 0; i < num_arquivos; i++) {
        if (arquivos_a_comprimir[i]) {
            option_ip(nome_arquivo, arquivos_a_comprimir[i], 1, &info[i]);
            free(arquivos_a_comprimir[i]);
        }
    }

    free(arquivos_a_comprimir);
    free(info);
}
//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO LISTAR:
void option_c(const char *nome_arquivo) {
    FILE *archive = fopen(nome_arquivo, "r+b");
    if (!archive) {
        perror("Erro ao abrir o arquivo.\n");
        return;
    }

    //verificando se há algo no archive:
    fseek(archive, 0, SEEK_END);
    long int tam_arquivo = ftell(archive);
    if (tam_arquivo < (long)sizeof(int)) {
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

    // aloca memória para ler os membros do diretório:
    struct membro *membros = malloc(sizeof(struct membro) * qntd_arquivos);
    if (!membros) {
        perror("Erro ao alocar memória para membros\n");
        printf("%d", qntd_arquivos);
        fclose(archive);
        return;
    }

    // lê os membros do diretório e coloca em "membros":
    fread(membros, sizeof(struct membro), qntd_arquivos, archive);

    if (qntd_arquivos > 0) {
        printf("ARQUIVOS DENTRO DO ARCHIVE.VC:\n");
        printf("--------------------------------------------------------------------------------------------------------------\n");
        printf("| %-20s | %-5s | %-17s | %-17s | %-25s | %-6s |\n", 
            "Nome", "UID", "Tamanho_Original", "Tamanho_em_Disco", "Data_Modificação", "Offset");
        printf("--------------------------------------------------------------------------------------------------------------\n");

        for (int i = 0; i < qntd_arquivos; i++) {
            char data_formatada[26];  // ctime sempre retorna 26 caracteres incluindo '\n'
            strncpy(data_formatada, ctime(&membros[i].data_modif), 25);
            data_formatada[24] = '\0'; // remove o '\n' ao final

            printf("| %-20s | %-5d | %-17ld | %-17ld | %-25s | %-6ld |\n",
                membros[i].nome,
                membros[i].uid,
                membros[i].tam_original,
                membros[i].tam_disco,
                data_formatada,
                membros[i].offset);
        }

        printf("--------------------------------------------------------------------------------------------------------------\n");

    }

    free(membros);
    fclose(archive);
}

//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO MOVER:
void option_m(const char *nome_arquivo, char *arquivo_mover, char *arquivo_destino) {
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
    printf("qntd de membros: %d\n", qntd_arquivos);
    

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


    //O formato para mover será o seguinte:
    // vinac <opção> <archive> membro1 membrox;
    //caso "membrox" não exista a movimentação deve ir para o início;
    //caso "membrox" exista a movimentação deve ir para logo depois de membrox
    //Atenção: a forma como estou implementando está identificando os arquivos e o target pelo nome.

    // encontrando e salvando o indice do arquivo_destino:
    int destino_idx = -1;
    for (int i = 0; i < qntd_arquivos; i++) {
        //essa função "strcmp" compara duas strings e retorna 0 quando são iguais.
        if (strcmp(membros[i].nome, arquivo_destino) == 0) {
            destino_idx = i;
            break;
        }
    }

    //caso não exista arquivo destino, mover para o início:
    if (destino_idx == -1) {
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

        membros[mover_idx].data_modif = time(NULL);

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
    //caso exista um arquivo destino em archive.vc:
    else {
        //primeiro caso: quero mover para um arquivo que está antes.
        if (mover_idx > destino_idx) {
            for (int i=mover_idx-1 ; i>destino_idx ; i--) {
                char *temp = malloc(membros[i].tam_disco);
                if (!temp) {
                    fprintf(stderr, "Erro ao alocar memória para temp.\n");
                    free(buffer_mover);
                    free(membros);
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
            //escrevendo o arquivo_mover logo após o arquivo_destino:
            membros[mover_idx].offset = membros[destino_idx].offset + membros[destino_idx].tam_disco;
            fseek(archive, membros[destino_idx].offset, SEEK_SET);
            fwrite(buffer_mover, 1, membros[mover_idx].tam_disco, archive);
            free(buffer_mover);

            membros[mover_idx].data_modif = time(NULL);

            // Atualizando o novo diretório:
            //aqui eu atualizo o diretório copiado
            struct membro mover_membro = membros[mover_idx];
            for (int i = mover_idx; i > destino_idx+1; i--) {
                membros[i] = membros[i - 1];
            }
            //colocando o membro movido logo após o membrox:
            membros[destino_idx+1] = mover_membro;
        }
        //segundo caso: quero mover para um arquivo que está depois:
        else {
            for (int i=mover_idx+1 ; i<=destino_idx ; i++) {
                char *temp = malloc(membros[i].tam_disco);
                if (!temp) {
                    fprintf(stderr, "Erro ao alocar memória para temp.\n");
                    free(buffer_mover);
                    free(membros);
                    fclose(archive);
                    return;
                }

                fseek(archive, membros[i].offset, SEEK_SET);
                fread(temp, 1, membros[i].tam_disco, archive);

                membros[i].offset -= membros[mover_idx].tam_disco;
                fseek(archive, membros[i].offset, SEEK_SET);
                fwrite(temp, 1, membros[i].tam_disco, archive);
                free(temp);
            }
            //escrevendo o arquivo_mover logo após o arquivo_destino:
            membros[mover_idx].offset = membros[destino_idx].offset+membros[destino_idx].tam_disco;
            fseek(archive, membros[mover_idx].offset, SEEK_SET);
            fwrite(buffer_mover, 1, membros[mover_idx].tam_disco, archive);
            free(buffer_mover);

            membros[mover_idx].data_modif = time(NULL);

            // Atualizando o novo diretório:
            //aqui eu atualizo o diretório copiado
            struct membro mover_membro = membros[mover_idx];
            for (int i = mover_idx; i< destino_idx; i++) {
                membros[i] = membros[i + 1];
            }
            //colocando o membro movido logo após o membrox:
            membros[destino_idx] = mover_membro;
        }
        //apagando o diretório antigo e escrevendo o atualizado:
        ftruncate(fileno(archive), inicio_dir);
        fseek(archive, 0, SEEK_END);
        fwrite(membros, sizeof(struct membro), qntd_arquivos, archive);
        fwrite(&qntd_arquivos, sizeof(int), 1, archive);

        free(membros);
        fclose(archive);

        printf("Arquivo %s movido para depois do %s com sucesso.\n", arquivo_mover, arquivo_destino);

    }

}
//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO REMOVER:
void option_r(const char *nome_arquivo, char *arquivo_remover) {
    FILE *archive = fopen(nome_arquivo, "r+b");
    if (!archive) {
        perror("Erro ao abrir o arquivo.\n");
        return;
    }

    //---------------------------------------------------------//
    //descobrindo o tamanho do archive.vc:
    fseek(archive, 0, SEEK_END);
    long int tam_arquivo = ftell(archive);

    //descobrindo a quantidade de membros em archive.vc:
    int qntd_arquivos;
    fseek(archive, -sizeof(int), SEEK_END);
    fread(&qntd_arquivos, sizeof(int), 1, archive);
    printf("qntd de membros: %d\n", qntd_arquivos);
    

    //descobindo aonde começa o diretório:
    //calculando o tamanho total do diretório.
    long int tam_dir = sizeof(struct membro) * qntd_arquivos + sizeof(int);
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
    //---------------------------------------------------------//


    // encontrando e salvando o indice do arquivo_remover:
    int guarda_ind = -1;
    for (int i = 0; i < qntd_arquivos; i++) {
        //essa função "strcmp" compara duas strings e retorna 0 quando são iguais.
        if (strcmp(membros[i].nome, arquivo_remover) == 0) {
            guarda_ind = i;
            break;
        }
    }

    //se não existir o arquivo, printa que não existe e vai para o próximo.
    if (guarda_ind == -1) {
        free(membros);
        fclose(archive);
        return;
    }

    //caso exista, sobreescrever ele com os que estão à frente e truncar ao final.
    for (int j=guarda_ind+1 ; j<qntd_arquivos ; j++) {
        char *temp = malloc(membros[j].tam_disco);
        if (!temp) {
            fprintf(stderr, "Erro ao alocar memória para temp.\n");
            free(membros);
            fclose(archive);
            return;
        }

        fseek(archive, membros[j].offset, SEEK_SET);
        fread(temp, 1, membros[j].tam_disco, archive);

        membros[j].offset -= membros[guarda_ind].tam_disco;
        fseek(archive, membros[j].offset, SEEK_SET);
        fwrite(temp, 1, membros[j].tam_disco, archive);
        free(temp);
    }

    //tirando o membro do diretório
    for (int j=guarda_ind; j<qntd_arquivos-1; j++) {
        membros[j] = membros[j+1];
    }

    qntd_arquivos--;

    // recalcular novo final de dados
    long int nova_pos_fim = (qntd_arquivos == 0) ? 0 :
        membros[qntd_arquivos - 1].offset + membros[qntd_arquivos - 1].tam_disco;

    // truncar o arquivo
    ftruncate(fileno(archive), nova_pos_fim);

    // reescrever diretório atualizado
    fseek(archive, nova_pos_fim, SEEK_SET);
    fwrite(membros, sizeof(struct membro), qntd_arquivos, archive);
    fwrite(&qntd_arquivos, sizeof(int), 1, archive);

    printf("Arquivo %s removido com sucesso.\n", arquivo_remover);

    free(membros);
    fclose(archive);
}
//---------------------------------------------------------------------------------------------------------//
//FUNÇÃO EXTRAIR ARQUIVO:
void option_x(char *nome_arquivo, char *arquivo_extrair, int controle) {
    //Função main vai falar:
    //Se controle == 0, extrai todos os arquivos porque não houve indicação de membros.
    //Se controle == 1, extrai membro por membro, com um looping pegando cada arquivo na main.

    FILE *archive = fopen(nome_arquivo, "r+b");
    if (!archive) {
        perror("Erro ao abrir o arquivo.\n");
        return;
    }

    //---------------------------------------------------------//
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
    //---------------------------------------------------------//
    fclose(archive);


    if (controle == 1) {
        // encontrando e salvando o indice do arquivo_remover:
        int guarda_i = -1;
        for (int i = 0; i < qntd_arquivos; i++) {
            //essa função "strcmp" compara duas strings e retorna 0 quando são iguais.
            if (strcmp(membros[i].nome, arquivo_extrair) == 0) {
                guarda_i = i;
                break;
            }
        }
        if (guarda_i == -1) {
            printf("Elemento %s não encontrado para a extração!\n", arquivo_extrair);
            return;
        }

        //Se o arquivo estiver comprimido, descomprimir:
        extrair_membro(nome_arquivo, membros[guarda_i]);
    }
    //controle == 0:
    else {
        for (int i=0 ; i < qntd_arquivos ; i++) {
            extrair_membro(nome_arquivo, membros[i]);
        }
    }
    free(membros);
}



