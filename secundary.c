#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "functions.h"
#include "lz.h"
#include "secundary.h"


int confere_existe(struct diretorio dir, char *nome) {
    int guarda_i = -1;
    for (int i=0 ; i<dir.qntd_de_membros ; i++) {
        //se o nome do arquivo for igual ao elemento i do diretório antigo:
        if (strcmp(dir.elemento[i].nome, nome) == 0) {
            guarda_i = i;
            break;
        }
    }
    return guarda_i;
}

void extrair_membro(char *nome_arquivo, struct membro m) {
    FILE *archive = fopen(nome_arquivo, "rb");
    if (!archive) {
        perror("Erro ao abrir o arquivo");
        return;
    }

    FILE *extraido = fopen(m.nome, "wb");
    if (!extraido) {
        fprintf(stderr, "Erro ao criar arquivo %s.\n", m.nome);
        return;
    }

    char *buffer = malloc(m.tam_disco);
    if (!buffer) {
        fprintf(stderr, "Erro ao alocar buffer.\n");
        fclose(extraido);
        return;
    }

    fseek(archive, m.offset, SEEK_SET);
    fread(buffer, 1, m.tam_disco, archive);

    if (m.tam_disco != m.tam_original) {
        printf("extraindo um comprimido\n");
        // Arquivo comprimido: descomprimir
        unsigned char *descomprimido = malloc(m.tam_original);
        if (!descomprimido) {
            fprintf(stderr, "Erro ao alocar buffer para descompressão.\n");
            free(buffer);
            fclose(extraido);
            return;
        }
        LZ_Uncompress((unsigned char *)buffer, descomprimido, (unsigned int)m.tam_disco);
        fwrite(descomprimido, 1, m.tam_original, extraido);
        free(descomprimido);
    } else {
        // Arquivo normal
        fwrite(buffer, 1, m.tam_disco, extraido);
    }

    free(buffer);
    fclose(extraido);
}