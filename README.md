# Laboratorio 6 - Acceso a Recursos Compartidos con Pthreads

**Universidad del Valle de Guatemala**  
**CC3086 Programación de Microprocesadores**  
**Ciclo 2 de 2025**

## Descripción

Este laboratorio implementa cinco prácticas progresivas sobre sincronización y acceso seguro a recursos compartidos usando POSIX Threads (Pthreads) en C++17.

## Estructura del Proyecto

```
Lab06/
├── include/
│   └── timing.hpp              # Temporizador para benchmarks
├── src/
│   ├── p1_counter.cpp          # Práctica 1: Race conditions
│   ├── p2_ring.cpp             # Práctica 2: Buffer circular
│   ├── p3_rw.cpp               # Práctica 3: Lectores/Escritores
│   ├── p4_deadlock.cpp         # Práctica 4: Deadlock
│   └── p5_pipeline.cpp         # Práctica 5: Pipeline con barreras
├── scripts/
│   ├── run_all.sh              # Ejecución completa
│   ├── benchmark.sh            # Benchmarking automatizado
│   └── analyze_results.py      # Análisis de resultados
├── bin/                        # Ejecutables (generado)
├── data/                       # Archivos de datos (generado)
├── results/                    # Resultados benchmarks (generado)
├── Makefile                    # Sistema de compilación
└── README.md                   # Este archivo
```

## Prerrequisitos

### Sistema Recomendado
- **Linux nativo** o **Windows 10/11 + WSL2 (Ubuntu 22.04+)**
- **GCC 9+** o **Clang 10+** con soporte C++17
- **Make** para automatización

### Instalación de Dependencias

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y build-essential clang gdb valgrind python3 python3-pip
pip3 install pandas matplotlib  # Opcional para análisis
```

#### Verificar Instalación
```bash
g++ --version
clang++ --version
make --version
```

## Compilación

### Compilación Básica
```bash
# Compilar todos los programas
make all

# Limpiar archivos generados
make clean

# Ver reglas disponibles
make help
```

### Compilación con Sanitizers (Debugging)
```bash
# ThreadSanitizer (detecta race conditions)
make tsan

# AddressSanitizer (detecta errores de memoria)
make asan
```

**⚠️ IMPORTANTE:** No usar sanitizers para benchmarks finales (añaden overhead significativo).

## Ejecución

### Ejecución Rápida (Todas las Prácticas)
```bash
# Hacer ejecutable el script
chmod +x scripts/run_all.sh

# Ejecutar todas las prácticas
./scripts/run_all.sh
```

### Ejecución Individual por Práctica

#### Práctica 1: Race Conditions en Contador
```bash
# Formato: ./bin/p1_counter [hilos] [iteraciones] [repeticiones]
./bin/p1_counter 4 1000000 1
./bin/p1_counter 8 500000 1

# Comparar diferentes enfoques:
# - NAIVE: Race condition intencional
# - MUTEX: Protección con mutex
# - SHARDED: Contadores por hilo
# - ATOMIC: std::atomic<long>
```

#### Práctica 2: Buffer Circular Productor/Consumidor
```bash
# Formato: ./bin/p2_ring [productores] [consumidores] [items_por_productor]
./bin/p2_ring 2 2 100000    # Balanceado
./bin/p2_ring 4 1 50000     # Múltiples productores
./bin/p2_ring 1 4 50000     # Múltiples consumidores

# Usa pthread_mutex_t + pthread_cond_t
# Evita busy waiting y pérdida de datos
```

#### Práctica 3: Lectores/Escritores
```bash
# Formato: ./bin/p3_rw [hilos] [operaciones_por_hilo]
./bin/p3_rw 6 50000

# Compara pthread_rwlock_t vs pthread_mutex_t
# Evalúa proporciones 90/10, 70/30, 50/50 (lectura/escritura)
```

#### Práctica 4: Deadlock y Corrección
```bash
# Ejecutar todas las demostraciones
./bin/p4_deadlock

