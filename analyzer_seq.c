// Comando de compilação: gcc ./analyzer_seq.c hash_table.c -o hash_table

#include <stdio.h>
#include <stdlib.h>

#include "hash_table.h"

int main() {
    HashTable* ht = ht_create(131071); // Cria uma tabela hash com 131.071 buckets

    FILE* file1 = fopen("cdn_data_logs\\manifest.txt", "r"); 
    
    if (!file1) {
        perror("Erro ao abrir o arquivo");
        return EXIT_FAILURE;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file1)) {
        ht_put(ht, line); // Adiciona cada linha do arquivo à tabela hash
    }
    
    FILE* file2 = fopen("cdn_data_logs\\log_concorrente.txt", "r"); 
    
    if (!file2) {
        perror("Erro ao abrir o arquivo");
        return EXIT_FAILURE;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file2)) {
        char *ptr = line; // Ponteiro para percorrer a linha
        char *urlStart = NULL; // Ponteiro para marcar o início da URL
        
        while (*ptr != 'H') { // Percorre a linha até encontrar o 'H' de "HTTP"
            if (*ptr == '/') urlStart = ptr; // Marca o início da URL quando encontrar a primeira barra
            ptr++; // Move o ponteiro para o próximo caractere
        }
        ptr--; // Move o ponteiro para o caractere antes do 'H'
        *ptr = '\0'; //Substitui o espaço antes do "HTTP" por '\0' para terminar a string da URL

        CacheNode* node = ht_get(ht, urlStart); // Verifica se a URL já existe na tabela hash
        if (node) {
            node->hit_count++; // Incrementa o contador de hits se a URL já existir
        } else {
            
            ht_put(ht, urlStart); // Adiciona a URL à tabela hash se não existir
        }
    }
    
    ht_destroy(ht); // Libera a memória
    return 0;
}