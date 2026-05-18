// Comando de compilação: gcc -O2 -fopenmp analyzer_par_lock.c hash_table.c -o analyzer_par_lock

//export OMP_NUM_THREADS=8
//time ./analyzer_par_lock log_distribuido.txt
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#include "hash_table.h"

#define TABLE_tam 131071
#define URL_tam 512

// STRUCT PARA OS DADOS DO LOG
typedef struct
{
    char *arq_buffer; // conteúdo bruto do arquivo
    char **linhas;    // vetor de ponteiros para cada linha
    long tot_linhas;
    long tam_arquivo;
} LogStruct;

/* METODO DE LEITURA
Em vez de ler linha por linha com fgets(),
usamos um único fread() igual no ultimo projeto
e depois indexamos as linhas com ponteiros
*/

LogStruct ler_log(const char *arquivo)
{
    LogStruct log = {NULL, NULL, 0, 0};

    FILE *fp = fopen(arquivo, "rb");
    if (!fp)
    {
        perror("fopen log");
        exit(EXIT_FAILURE);
    }

    // pega o tamanho do arquivo
    fseek(fp, 0, SEEK_END);
    long tam = ftell(fp);
    rewind(fp);

    log.tam_arquivo = tam;

    printf("Tamanho do arquivo: %ld bytes\n", tam);

    // aloca um buffer para o arquivo inteiro
    char *buf = malloc(tam + 1);
    if (!buf)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // lê tudo de uma vez
    if (fread(buf, 1, tam, fp) != (size_t)tam)
    {
        fprintf(stderr, "Erro ao ler arquivo\n");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    buf[tam] = '\0';
    log.arq_buffer = buf;

    // conta as linhas em paralelo
    long total = 0;
    #pragma omp parallel for
    for (long i = 0; i < tam; i++){
        if (buf[i] == '\n'){
            #pragma omp critical
            total++;
        }
    }

    // última linha pode não ter '\n'
    if (tam > 0 && buf[tam - 1] != '\n')
        total++;

    log.tot_linhas = total;
    printf("Total de linhas: %ld\n", total);

    // monta o vetor de ponteiros, cada elemento aponta para o início de uma linha dentro do buffer
    char **linhas = malloc(sizeof(char *) * total);

    if (!linhas)
    {
        perror("malloc linhas");
        exit(EXIT_FAILURE);
    }

    long indc = 0;
    linhas[indc++] = buf;
    for (long i = 0; i < tam; i++)
    {
        if (buf[i] == '\n')
        {
            buf[i] = '\0'; // termina a linha
            if (i + 1 < tam)
            {
                linhas[indc++] = &buf[i + 1];
            }
        }
    }
    log.linhas = linhas;
    return log;
}

void free_log(LogStruct *log)
{
    free(log->linhas);
    free(log->arq_buffer);
}

// -----------------------------------------------------------------

/*
Cria tabela hash
*/
void build_hash_table(HashTable *ht, const char *manifest)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("TABELA HASH - COMEÇO\n");
    printf("\n(...)\n");
    FILE *fp = fopen(manifest, "r");
    if (!fp)
    {
        perror("fopen manifest");
        exit(EXIT_FAILURE);
    }

    char url[URL_tam];
    long inserted = 0;
    while (fgets(url, sizeof(url), fp))
    {
        url[strcspn(url, "\r\n")] = '\0'; // remove quebra de linha
        ht_put(ht, url);
        inserted++;
    }
    fclose(fp);

    printf("\nTABELA HASH - TERMINOU\n");
    printf("URLs inseridas : %ld\n", inserted);
    printf("Buckets        : %d\n", TABLE_tam);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}

// -----------------------------------------------------------------

/* METODO PARSING MANUAL
Em vez de scanf fizemos o parsing manual com ponteiro direto, igual
no ultimo projeto.
Formato: IP - - [data] "METHOD /url HTTP/1.1" status bytes
*/
int parsing_linha(const char* linha, char* url_out) {
    const char* p = linha;

    // pular: IP  espaço  -  espaço  -  espaço (---) 127.0.0.1 - - 
    for (int i = 0; i < 3; i++){
        p = strchr(p, ' ');
        if (!p) return 0;
        p++;    // avança o espaço
    }

    // pula o campo de data entre colchetes: [01/Nov/2025:10:00:01 -0300]
    if (*p != '[') return 0;
    p = strchr(p, ']');
    
    if (!p) return 0;
    p += 2;     // pula '] '

    // p agora aponta para: "GET /video/test.mp4 HTTP/1.1"
    if (*p != '"') return 0;
    p++;        // pula a aspas

    // pula o método (GET, POST …) até o próximo espaço
    p = strchr(p, ' ');
    if (!p) return 0;
    p++; // p aponta para a URL

    // copia a URL até o próximo espaço
    const char* fim = strchr(p, ' ');
    if (!fim) return 0;

    size_t len = fim - p;
    if (len == 0 || len >= URL_tam) return 0;

    memcpy(url_out, p, len);
    url_out[len] = '\0';
    return 1;
}