# O ejecutar individualmente:
./bin/p4_deadlock 1    # Demostración de deadlock (⚠️ puede colgarse)
./bin/p4_deadlock 2    # Solución: orden total
./bin/p4_deadlock 3    # Solución: trylock + backoff
```

**⚠️ PRECAUCIÓN:** La opción 1 puede generar deadlock real. Usar Ctrl+C si se cuelga.

#### Práctica 5: Pipeline con Barreras
```bash
./bin/p5_pipeline

# Pipeline de 3 etapas: Generador → Filtro → Reducer
# Sincronización con pthread_barrier_t
# Inicialización con pthread_once_t
```

## Benchmarking Automatizado

### Ejecutar Benchmarks Completos
```bash
# Hacer ejecutable
chmod +x scripts/benchmark.sh

# Ejecutar benchmarks (5 repeticiones por configuración)
./scripts/benchmark.sh

# Resultados guardados en results/
```

### Análisis de Resultados
```bash
# Hacer ejecutable
chmod +x scripts/analyze_results.py

# Analizar resultados
python3 scripts/analyze_results.py results/

# Genera:
# - Estadísticas por configuración
# - Comparativas entre enfoques
# - CSV con datos agregados
# - Gráficos (si matplotlib disponible)
```

## Descripción de las Prácticas

### Práctica 1: Race Conditions en Contador Global
**Objetivos:**
- Demostrar race conditions en incremento concurrente
- Comparar mutex vs sharding vs atomic
- Medir throughput y overhead de sincronización

**Puntos Clave:**
- Race condition causa pérdida de incrementos
- Mutex garantiza corrección pero reduce paralelismo
- Sharding evita contención, requiere reduce
- std::atomic balance entre corrección y rendimiento

### Práctica 2: Buffer Circular MPMC
**Objetivos:**
- Implementar cola FIFO acotada thread-safe
- Usar condition variables para evitar busy waiting
- Manejar shutdown graceful sin pérdida de datos

**Puntos Clave:**
- pthread_cond_wait libera mutex atómicamente
- Usar while (no if) para spurious wakeups
- signal vs broadcast para eficiencia
- Política de terminación limpia

### Práctica 3: Lectores/Escritores con RWLock
**Objetivos:**
- Comparar mutex vs rwlock en tabla hash
- Evaluar throughput según proporción lectura/escritura
- Analizar starvation del escritor

**Puntos Clave:**
- RWLock permite múltiples lectores concurrentes
- Beneficioso cuando lecturas >> escrituras (≥70%)
- Overhead de rwlock puede ser contraproducente con muchas escrituras
- Considerar equidad entre lectores y escritores

### Práctica 4: Deadlock Clásico y Soluciones
**Objetivos:**
- Reproducir deadlock con dos mutex y orden opuesto
- Implementar soluciones: orden total y trylock
- Analizar condiciones de Coffman

**Puntos Clave:**
- Deadlock requiere: exclusión mutua + hold&wait + no preemption + circular wait
- Orden total elimina circular wait
- trylock + backoff elimina hold&wait
- Detección vs prevención vs recuperación

### Práctica 5: Pipeline Sincronizado con Barreras
**Objetivos:**
- Coordinar etapas de pipeline con barreras
- Usar pthread_once para inicialización única
- Medir throughput por etapa

**Puntos Clave:**
- pthread_barrier_wait sincroniza todos los hilos
- pthread_once garantiza inicialización única thread-safe
- Barreras vs colas: menor latencia, sincronización estricta
- Throughput limitado por etapa más lenta

## Debugging y Verificación

### Detectar Race Conditions
```bash
# Compilar con ThreadSanitizer
make tsan

# Ejecutar versión instrumentada
./bin/p1_counter_tsan 4 100000 1

# TSan reportará race conditions detectados
```

### Detectar Memory Errors
```bash
# Compilar con AddressSanitizer
make asan

# Ejecutar versión instrumentada
./bin/p2_ring_asan 2 2 10000
```

### Usar Valgrind (Alternativo)
```bash
# Helgrind para race conditions
valgrind --tool=helgrind ./bin/p1_counter 2 50000 1

# Memcheck para memory leaks
valgrind --tool=memcheck --leak-check=full ./bin/p2_ring 1 1 1000
```

### GDB para Deadlocks
```bash
# Compilar con debug info
g++ -g -pthread src/p4_deadlock.cpp -o bin/p4_deadlock_debug

