# Laboratorio 6 - Acceso a Recursos Compartidos con Pthreads

**Universidad del Valle de Guatemala**  
**CC3086 Programación de Microprocesadores**  
**Ciclo 2 de 2025**

## Descripción

Este laboratorio implementa cinco prácticas progresivas sobre sincronización y acceso seguro a recursos compartidos usando POSIX Threads (Pthreads) en C++17.

## Estructura del Proyecto

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
│   ├── run_all.sh              # Ejecutar todas las prácticas
│   ├── benchmark.sh            # Benchmarks automatizados
│   └── analyze_results.py      # Análisis de resultados
├── bin/                        # Ejecutables (generado)
├── data/                       # Archivos de datos (generado)
├── results/                    # Resultados benchmarks (generado)
├── Makefile                    # Sistema de compilación
└── README.md                   # Este archivo

## Prerrequisitos

- **Linux nativo** o **Windows 10/11 + WSL2 (Ubuntu 22.04+)**
- **GCC 9+** o **Clang 10+** con soporte C++17
- **Make**
- Opcional: **Python 3 + pandas/matplotlib** para análisis

Instalación en Ubuntu/Debian:
sudo apt update
sudo apt install -y build-essential clang gdb valgrind python3 python3-pip
pip3 install pandas matplotlib

## Compilación

# Compilar todo
make all

# Limpiar archivos generados
make clean

# Ver reglas disponibles
make help

Sanitizers (debugging):
make tsan   # ThreadSanitizer: detecta race conditions
make asan   # AddressSanitizer: detecta errores de memoria

⚠️ No usar sanitizers en benchmarks finales (añaden overhead).

## Ejecución

### Ejecución rápida
chmod +x scripts/run_all.sh
./scripts/run_all.sh

### Ejecución individual

Práctica 1 (Counter):
./bin/p1_counter 4 1000000 1

Práctica 2 (Ring Buffer):
./bin/p2_ring 2 2 100000

Práctica 3 (Lectores/Escritores):
./bin/p3_rw 6 50000

Práctica 4 (Deadlock):
./bin/p4_deadlock 1   # Deadlock (puede colgarse, Ctrl+C)
./bin/p4_deadlock 2   # Orden total
./bin/p4_deadlock 3   # Trylock

Práctica 5 (Pipeline):
./bin/p5_pipeline

## Benchmarks y Análisis

chmod +x scripts/benchmark.sh
./scripts/benchmark.sh

# Resultados en results/
python3 scripts/analyze_results.py results/

## Debugging y Verificación

ThreadSanitizer:
make tsan
./bin/p1_counter_tsan 4 100000 1

AddressSanitizer:
make asan
./bin/p2_ring_asan 2 2 10000

Valgrind:
valgrind --tool=helgrind ./bin/p1_counter 2 50000 1
valgrind --tool=memcheck --leak-check=full ./bin/p2_ring 1 1 1000

GDB:
g++ -g -pthread src/p4_deadlock.cpp -o bin/p4_deadlock_debug
gdb -p $(pgrep p4_deadlock)

## Descripción de las Prácticas

- **P1 (Counter):** Demuestra race conditions, compara mutex, sharding y atomic.  
- **P2 (Ring Buffer):** Cola FIFO con productores/consumidores, usa condition variables.  
- **P3 (Lectores/Escritores):** Evalúa mutex vs rwlock y el problema de starvation.  
- **P4 (Deadlock):** Reproduce deadlock clásico, soluciones con orden total y trylock.  
- **P5 (Pipeline):** Coordina etapas con barreras, mide throughput por etapa.

## Troubleshooting

- **pthread functions not found:** agregar -pthread.  
- **Resultados inconsistentes:** ejecutar con ThreadSanitizer.  
- **Programa colgado:** posible deadlock, terminar con Ctrl+C o usar timeout.  
- **Rendimiento bajo:** revisar contención de mutex, false sharing o granularidad de locks.