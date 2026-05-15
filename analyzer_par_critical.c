#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "hash_table.h"

#define TAM_HASH 131071
#define URL_BUF_TAM   512


// STRUCT PARA OS DADOS DO LOG
typedef struct {
    char*  arq_buffer;// conteúdo bruto do arquivo
    char** linhas;// vetor de ponteiros para cada linha
    long   tot_linhas;
    long   tam_arquivo;
} LogStruct;

/* METODO DE LEITURA
Em vez de ler linha por linha com fgets(),
usamos um único fread() igual no ultimo projeto
e depois indexamos as linhas com ponteiros
*/

LogStruct ler_log(const char* arquivo){
    LogStruct log = {NULL, NULL, 0, 0};

    FILE* fp = fopen(arquivo, "rb");
    if (!fp){ 
        perror("fopen log"); 
        exit(EXIT_FAILURE); 
    }

    /* PJ1 - OTMZD
    char *arquivo_nome = argv[2];

    FILE *fp = fopen(arquivo_nome, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return 1;
    }
    // leitura em buffer
    fseek(fp, 0, SEEK_fim);
    long tamanho = ftell(fp);
    rewind(fp);

    char *buffer = malloc(tamanho + 1);
    size_t lidos = fread(buffer, 1, tamanho, fp);

    */
    // pega o tamanho do arquivo
    fseek(fp, 0, SEEK_END);
    long tam = ftell(fp);
    rewind(fp);

    log.tam_arquivo = tam;

    printf("Tamanho do arquivo: %ld bytes\n", tam);

    // aloca um buffer para o arquivo inteiro
    char* buf = malloc(tam + 1);
    if (!buf){ 
        perror("malloc"); 
        exit(EXIT_FAILURE);
    }

    // lê tudo de uma vez
    if (fread(buf, 1, tam, fp) != (size_t)tam) {
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
            {
                total++;
            }
        }
    }

    // última linha pode não ter '\n'
    if (tam > 0 && buf[tam - 1] != '\n') total++;
    
    log.tot_linhas = total;
    printf("Total de linhas: %ld\n", total);

    // monta o vetor de ponteiros, cada elemento aponta para o início de uma linha dentro do buffer
    char** linhas = malloc(sizeof(char*) * total);
    
    if (!linhas){ 
        perror("malloc linhas"); 
        exit(EXIT_FAILURE); 
    }

    long indc = 0;
    linhas[indc++] = buf;
    for (long i = 0; i < tam; i++){
        if (buf[i] == '\n'){
            buf[i] = '\0'; // termina a linha
            if (i + 1 < tam){
               linhas[indc++] = &buf[i + 1]; 
            } 
        }
    }
    log.linhas = linhas;
    return log;
}

void free_log(LogStruct* log) {
    free(log->linhas);
    free(log->arq_buffer);
}


// -----------------------------------------------------------------



/* 
Cria tablea
*/
void build_hash_table(HashTable* ht, const char* manifest) {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("TABELA HASH - COMEÇO\n");
    printf("\n(...)\n");
    FILE* fp = fopen(manifest, "r");
    if (!fp){ 
        perror("fopen manifest"); 
        exit(EXIT_FAILURE); 
    }

    char url[URL_BUF_TAM];
    long cont_put = 0;
    while (fgets(url, sizeof(url), fp)) {
        url[strcspn(url, "\r\n")] = '\0';   // remove quebra de linha
        ht_put(ht, url);
        cont_put++;
    }
    fclose(fp);


    printf("\nTABELA HASH - TERMINOU\n");
    printf("URLs inseridas : %ld\n", cont_put);
    printf("Buckets        : %d\n", TAM_HASH);
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
    if (len == 0 || len >= URL_BUF_TAM) return 0;

    memcpy(url_out, p, len);
    url_out[len] = '\0';
    return 1;
}


void process_log_critical(HashTable* ht, char** linhas, long total) {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("PROCESSAMENTO - INICIO\n");
    printf("Threads: %d  |  Linhas: %ld\n", omp_get_max_threads(), total);
    printf("Sincronizacao: omp critical\n\n");



    double inicio = omp_get_wtime();

    /* 
    GERAL: https://learn.microsoft.com/pt-br/cpp/parallel/openmp/reference/openmp-directives?view=msvc-170
    lendo sobre o parallel for, vimos:
    SCHEDULE: https://learn.microsoft.com/pt-br/cpp/parallel/openmp/reference/openmp-clauses?view=msvc-170#schedule
    schedule(static) divide as iterações em blocos iguais
    entre as threads. é bom quando o trabalho por linha é uniforme (
    */ 
    //#pragma omp parallel for schedule(static)
    #pragma omp parallel for 
    for (long i = 0; i < total; i++) {
        char url[URL_BUF_TAM];

        if (parsing_linha(linhas[i], url)){
            CacheNode* node = ht_get(ht, url);
            if (node){
                // apenas uma thread por vez incrementa o contador
                #pragma omp critical
                {
                    node->hit_count++;
                }
            }
        }

        // print do progresso (a cada 1M)
        if (i % 1000000 == 0 && i != 0) {
            #pragma omp critical
            printf("PROCESSAMENTO: %ld linhas processadas\n", i);
        }
    }

    double final = omp_get_wtime();
    printf("\nPROCESSAMENTO - TERMINOU\n");
    printf("Processamento finalizado em %.4f s\n", final - inicio);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}



// -----------------------------------------------------------------




int main(int argc, char* argv[]) {
    if (argc != 2){
        fprintf(stderr, "Uso: %s <arquivo_log>\n", argv[0]);
        return EXIT_FAILURE;
    }


    printf("\nOMP CRITICAL - analyzer_par_critical.c\n");
    printf("Log: %s  |  Threads: %d\n", argv[1], omp_get_max_threads());
    printf("\n\n");



    // 1. TABELA HASH
    HashTable* ht = ht_create(TAM_HASH);
    if (!ht){
         fprintf(stderr, "Erro ao criar hash table\n"); 
         return EXIT_FAILURE; 
    }
    build_hash_table(ht, "manifest.txt");



    // 2. LEITURA DO LOG
    printf("CARREGANDO LOG\n");
    double t0 = omp_get_wtime();
    LogStruct log = ler_log(argv[1]);
    printf("Carregado em %f s\n\n", omp_get_wtime() - t0);



    // 3. PROCESSAMENTO
    process_log_critical(ht, log.linhas, log.tot_linhas);




    // 4. SALVA RESULT
    printf("Salvando results.csv...\n");
    ht_save_results(ht, "results.csv");
    printf("Resultados salvos.\n\n");



    // 5. LIMPA MEMORIA
    free_log(&log);// func pra limpar
    ht_destroy(ht);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Memória limpa.\n");
    return EXIT_SUCCESS;
}
