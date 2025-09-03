/**
 * Universidad del Valle de Guatemala
 * CC3086 Programación de Microprocesadores
 * Laboratorio 6 - Práctica 4: Deadlock Intencional y Corrección
 * 
 * Demuestra un deadlock clásico con dos mutex y dos hilos
 * Implementa soluciones: ordenación total y trylock con backoff
 */

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "../include/timing.hpp"

pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

// Variables para simular trabajo compartido
int shared_resource_A = 0;
int shared_resource_B = 0;
int operations_completed = 0;

// VERSIÓN CON DEADLOCK - DEMOSTRACIÓN
void* thread1_deadlock(void* arg) {
    int id = *static_cast<int*>(arg);
    printf("[Hilo %d] Iniciando (intentará A -> B)\n", id);
    
    printf("[Hilo %d] Solicitando mutex A...\n", id);
    pthread_mutex_lock(&mutex_A);
    printf("[Hilo %d] ✓ Obtuvo mutex A\n", id);
    
    // Simular trabajo con recurso A
    shared_resource_A += 10;
    usleep(100000);  // 100ms - ventana crítica para deadlock
    
    printf("[Hilo %d] Solicitando mutex B...\n", id);
    pthread_mutex_lock(&mutex_B);  // ⚠️ POTENCIAL DEADLOCK AQUÍ
    printf("[Hilo %d] ✓ Obtuvo mutex B\n", id);
    
    // Trabajo que requiere ambos recursos
    shared_resource_B += shared_resource_A;
    operations_completed++;
    
    printf("[Hilo %d] Liberando mutex B\n", id);
    pthread_mutex_unlock(&mutex_B);
    printf("[Hilo %d] Liberando mutex A\n", id);
    pthread_mutex_unlock(&mutex_A);
    
    printf("[Hilo %d] ✓ Completado exitosamente\n", id);
    return nullptr;
}

void* thread2_deadlock(void* arg) {
    int id = *static_cast<int*>(arg);
    printf("[Hilo %d] Iniciando (intentará B -> A)\n", id);
    
    printf("[Hilo %d] Solicitando mutex B...\n", id);
    pthread_mutex_lock(&mutex_B);
    printf("[Hilo %d] ✓ Obtuvo mutex B\n", id);
    
    // Simular trabajo con recurso B
    shared_resource_B += 20;
    usleep(100000);  // 100ms - ventana crítica para deadlock
    
    printf("[Hilo %d] Solicitando mutex A...\n", id);
    pthread_mutex_lock(&mutex_A);  // ⚠️ POTENCIAL DEADLOCK AQUÍ
    printf("[Hilo %d] ✓ Obtuvo mutex A\n", id);
    
    // Trabajo que requiere ambos recursos
    shared_resource_A += shared_resource_B;
    operations_completed++;
    
    printf("[Hilo %d] Liberando mutex A\n", id);
    pthread_mutex_unlock(&mutex_A);
    printf("[Hilo %d] Liberando mutex B\n", id);
    pthread_mutex_unlock(&mutex_B);
    
    printf("[Hilo %d] ✓ Completado exitosamente\n", id);
    return nullptr;
}

// SOLUCIÓN 1: ORDEN TOTAL (siempre adquirir A antes que B)
void* thread_ordered(void* arg) {
    int id = *static_cast<int*>(arg);
    printf("[Hilo %d] Iniciando con orden total (A -> B)\n", id);
    
    // SIEMPRE adquirir en el mismo orden: A primero, B después
    printf("[Hilo %d] Solicitando mutex A...\n", id);
    pthread_mutex_lock(&mutex_A);
    printf("[Hilo %d] ✓ Obtuvo mutex A\n", id);
    
    usleep(50000);  // Simular trabajo
    
    printf("[Hilo %d] Solicitando mutex B...\n", id);
    pthread_mutex_lock(&mutex_B);
    printf("[Hilo %d] ✓ Obtuvo mutex B\n", id);
    
    // Trabajo crítico
    shared_resource_A += id * 10;
    shared_resource_B += id * 20;
    operations_completed++;
    
    // Liberar en orden inverso
    printf("[Hilo %d] Liberando mutex B\n", id);
    pthread_mutex_unlock(&mutex_B);
    printf("[Hilo %d] Liberando mutex A\n", id);
    pthread_mutex_unlock(&mutex_A);
    
    printf("[Hilo %d] ✓ Completado con orden total\n", id);
    return nullptr;
}

// SOLUCIÓN 2: TRYLOCK CON BACKOFF
void* thread_trylock(void* arg) {
    int id = *static_cast<int*>(arg);
    printf("[Hilo %d] Iniciando con trylock + backoff\n", id);
    
    bool success = false;
    int attempts = 0;
    const int max_attempts = 10;
    
    while (!success && attempts < max_attempts) {
        attempts++;
        printf("[Hilo %d] Intento %d...\n", id, attempts);
        
        // Intentar adquirir el primer mutex
        pthread_mutex_t* first = (id == 1) ? &mutex_A : &mutex_B;
        pthread_mutex_t* second = (id == 1) ? &mutex_B : &mutex_A;
        
        if (pthread_mutex_trylock(first) == 0) {
            printf("[Hilo %d] ✓ Obtuvo primer mutex\n", id);
            
            // Intentar el segundo con timeout
            usleep(10000);  // Simular trabajo
            
            if (pthread_mutex_trylock(second) == 0) {
                printf("[Hilo %d] ✓ Obtuvo segundo mutex - SUCCESS!\n", id);
                
                // Trabajo crítico
                shared_resource_A += id * 5;
                shared_resource_B += id * 15;
                operations_completed++;
                success = true;
                
                pthread_mutex_unlock(second);
                printf("[Hilo %d] Liberó segundo mutex\n", id);
            } else {
                printf("[Hilo %d] ⚠️ No pudo obtener segundo mutex, reintentando...\n", id);
            }
            
            pthread_mutex_unlock(first);
            printf("[Hilo %d] Liberó primer mutex\n", id);
        } else {
            printf("[Hilo %d] ⚠️ No pudo obtener primer mutex\n", id);
        }
        
        if (!success) {
            // Backoff exponencial
            int backoff_ms = (1 << attempts) * 1000;  // 2^n milliseconds
            printf("[Hilo %d] Esperando %d ms antes de reintentar...\n", id, backoff_ms / 1000);
            usleep(backoff_ms);
        }
    }
    
    if (success) {
        printf("[Hilo %d] ✓ Completado con trylock (intentos: %d)\n", id, attempts);
    } else {
        printf("[Hilo %d] ✗ FALLÓ después de %d intentos\n", id, attempts);
    }
    
    return nullptr;
}

