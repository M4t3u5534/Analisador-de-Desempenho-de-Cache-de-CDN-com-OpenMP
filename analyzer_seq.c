// Comando de compilação: gcc ./analyzer_seq.c hash_table.c -o hash_table

#include <stdio.h>
#include <stdlib.h>

#include "hash_table.h"

int main() {
    HashTable* ht = ht_create(10);
    
    ht_put(ht, "http://example.com/page1");
    ht_put(ht, "http://example.com/page2");
    ht_put(ht, "http://example.com/page3");
    
    CacheNode* node = ht_get(ht, "http://example.com/page2");
    if (node) {
        printf("URL: %s, Hit Count: %ld\n", node->url, node->hit_count);
    } else {
        printf("URL não encontrada.\n");
    }
    
    ht_destroy(ht); // Libera a memória
    return 0;
}