// Comando de compilação: gcc ./analyzer_seq.c hash_table.c -o hash_table

#include <stdio.h>
#include <stdlib.h>

#include "hash_table.h"

void ler_log(HashTable* ht, FILE* file) {
    char line[1024];

    while (fgets(line, sizeof(line), file)) {
        char *ptr = line; // Ponteiro para percorrer a linha
        char *urlStart = NULL; // Ponteiro para marcar o início da URL
        while (*ptr != 'H') { // Percorre a linha até encontrar o 'H' de "HTTP"
            if (*ptr == '/') urlStart = ptr; // Marca o início da URL quando encontrar a primeira barra
            ptr++; // Move o ponteiro para o próximo caractere
        }
        ptr--; // Move o ponteiro para o caractere antes do 'H'
        *ptr = '\0'; //Substitui o espaço antes do "HTTP" por '\0' para terminar a string da URL
        ht_put(ht, urlStart); // Verifica se a URL já existe na tabela hash e incrementa o contador de hits
    }
}

int main(int argc, char* argv[]) {
    HashTable* ht = ht_create(131071); // Cria uma tabela hash com 131.071 buckets

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo_de_urls>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file1 = fopen(argv[1], "r");
    if (!file1) {
        perror("Erro ao abrir o arquivo");
        return EXIT_FAILURE;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file1)) {
        ht_put(ht, line); // Adiciona cada linha do arquivo à tabela hash
    }
    
    FILE* file2 = fopen(argv[2], "r");
    if (!file2) {
        perror("Erro ao abrir o arquivo");
        return EXIT_FAILURE;
    }
    
    ler_log(ht, file2); // Lê o arquivo de log e atualiza a tabela hash

    ht_save_results(ht, "results_seq.csv"); // Salva os resultados em um arquivo CSV
    
    ht_destroy(ht); // Libera a memória
    return 0;
}