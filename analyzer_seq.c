// Comando de compilação: gcc ./analyzer_seq.c hash_table.c -o hash_table

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

    for (long i = 0; i < tam; i++){
        if (buf[i] == '\n'){
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
Cria tablea
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
Em vez de scanf fizemos o parsing manual ocm ponteiro direto, igual
no utlimo projeto.
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



void process_log(HashTable *ht, char **linhas, long tot_linhas)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("PROCESSAMENTO - INICIO\n");
    printf("Linhas: %ld\n", tot_linhas);

    clock_t inicio = clock();


    for (long i = 0; i < tot_linhas; i++) {
        char url[URL_tam];

        if (parsing_linha(linhas[i], url)) {
            CacheNode *node = ht_get(ht, url);
            if (node) node->hit_count++;
        }

        // progresso ocasional (a cada 1 milhão de linhas)
        if (i % 1000000 == 0 && i != 0) printf("PROCESSAMENTO: %ld linhas processadas\n", i);
    }

    clock_t final = clock();
    double diferenca = (double)(final - inicio) / CLOCKS_PER_SEC;
    printf("\nPROCESSAMENTO - TERMINOU\n");
    printf("Processamento finalizado em %f s\n", diferenca);
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
    HashTable *ht = ht_create(TABLE_tam); // Cria uma tabela hash com 131.071 buckets
    if (!ht)
    {
        fprintf(stderr, "Erro ao criar hash table\n");
        return EXIT_FAILURE;
    }
    build_hash_table(ht, "manifest.txt");

    // 2. LEITURA DO LOG
    printf("CARREGANDO LOG\n");
    clock_t t0 = clock();
    LogStruct log = ler_log(argv[1]);
    clock_t t1 = clock();
    double dif = (double)(t1 - t0) / CLOCKS_PER_SEC;
    printf("Carregado em %f s\n\n", dif);

    // 3. PROCESSAMENTO
    process_log(ht, log.linhas, log.tot_linhas);

    // 4. SALVA RESULT
    printf("Salvando results.csv...\n");
    ht_save_results(ht, "results.csv");
    printf("Resultados salvos.\n\n");

    // 5. LIMPA MEMORIA
    free_log(&log); // func pra limpar
    ht_destroy(ht);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Memória limpa.\n");
    return EXIT_SUCCESS;
}
