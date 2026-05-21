# Makefile para Analisador de Desempenho de Cache de CDN com OpenMP
# Este Makefile foi baseado nos Makefiles escritos pelo professor Lucas
# durante a disciplina de Sistemas Operacionais.


# Compilador
CC = gcc

# Flags de compilacao
CFLAGS = -O2 -Wall -Wextra -fopenmp

# Arquivos fonte
SEQ_SRC = analyzer_seq.c
CRITICAL_SRC = analyzer_par_critical.c
ATOMIC_SRC = analyzer_par_atomic.c
LOCK_SRC = analyzer_par_lock.c
HASH_SRC = hash_table.c

# Executaveis
SEQ = analyzer_seq
CRITICAL = analyzer_par_critical
ATOMIC = analyzer_par_atomic
LOCK = analyzer_par_lock
PADDED = analyzer_par_atomic_padded

# Lista de todos os executaveis
TARGETS = $(SEQ) $(CRITICAL) $(ATOMIC) $(LOCK) $(PADDED)

# Regra padrao
.PHONY: all
all: $(TARGETS)
	@echo "Todos os programas foram compilados com sucesso!"
	@echo "Execute os programas individualmente:"
	@echo "  ./$(SEQ)"
	@echo "  ./$(CRITICAL)"
	@echo "  ./$(ATOMIC)"
	@echo "  ./$(LOCK)"
	@echo "  ./$(PADDED)"

# Regras de compilacao

$(SEQ):
	$(CC) $(CFLAGS) \
	$(SEQ_SRC) $(HASH_SRC) \
	-o $(SEQ)

$(CRITICAL):
	$(CC) $(CFLAGS) \
	$(CRITICAL_SRC) $(HASH_SRC) \
	-o $(CRITICAL)

$(ATOMIC):
	$(CC) $(CFLAGS) \
	$(ATOMIC_SRC) $(HASH_SRC) \
	-o $(ATOMIC)

$(LOCK):
	$(CC) $(CFLAGS) \
	$(LOCK_SRC) $(HASH_SRC) \
	-o $(LOCK)

$(PADDED):
	$(CC) $(CFLAGS) -DCOMPAT_PADDING \
	$(ATOMIC_SRC) $(HASH_SRC) \
	-o $(PADDED)

# Validacao dos resultados

.PHONY: validate_distribuido
validate_distribuido:
	sort results.csv > sorted_res.csv
	sort gabarito_distribuido.csv > sorted_gab.csv
	diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv

.PHONY: validate_concorrente
validate_concorrente:
	sort results.csv > sorted_res.csv
	sort gabarito_concorrente.csv > sorted_gab.csv
	diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv

# Limpar executaveis

.PHONY: clean
clean:
	rm -f $(TARGETS)
	@echo "Executaveis removidos."

# Limpar arquivos de validacao

.PHONY: clean_validation
clean_validation:
	rm -f sorted_res.csv
	rm -f sorted_gab.csv
	@echo "Arquivos de validacao removidos."

# Ajuda

.PHONY: help
help:
	@echo "Makefile para Analisador de Desempenho de Cache de CDN com OpenMP"
	@echo ""
	@echo "Comandos disponiveis:"
	@echo "  make all                    - Compila todas as versoes"
	@echo "  make analyzer_seq           - Compila apenas a versao sequencial"
	@echo "  make analyzer_par_critical  - Compila apenas a versao critical"
	@echo "  make analyzer_par_atomic    - Compila apenas a versao atomic"
	@echo "  make analyzer_par_lock      - Compila apenas a versao bucket lock"
	@echo "  make analyzer_par_atomic_padded - Compila a versao padded"
	@echo ""
	@echo "Validacao:"
	@echo "  make validate_distribuido   - Valida log_distribuido.txt"
	@echo "  make validate_concorrente   - Valida log_concorrente.txt"
	@echo ""
	@echo "Limpeza:"
	@echo "  make clean                  - Remove os executaveis"
	@echo "  make clean_validation       - Remove arquivos de validacao"
	@echo ""
	@echo "Ajuda:"
	@echo "  make help                   - Mostra esta mensagem"

# Regras de atalho individuais

.PHONY: seq critical atomic lock padded

seq: $(SEQ)
	@echo "Versao Sequencial compilada!"

critical: $(CRITICAL)
	@echo "Versao Critical compilada!"

atomic: $(ATOMIC)
	@echo "Versao Atomic compilada!"

lock: $(LOCK)
	@echo "Versao Bucket Lock compilada!"

padded: $(PADDED)
	@echo "Versao Atomic Padded compilada!"