// -----------------------------------------------------------------
//555555555555555555555555555555555
/* FUNÇÃO HASH - COPIADA DE hash_table.c
Necessária para calcular o bucket correto sem modificar hash_table.h
Esta é a função DJB2 padrão usada na tabela hash
*/
static unsigned long hash_djb2(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// -----------------------------------------------------------------
//55555555555555555555555555555555555
/* PROCESSAMENTO COM BUCKET LOCK
Estratégia: cada bucket da tabela hash tem um lock independente
As threads só colidem se acessarem URLs que mapeiam para o MESMO bucket
Isso reduz contenção em comparação com critical (lock global)
*/
void process_log_lock(HashTable *ht, char **linhas, long tot_linhas, omp_lock_t *locks)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("PROCESSAMENTO (BUCKET LOCK) - INICIO\n");
    printf("Linhas: %ld\n", tot_linhas);

    double inicio = omp_get_wtime();

    // Cada thread processa um subset das linhas
    //55555555555555555555555555555
    #pragma omp parallel for
    for (long i = 0; i < tot_linhas; i++) {
        char url[URL_tam];

        // Extrai a URL da linha do log
        if (parsing_linha(linhas[i], url)) {
            // Calcula o bucket (índice) desta URL
            // Usa a mesma função hash da tabela para garantir consistência
            unsigned long hash_val = hash_djb2(url);
            size_t bucket = hash_val % TABLE_tam;

            // Adquire o lock específico deste bucket
            omp_set_lock(&locks[bucket]);

            // Busca o nó na tabela hash
            CacheNode *node = ht_get(ht, url);
            
            // Incrementa o contador apenas se o nó foi encontrado
            if (node) node->hit_count++;

            // Libera o lock deste bucket
            omp_unset_lock(&locks[bucket]);
        }
//55555555555555555555555555555555555555555
        // Progresso ocasional (a cada 1 milhão de linhas)
        if (i % 1000000 == 0 && i != 0) {
            #pragma omp critical
            printf("PROCESSAMENTO: %ld linhas processadas\n", i);
        }
    }
//555555555555555555555555555555555555555555
    double final = omp_get_wtime();
    printf("\nPROCESSAMENTO - TERMINOU\n");
    printf("Processamento finalizado em %.4f s\n", final - inicio);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}

// -----------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <arquivo_log>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 1. TABELA HASH
    printf("Criando tabela hash...\n");
    HashTable *ht = ht_create(TABLE_tam); // Cria uma tabela hash com 131.071 buckets
    if (!ht)
    {
        fprintf(stderr, "Erro ao criar hash table\n");
        return EXIT_FAILURE;
    }

    // PASSO 2: CRIAR UMA TABELA HASH5555555555555555
    build_hash_table(ht, "manifest.txt");

    // PASSO 3:CRIA UM ARRAY DE LOCKS5555555555555555
    // Um lock para cada bucket da tabela hash
    printf("Inicializando locks dos buckets...\n");
    omp_lock_t *locks = malloc(sizeof(omp_lock_t) * TABLE_tam);
    if (!locks)
    {
        fprintf(stderr, "Erro ao alocar array de locks\n");
        ht_destroy(ht);
        return EXIT_FAILURE;
    }

    // Inicializar cada lock sequencialmente (fora da seção paralela)
    for (size_t i = 0; i < TABLE_tam; i++)
    {
        omp_init_lock(&locks[i]);
    }
    printf("Locks inicializados.\n\n");

    // 4. LEITURA DO LOG
    printf("CARREGANDO LOG\n");
    printf("CARREGANDO LOG\n");
    double t0 = omp_get_wtime();
    LogStruct log = ler_log(argv[1]);
    printf("Carregado em %f s\n\n", omp_get_wtime() - t0);

    // 5. PROCESSAMENTO
    process_log_lock(ht, log.linhas, log.tot_linhas, locks);

    // 6. SALVA RESULT
    printf("Salvando results.csv...\n");
    ht_save_results(ht, "results.csv");
    printf("Resultados salvos.\n\n");

    // PASSO 7: DESTRUIR OS LOCKS5555555555555
    printf("Destruindo locks...\n");
    for (size_t i = 0; i < TABLE_tam; i++)
    {
        omp_destroy_lock(&locks[i]);
    }
    free(locks);
    printf("Locks destruídos.\n\n");

    // 8. LIMPA MEMORIA
    free_log(&log); // func pra limpar
    ht_destroy(ht);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Memória limpa. Programa finalizado.\n");
    return EXIT_SUCCESS;
}