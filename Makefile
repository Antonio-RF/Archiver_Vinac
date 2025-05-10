# Diretório do executável
BINDIR = login
EXEC = vina

# Arquivos-fonte e objetos
SRC = functions.c lz.c main.c secundary.c
OBJ = $(SRC:.c=.o)

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Regra principal
all: $(BINDIR)/$(EXEC)

# Compilação do executável
$(BINDIR)/$(EXEC): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Compilar arquivos .c em .o
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos gerados
clean:
	rm -f *.o
	rm -f $(BINDIR)/$(EXEC)