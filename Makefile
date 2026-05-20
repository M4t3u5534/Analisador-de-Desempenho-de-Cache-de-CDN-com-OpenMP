# Makefile para Analisador de Logs de Sensores IoT
# Este Makefile foi baseado nos Makefiles escritos pelo professor Lucas
# durante a disciplina de Sistemas Operacionais.


# Compilador
CC = gcc

# Arquivos fonte
SEQ_SRC = sensor_analyzer_seq.c
PAR_SRC = sensor_analyzer_par.c
OPT_SRC = sensor_analyzer_optimized.c

# Executáveis
SEQ = sensor_seq
PAR = sensor_par
OPT = sensor_opt

# Lista de todos os executáveis
TARGETS = $(SEQ) $(PAR) $(OPT)

# Regra padrão
.PHONY: all
all: $(TARGETS)
	@echo "Todos os programas foram compilados com sucesso!"
	@echo "Execute os programas individualmente:"
	@echo "  ./$(SEQ)"
	@echo "  ./$(PAR)"
	@echo "  ./$(OPT)"

# Regras de compilação (Utilizando os comandos específicos solicitados)
$(SEQ): $(SEQ_SRC)
	$(CC) $(SEQ_SRC) -o $(SEQ) -lm -O2

$(PAR): $(PAR_SRC)
	$(CC) $(PAR_SRC) -o $(PAR) -lpthread -lm

$(OPT): $(OPT_SRC)
	$(CC) $(OPT_SRC) -o $(OPT) -pthread -O2

# Executar testes
.PHONY: test
test: $(TARGETS)
	@echo "=== Executando Versão Sequencial ==="
	./$(SEQ)
	@echo ""
	@echo "=== Executando Versão Paralela ==="
	./$(PAR)
	@echo ""
	@echo "=== Executando Versão Otimizada ==="
	./$(OPT)

# Limpar executáveis
.PHONY: clean
clean:
	rm -f $(TARGETS)
	@echo "Executáveis removidos."

# Preparar ambiente (aproveitando o seu script python)
.PHONY: setup
setup:
	@echo "Gerando novo arquivo de logs..."
	python3 generate_sensor_log.py
	@echo "Arquivo sensores.log atualizado!"

# Ajuda
.PHONY: help
help:
	@echo "Makefile para Analisador de Logs de Sensores IoT"
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make all   - Compila todas as versões (Sequencial, Paralela, Otimizada)"
	@echo "  make seq   - Compila apenas a versão sequencial"
	@echo "  make par   - Compila apenas a versão paralela"
	@echo "  make opt   - Compila apenas a versão otimizada"
	@echo "  make test  - Executa as 3 versões em sequência"
	@echo "  make clean - Remove os executáveis compilados"
	@echo "  make setup - Roda o script python para gerar novos logs"
	@echo "  make help  - Mostra esta mensagem"

# Regras de atalho individuais
.PHONY: seq par opt
seq: $(SEQ)
	@echo "Versão Sequencial compilada!"

par: $(PAR)
	@echo "Versão Paralela compilada!"

opt: $(OPT)
	@echo "Versão Otimizada compilada!"