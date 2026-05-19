# Analisador de Desempenho de Cache de CDN com OpenMP
**Computação Paralela — FCI Mackenzie**

---

## Sumário
- [Critical](#critical)
- [Atomic](#atomic)
- [Padded (False Sharing)](#padded)
- [Observações Gerais](#observações-gerais)

---

## Critical

### Compilação
```bash
gcc -O2 -fopenmp analyzer_par_critical.c hash_table.c -o analyzer_par_critical
```

### Execução — Experimento A (Escalabilidade)
```bash
export OMP_NUM_THREADS=1 && time ./analyzer_par_critical log_distribuido.txt
export OMP_NUM_THREADS=2 && time ./analyzer_par_critical log_distribuido.txt
export OMP_NUM_THREADS=4 && time ./analyzer_par_critical log_distribuido.txt
export OMP_NUM_THREADS=8 && time ./analyzer_par_critical log_distribuido.txt
```

### Execução — Experimento B (Contenção)
```bash
export OMP_NUM_THREADS=8
/usr/bin/time -v ./analyzer_par_critical log_concorrente.txt
```

### Análise — IPC via perf
```bash
perf stat -e instructions,cycles ./analyzer_par_critical log_concorrente.txt
```

### Análise — Cachegrind / Callgrind (laboratório)
```bash
valgrind --tool=cachegrind ./analyzer_par_critical log_concorrente.txt
valgrind --tool=callgrind  ./analyzer_par_critical log_concorrente.txt
```

### Validação
```bash
# log_distribuido
sort results.csv > sorted_res.csv
sort gabarito_distribuido.csv > sorted_gab.csv
diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv

# log_concorrente
sort results.csv > sorted_res.csv
sort gabarito_concorrente.csv > sorted_gab.csv
diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv
```

---

## Atomic

### Compilação
```bash
gcc -O2 -fopenmp analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic
```

### Execução — Experimento A (Escalabilidade)
```bash
export OMP_NUM_THREADS=1 && time ./analyzer_par_atomic log_distribuido.txt
export OMP_NUM_THREADS=2 && time ./analyzer_par_atomic log_distribuido.txt
export OMP_NUM_THREADS=4 && time ./analyzer_par_atomic log_distribuido.txt
export OMP_NUM_THREADS=8 && time ./analyzer_par_atomic log_distribuido.txt
```

### Execução — Experimento B (Contenção)
```bash
export OMP_NUM_THREADS=8
/usr/bin/time -v ./analyzer_par_atomic log_concorrente.txt
```

### Análise — IPC via perf
```bash
perf stat -e instructions,cycles ./analyzer_par_atomic log_concorrente.txt
```

### Análise — Cachegrind / Callgrind (laboratório)
```bash
valgrind --tool=cachegrind ./analyzer_par_atomic log_concorrente.txt
valgrind --tool=callgrind  ./analyzer_par_atomic log_concorrente.txt
```

### Validação
```bash
# log_distribuido
sort results.csv > sorted_res.csv
sort gabarito_distribuido.csv > sorted_gab.csv
diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv

# log_concorrente
sort results.csv > sorted_res.csv
sort gabarito_concorrente.csv > sorted_gab.csv
diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv
```

---

## Padded

> A flag `-DCOMPAT_PADDING` ativa o `long padding[5]` na struct `CacheNode`,
> alinhando cada nó a 64 bytes (uma cache line inteira).

### Compilação
```bash
gcc -O2 -fopenmp -DCOMPAT_PADDING analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic_padded
```

### Execução — Experimento C (False Sharing)
```bash
export OMP_NUM_THREADS=8
/usr/bin/time -v ./analyzer_par_atomic_padded log_concorrente.txt
```

### Análise — Cache misses via perf
```bash
perf stat -e cache-references,cache-misses ./analyzer_par_atomic_padded log_concorrente.txt
```

### Análise — Cachegrind (laboratório)
```bash
valgrind --tool=cachegrind ./analyzer_par_atomic_padded log_concorrente.txt
```

### Validação
```bash
sort results.csv > sorted_res.csv
sort gabarito_concorrente.csv > sorted_gab.csv
diff -s --strip-trailing-cr sorted_res.csv sorted_gab.csv
```

---

## Observações Gerais

- Os comandos `perf` podem **não funcionar no WSL2** com processadores Intel híbridos
  (Alder Lake / Raptor Lake) por falta de suporte ao PMU. Nesses casos, utilizar
  **Cachegrind/Callgrind no laboratório** como alternativa.

- A flag `--strip-trailing-cr` no `diff` resolve conflitos de fim de linha (`\r\n` vs `\n`)
  causados pela execução em ambiente WSL Ubuntu.

- O tempo medido por `omp_get_wtime()` representa apenas a **região paralela principal**,
  enquanto o `time` / `/usr/bin/time -v` mede o tempo total de execução do programa.

- Relação entre os tempos:
  ```
  T_real ≈ T_hash + T_carregamento + T_processamento(OMP) + T_salvamento + overheads
  T_OMP ⊂ T_real
  ```