void reset_state() {
    shared_resource_A = 0;
    shared_resource_B = 0;
    operations_completed = 0;
}

void print_separator(const char* title) {
    printf("\n");
    for(int i = 0; i < 50; i++) printf("=");
    printf("\n%s\n", title);
    for(int i = 0; i < 50; i++) printf("=");
    printf("\n");
}

void run_deadlock_demo() {
    print_separator("DEMOSTRACIÓN DE DEADLOCK");
    printf("⚠️  Esta versión PUEDE generar deadlock!\n");
    printf("Si el programa se cuelga, usa Ctrl+C para terminar\n\n");
    
    reset_state();
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    double start = now_s();
    
    pthread_create(&t1, nullptr, thread1_deadlock, &id1);
    pthread_create(&t2, nullptr, thread2_deadlock, &id2);
    
    printf("Esperando máximo 5 segundos...\n");
    
    // Método simple de timeout usando alarma
    bool timeout_occurred = false;
    double elapsed = 0;
    
    while (elapsed < 5.0) {
        // Intentar join con timeout usando pthread_tryjoin_np si está disponible
        // Si no, usar método de polling simple
        usleep(100000);  // 100ms
        elapsed = now_s() - start;
        
        // Verificar si los hilos aún existen (método aproximado)
        int kill_result1 = pthread_kill(t1, 0);
        int kill_result2 = pthread_kill(t2, 0);
        
        if (kill_result1 != 0 && kill_result2 != 0) {
            // Ambos hilos terminaron
            pthread_join(t1, nullptr);
            pthread_join(t2, nullptr);
            break;
        }
        
        if (elapsed >= 5.0) {
            timeout_occurred = true;
            break;
        }
    }
    
    if (timeout_occurred) {
        printf("⚠️ POSIBLE DEADLOCK DETECTADO después de %.1f segundos\n", elapsed);
        printf("Los hilos no terminaron en tiempo razonable\n");
        
        // Cancelar hilos
        pthread_cancel(t1);
        pthread_cancel(t2);
        pthread_join(t1, nullptr);
        pthread_join(t2, nullptr);
    } else {
        printf("✓ Ambos hilos completaron en %.4f segundos\n", elapsed);
        printf("Operaciones completadas: %d/2\n", operations_completed);
    }
}

void run_ordered_solution() {
    print_separator("SOLUCIÓN 1: ORDEN TOTAL");
    
    reset_state();
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    double start = now_s();
    
    pthread_create(&t1, nullptr, thread_ordered, &id1);
    pthread_create(&t2, nullptr, thread_ordered, &id2);
    
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    
    double elapsed = now_s() - start;
    
    printf("✓ Solución completada en %.4f segundos\n", elapsed);
    printf("Operaciones completadas: %d/2\n", operations_completed);
    printf("Estado final - A: %d, B: %d\n", shared_resource_A, shared_resource_B);
}

void run_trylock_solution() {
    print_separator("SOLUCIÓN 2: TRYLOCK + BACKOFF");
    
    reset_state();
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    double start = now_s();
    
    pthread_create(&t1, nullptr, thread_trylock, &id1);
    pthread_create(&t2, nullptr, thread_trylock, &id2);
    
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    
    double elapsed = now_s() - start;
    
    printf("✓ Solución completada en %.4f segundos\n", elapsed);
    printf("Operaciones completadas: %d/2\n", operations_completed);
    printf("Estado final - A: %d, B: %d\n", shared_resource_A, shared_resource_B);
}

int main(int argc, char** argv) {
    printf("Laboratorio 6 - Práctica 4: Deadlock y Corrección\n");
    
    int demo_type = (argc > 1) ? std::atoi(argv[1]) : 0;
    
    switch (demo_type) {
        case 1:
            run_deadlock_demo();
            break;
        case 2:
            run_ordered_solution();
            break;
        case 3:
            run_trylock_solution();
            break;
        default:
            printf("Ejecutando todas las demostraciones...\n");
            run_deadlock_demo();
            run_ordered_solution();
            run_trylock_solution();
    }
    
    printf("\n=== ANÁLISIS DE CONDICIONES DE COFFMAN ===\n");
    printf("1. Exclusión mutua: ✓ (mutex no compartibles)\n");
    printf("2. Hold and wait: ✓ (mantener A mientras espera B)\n");
    printf("3. No preemption: ✓ (no se pueden quitar mutex por fuerza)\n");
    printf("4. Circular wait: ✓ (T1 espera B, T2 espera A)\n");
    printf("\nSOLUCIONES:\n");
    printf("- Orden total: Elimina circular wait\n");
    printf("- Trylock: Elimina hold and wait con timeout\n");
    
    pthread_mutex_destroy(&mutex_A);
    pthread_mutex_destroy(&mutex_B);
    
    return 0;
}