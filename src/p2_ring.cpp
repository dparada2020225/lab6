/**
 * Universidad del Valle de Guatemala
 * CC3086 Programación de Microprocesadores
 * Laboratorio 6 - Práctica 2: Buffer Circular Productor/Consumidor
 * Autor: Denil Parada 24761
 * Implementa una cola FIFO acotada con pthread_mutex_t y pthread_cond_t
 * Soporta múltiples productores y consumidores (MPMC)
 * Evita busy waiting usando condition variables
 */

#include <pthread.h>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include "../include/timing.hpp"

constexpr std::size_t QUEUE_SIZE = 1024;

struct Ring {
    int buf[QUEUE_SIZE];
    std::size_t head = 0;      // Índice de escritura
    std::size_t tail = 0;      // Índice de lectura
    std::size_t count = 0;     // Elementos actuales
    
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
    pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
    
    bool stop = false;         // Señal de parada
    
    // Estadísticas
    long total_produced = 0;
    long total_consumed = 0;
    long wait_full = 0;        // Veces que productores esperaron
    long wait_empty = 0;       // Veces que consumidores esperaron
};

/**
 * Insertar elemento en la cola (productor)
 * Bloquea si la cola está llena hasta que haya espacio
 */
void ring_push(Ring* r, int value) {
    pthread_mutex_lock(&r->mutex);
    
    // Esperar hasta que haya espacio O se active stop
    while (r->count == QUEUE_SIZE && !r->stop) {
        r->wait_full++;
        pthread_cond_wait(&r->not_full, &r->mutex);
    }
    
    // Solo insertar si no estamos en shutdown
    if (!r->stop) {
        r->buf[r->head] = value;
        r->head = (r->head + 1) % QUEUE_SIZE;
        r->count++;
        r->total_produced++;
        
        // Despertar a consumidores esperando
        pthread_cond_signal(&r->not_empty);
    }
    
    pthread_mutex_unlock(&r->mutex);
}

/**
 * Extraer elemento de la cola (consumidor)
 * Retorna false si la cola está vacía y se activó stop
 */
bool ring_pop(Ring* r, int* output) {
    pthread_mutex_lock(&r->mutex);
    
    // Esperar hasta que haya elementos O se active stop
    while (r->count == 0 && !r->stop) {
        r->wait_empty++;
        pthread_cond_wait(&r->not_empty, &r->mutex);
    }
    
    // Si no hay elementos y estamos en shutdown, terminar
    if (r->count == 0 && r->stop) {
        pthread_mutex_unlock(&r->mutex);
        return false;
    }
    
    // Extraer elemento
    *output = r->buf[r->tail];
    r->tail = (r->tail + 1) % QUEUE_SIZE;
    r->count--;
    r->total_consumed++;
    
    // Despertar a productores esperando
    pthread_cond_signal(&r->not_full);
    
    pthread_mutex_unlock(&r->mutex);
    return true;
}

/**
 * Iniciar shutdown graceful
 * Despierta a todos los hilos esperando
 */
void ring_shutdown(Ring* r) {
    pthread_mutex_lock(&r->mutex);
    r->stop = true;
    pthread_cond_broadcast(&r->not_full);
    pthread_cond_broadcast(&r->not_empty);
    pthread_mutex_unlock(&r->mutex);
}

void ring_destroy(Ring* r) {
    pthread_mutex_destroy(&r->mutex);
    pthread_cond_destroy(&r->not_full);
    pthread_cond_destroy(&r->not_empty);
}

struct ThreadArgs {
    Ring* ring;
    int thread_id;
    long iterations;
    double* thread_time;
};

// Hilo productor
void* producer_thread(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);
    Ring* r = args->ring;
    int id = args->thread_id;
    long iters = args->iterations;
    
    double start = now_s();
    
    for (long i = 0; i < iters; i++) {
        int value = id * 1000000 + i;  // Valor único por hilo
        ring_push(r, value);
        
        // Simular trabajo de producción
        if (i % 10000 == 0) {
            usleep(1);  // 1 microsegundo cada 10k items
        }
    }
    
    double end = now_s();
    args->thread_time[id] = end - start;
    
    printf("Productor %d terminó: %ld elementos en %.4fs\n", 
           id, iters, end - start);
    return nullptr;
}