# Si programa se cuelga, attach con GDB
gdb -p $(pgrep p4_deadlock)

# En GDB:
(gdb) info threads
(gdb) thread apply all bt
(gdb) thread 1
(gdb) bt
```

## Interpretación de Resultados

### Métricas Importantes
- **Throughput**: Operaciones por segundo
- **Latencia**: Tiempo de respuesta
- **Corrección**: Ausencia de pérdidas/corrupciones
- **Escalabilidad**: Rendimiento vs número de hilos
- **Fairness**: Distribución equitativa de recursos

### Indicadores de Problemas
- **Throughput muy bajo**: Alta contención
- **Resultados incorrectos**: Race conditions
- **Programa colgado**: Deadlock
- **Timeouts frecuentes**: Livelock o starvation
- **Memoria creciente**: Memory leaks

## Troubleshooting

### Problemas Comunes

#### Compilation Errors
```bash
# Error: pthread functions not found
# Solución: Verificar flag -pthread
g++ -pthread src/programa.cpp -o bin/programa

# Error: C++17 features not available
# Solución: Actualizar GCC/Clang o usar -std=gnu++17
```

#### Runtime Issues
```bash
# Programa se cuelga (posible deadlock)
# 1. Usar Ctrl+C para terminar
# 2. Ejecutar con timeout: timeout 30s ./programa
# 3. Debuggear con GDB

# Resultados inconsistentes (race conditions)
# 1. Ejecutar con ThreadSanitizer
# 2. Aumentar número de iteraciones
# 3. Verificar secciones críticas
```

#### Performance Issues
```bash
# Throughput muy bajo
# 1. Verificar contención en mutex
# 2. Considerar alternativas (sharding, rwlock)
# 3. Optimizar granularidad de locks

# Escalabilidad pobre
# 1. Analizar false sharing
# 2. Balancear carga entre hilos
# 3. Usar herramientas de profiling
```

## Comandos

1. Preparación y Compilación Inicial
```bash 
# Hacer scripts ejecutables
# chmod +x scripts/*.sh *.sh


# Verificación rápida inicial
# ./verify_lab.sh
```

2. Compilación con Sanitizers (para detectar problemas)
```bash 
Compilar con ThreadSanitizer
make tsan

# Probar con ThreadSanitizer (buscar race conditions)
./bin/p1_counter_tsan 2 10000 1
```

3. Pruebas Funcionales por Práctica
```bash 
# PRÁCTICA 1: Counter Race Conditions
# echo "=== P1: COUNTER ==="
# ./bin/p1_counter 4 100000 1

# PRÁCTICA 2: Buffer Circular
# echo "=== P2: RING BUFFER ==="
# ./bin/p2_ring 2 2 50000

# PRÁCTICA 3: Lectores/Escritores  
# echo "=== P3: READERS/WRITERS ==="
# ./bin/p3_rw 4 10000

# PRÁCTICA 4: Deadlock (solo soluciones seguras)
# echo "=== P4: DEADLOCK SOLUTIONS ==="
# ./bin/p4_deadlock 2  # Orden total
# ./bin/p4_deadlock 3  # Trylock

# PRÁCTICA 5: Pipeline
# echo "=== P5: PIPELINE ==="
# ./bin/p5_pipeline
```
4. Ejecución Completa Automatizada
Ejecutar todas las prácticas
./scripts/run_all.sh
5. Benchmarks Automatizados
Ejecutar benchmarks (esto toma varios minutos)
./scripts/benchmark.sh
6. Análisis de Resultados
Ver qué archivos se generaron
ls -la results/

# Analizar resultados (si tienes Python con pandas/matplotlib)
python3 scripts/analyze_results.py results/

# Si no tienes pandas, solo ver un resumen manual
head -20 results/benchmark_summary.txt
7. Verificación de Archivos del Proyecto
Estructura completa del proyecto
find . -type f -name "*.cpp" -o -name "*.hpp" -o -name "*.sh" -o -name "*.md" -o -name "Makefile" | sort

# Ver estadísticas de código
wc -l src/*.cpp include/*.hpp