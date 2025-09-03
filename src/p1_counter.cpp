/**
 * Universidad del Valle de Guatemala
 * CC3086 Programación de Microprocesadores
 * Laboratorio 6 - Práctica 1: Contador con Race Conditions
 * Autor: Denil Parada 24761
 * Demuestra race conditions en un contador global y sus soluciones:
 * A) Versión insegura con race conditions
 * B) Protección con pthread_mutex_t
 * C) Contadores particionados (sharded) con reduce
 * D) Comparación con std::atomic<long>
 */

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <atomic>
#include "../include/timing.hpp"

struct Args {
    long iters;
    long* global;
    pthread_mutex_t* mtx;
    long* local_counter;  // Para versión sharded
    int thread_id;
};

// A) Versión insegura - RACE CONDITION INTENCIONAL
void* worker_naive(void* p) {
    auto* a = static_cast<Args*>(p);
    for (long i = 0; i < a->iters; i++) {
        (*a->global)++;  // RACE: múltiples hilos modifican sin sincronización
    }
    return nullptr;
}

// B) Versión protegida con mutex
void* worker_mutex(void* p) {
    auto* a = static_cast<Args*>(p);
    for (long i = 0; i < a->iters; i++) {
        pthread_mutex_lock(a->mtx);
        (*a->global)++;
        pthread_mutex_unlock(a->mtx);
    }
    return nullptr;
}

// C) Versión sharded - cada hilo tiene su contador local
void* worker_sharded(void* p) {
    auto* a = static_cast<Args*>(p);
    for (long i = 0; i < a->iters; i++) {
        a->local_counter[a->thread_id]++;  // Sin contención
    }
    return nullptr;
}

// D) Versión con atomic para comparación
std::atomic<long> atomic_counter{0};

void* worker_atomic(void* p) {
    auto* a = static_cast<Args*>(p);
    for (long i = 0; i < a->iters; i++) {
        atomic_counter.fetch_add(1, std::memory_order_relaxed);
    }
    return nullptr;
}

void run_test(const char* name, void* (*worker)(void*), int T, long iterations) {
    printf("\n=== %s ===\n", name);
    
    long global = 0;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    std::vector<pthread_t> threads(T);
    std::vector<long> local_counters(T, 0);
    std::vector<Args> args(T);
    
    // Reset atomic counter
    atomic_counter.store(0);
    
    // Preparar argumentos para cada hilo
    for (int i = 0; i < T; i++) {
        args[i] = {iterations, &global, &mtx, local_counters.data(), i};
    }
    
    double start = now_s();
    
    // Crear hilos
    for (int i = 0; i < T; i++) {
        pthread_create(&threads[i], nullptr, worker, &args[i]);
    }
    
    // Esperar terminación
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], nullptr);
    }
    
    double end = now_s();
    double elapsed = end - start;
    
    // Para versión sharded, hacer reduce
    if (worker == worker_sharded) {
        for (int i = 0; i < T; i++) {
            global += local_counters[i];
        }
    }
    
    // Para versión atomic, obtener el valor final
    if (worker == worker_atomic) {
        global = atomic_counter.load();
    }
    
    long expected = (long)T * iterations;
    double ops_per_sec = (expected) / elapsed;
    
    printf("Hilos: %d, Iteraciones: %ld\n", T, iterations);
    printf("Resultado: %ld (esperado: %ld)\n", global, expected);
    printf("Diferencia: %ld (%.2f%%)\n", expected - global, 
           100.0 * (expected - global) / expected);
    printf("Tiempo: %.4f segundos\n", elapsed);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    
    pthread_mutex_destroy(&mtx);
}

int main(int argc, char** argv) {
    int T = (argc > 1) ? std::atoi(argv[1]) : 4;
    long iterations = (argc > 2) ? std::atol(argv[2]) : 1000000;
    int runs = (argc > 3) ? std::atoi(argv[3]) : 1;
    
    printf("Laboratorio 6 - Práctica 1: Race Conditions en Contador\n");
    printf("Configuración: %d hilos, %ld iteraciones por hilo\n", T, iterations);
    
    for (int run = 0; run < runs; run++) {
        printf("\n>>> EJECUCIÓN %d <<<\n", run + 1);
        
        run_test("A) NAIVE (Race Condition)", worker_naive, T, iterations);
        run_test("B) MUTEX (Protegido)", worker_mutex, T, iterations);
        run_test("C) SHARDED (Sin contención)", worker_sharded, T, iterations);
        run_test("D) ATOMIC (C++17)", worker_atomic, T, iterations);
    }
    
    printf("\n=== ANÁLISIS ===\n");
    printf("1. NAIVE: Muestra pérdidas por race conditions\n");
    printf("2. MUTEX: Correcto pero con alta contención\n");
    printf("3. SHARDED: Mayor throughput, requiere reduce\n");
    printf("4. ATOMIC: Balance entre corrección y rendimiento\n");
    
    return 0;
}