// Hilo consumidor
void* consumer_thread(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);
    Ring* r = args->ring;
    int id = args->thread_id;
    
    double start = now_s();
    long consumed = 0;
    int value;
    
    while (ring_pop(r, &value)) {
        consumed++;
        
        // Simular trabajo de consumo
        if (consumed % 10000 == 0) {
            usleep(1);  // 1 microsegundo cada 10k items
        }
    }
    
    double end = now_s();
    args->thread_time[id] = end - start;
    
    printf("Consumidor %d terminó: %ld elementos en %.4fs\n", 
           id, consumed, end - start);
    return nullptr;
}

int main(int argc, char** argv) {
    int producers = (argc > 1) ? std::atoi(argv[1]) : 2;
    int consumers = (argc > 2) ? std::atoi(argv[2]) : 2;
    long items_per_producer = (argc > 3) ? std::atol(argv[3]) : 100000;
    
    printf("Laboratorio 6 - Práctica 2: Buffer Circular\n");
    printf("Configuración: %d productores, %d consumidores\n", producers, consumers);
    printf("Items por productor: %ld (total: %ld)\n", 
           items_per_producer, items_per_producer * producers);
    
    Ring ring;
    
    std::vector<pthread_t> producer_threads(producers);
    std::vector<pthread_t> consumer_threads(consumers);
    std::vector<ThreadArgs> producer_args(producers);
    std::vector<ThreadArgs> consumer_args(consumers);
    std::vector<double> producer_times(producers);
    std::vector<double> consumer_times(consumers);
    
    double start_time = now_s();
    
    // Crear productores
    for (int i = 0; i < producers; i++) {
        producer_args[i] = {&ring, i, items_per_producer, producer_times.data()};
        pthread_create(&producer_threads[i], nullptr, producer_thread, &producer_args[i]);
    }
    
    // Crear consumidores
    for (int i = 0; i < consumers; i++) {
        consumer_args[i] = {&ring, i, 0, consumer_times.data()};
        pthread_create(&consumer_threads[i], nullptr, consumer_thread, &consumer_args[i]);
    }
    
    // Esperar que terminen los productores
    for (int i = 0; i < producers; i++) {
        pthread_join(producer_threads[i], nullptr);
    }
    
    // Dar tiempo para que se procesen los elementos restantes
    printf("Productores terminaron, esperando consumidores...\n");
    sleep(1);
    
    // Iniciar shutdown
    ring_shutdown(&ring);
    
    // Esperar que terminen los consumidores
    for (int i = 0; i < consumers; i++) {
        pthread_join(consumer_threads[i], nullptr);
    }
    
    double total_time = now_s() - start_time;
    
    // Estadísticas finales
    printf("\n=== RESULTADOS ===\n");
    printf("Tiempo total: %.4f segundos\n", total_time);
    printf("Elementos producidos: %ld\n", ring.total_produced);
    printf("Elementos consumidos: %ld\n", ring.total_consumed);
    printf("Elementos perdidos: %ld\n", ring.total_produced - ring.total_consumed);
    printf("Elementos en cola: %zu\n", ring.count);
    
    printf("\n=== ESTADÍSTICAS DE BLOQUEO ===\n");
    printf("Productores esperaron (cola llena): %ld veces\n", ring.wait_full);
    printf("Consumidores esperaron (cola vacía): %ld veces\n", ring.wait_empty);
    
    if (ring.total_consumed > 0) {
        double throughput = ring.total_consumed / total_time;
        printf("Throughput: %.0f elementos/segundo\n", throughput);
    }
    
    // Verificar corrección
    bool correct = (ring.total_consumed == ring.total_produced) && (ring.count == 0);
    printf("Corrección: %s\n", correct ? "CORRECTO" : "ERROR - pérdida de datos");
    
    ring_destroy(&ring);
    return 0;
}