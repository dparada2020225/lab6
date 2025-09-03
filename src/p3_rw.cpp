/**
 * Universidad del Valle de Guatemala
 * CC3086 Programación de Microprocesadores
 * Laboratorio 6 - Práctica 3: Lectores/Escritores y Equidad
 * Autor: Denil Parada 24761
 * Compara pthread_rwlock_t vs pthread_mutex_t en una tabla hash compartida
 * Evalúa throughput bajo diferentes proporciones de lectura/escritura
 */

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include "../include/timing.hpp"

constexpr int NBUCKET = 1024;
constexpr int MAX_CHAIN = 8;

struct Node {
    int key;
    int value;
    Node* next;
    
    Node(int k, int v) : key(k), value(v), next(nullptr) {}
};

struct HashMapMutex {
    Node* buckets[NBUCKET];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Estadísticas
    long reads = 0;
    long writes = 0;
    long collisions = 0;
    
    HashMapMutex() {
        for (int i = 0; i < NBUCKET; i++) {
            buckets[i] = nullptr;
        }
    }
    
    ~HashMapMutex() {
        pthread_mutex_destroy(&mutex);
        // Cleanup chains
        for (int i = 0; i < NBUCKET; i++) {
            Node* current = buckets[i];
            while (current) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
    }
};

struct HashMapRWLock {
    Node* buckets[NBUCKET];
    pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
    
    // Estadísticas
    long reads = 0;
    long writes = 0;
    long collisions = 0;
    
    HashMapRWLock() {
        for (int i = 0; i < NBUCKET; i++) {
            buckets[i] = nullptr;
        }
    }
    
    ~HashMapRWLock() {
        pthread_rwlock_destroy(&rwlock);
        // Cleanup chains
        for (int i = 0; i < NBUCKET; i++) {
            Node* current = buckets[i];
            while (current) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
    }
};

// Función hash simple
inline int hash_func(int key) {
    return ((key * 2654435761U) >> 22) % NBUCKET;
}

// Operaciones con mutex
int map_get_mutex(HashMapMutex* map, int key) {
    pthread_mutex_lock(&map->mutex);
    
    int bucket = hash_func(key);
    Node* current = map->buckets[bucket];
    int result = -1;
    
    while (current) {
        if (current->key == key) {
            result = current->value;
            break;
        }
        current = current->next;
    }
    
    __sync_fetch_and_add(&map->reads, 1);
    pthread_mutex_unlock(&map->mutex);
    return result;
}

void map_put_mutex(HashMapMutex* map, int key, int value) {
    pthread_mutex_lock(&map->mutex);
    
    int bucket = hash_func(key);
    Node* current = map->buckets[bucket];
    
    // Buscar si ya existe
    while (current) {
        if (current->key == key) {
            current->value = value;  // Actualizar
            __sync_fetch_and_add(&map->writes, 1);
            pthread_mutex_unlock(&map->mutex);
            return;
        }
        current = current->next;
    }
    
    // Insertar nuevo nodo al inicio
    Node* new_node = new Node(key, value);
    new_node->next = map->buckets[bucket];
    if (map->buckets[bucket] != nullptr) {
        __sync_fetch_and_add(&map->collisions, 1);
    }
    map->buckets[bucket] = new_node;
    
    __sync_fetch_and_add(&map->writes, 1);
    pthread_mutex_unlock(&map->mutex);
}

// Operaciones con rwlock
int map_get_rwlock(HashMapRWLock* map, int key) {
    pthread_rwlock_rdlock(&map->rwlock);
    
    int bucket = hash_func(key);
    Node* current = map->buckets[bucket];
    int result = -1;
    
    while (current) {
        if (current->key == key) {
            result = current->value;
            break;
        }
        current = current->next;
    }
    
    __sync_fetch_and_add(&map->reads, 1);
    pthread_rwlock_unlock(&map->rwlock);
    return result;
}

void map_put_rwlock(HashMapRWLock* map, int key, int value) {
    pthread_rwlock_wrlock(&map->rwlock);
    
    int bucket = hash_func(key);
    Node* current = map->buckets[bucket];
    
    // Buscar si ya existe
    while (current) {
        if (current->key == key) {
            current->value = value;  // Actualizar
            __sync_fetch_and_add(&map->writes, 1);
            pthread_rwlock_unlock(&map->rwlock);
            return;
        }
        current = current->next;
    }
    
    // Insertar nuevo nodo al inicio
    Node* new_node = new Node(key, value);
    new_node->next = map->buckets[bucket];
    if (map->buckets[bucket] != nullptr) {
        __sync_fetch_and_add(&map->collisions, 1);
    }
    map->buckets[bucket] = new_node;
    
    __sync_fetch_and_add(&map->writes, 1);
    pthread_rwlock_unlock(&map->rwlock);
}

struct ThreadArgs {
    int thread_id;
    int total_ops;
    int read_percentage;  // 0-100
    double* execution_time;
    void* map;
    int map_type;  // 0=mutex, 1=rwlock
};

void* worker_thread(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);
    int id = args->thread_id;
    int ops = args->total_ops;
    int read_pct = args->read_percentage;
    
    // Seed para números aleatorios por hilo
    unsigned int seed = id * 12345;
    
    double start = now_s();
    
    for (int i = 0; i < ops; i++) {
        int key = rand_r(&seed) % 10000;  // Rango de claves
        int operation = rand_r(&seed) % 100;
        
        if (operation < read_pct) {
            // Operación de lectura
            if (args->map_type == 0) {
                map_get_mutex(static_cast<HashMapMutex*>(args->map), key);
            } else {
                map_get_rwlock(static_cast<HashMapRWLock*>(args->map), key);
            }
        } else {
            // Operación de escritura
            int value = id * 1000000 + i;
            if (args->map_type == 0) {
                map_put_mutex(static_cast<HashMapMutex*>(args->map), key, value);
            } else {
                map_put_rwlock(static_cast<HashMapRWLock*>(args->map), key, value);
            }
        }
        
        // Simular algo de trabajo
        if (i % 1000 == 0) {
            usleep(1);
        }
    }
    
    double end = now_s();
    args->execution_time[id] = end - start;
    
    return nullptr;
}

void run_benchmark(const char* name, int map_type, int threads, 
                   int ops_per_thread, int read_percentage) {
    printf("\n=== %s (R/W: %d/%d%%) ===\n", name, read_percentage, 100 - read_percentage);
    
    void* map;
    HashMapMutex* mutex_map = nullptr;
    HashMapRWLock* rwlock_map = nullptr;
    
    if (map_type == 0) {
        mutex_map = new HashMapMutex();
        map = mutex_map;
    } else {
        rwlock_map = new HashMapRWLock();
        map = rwlock_map;
    }
    
    std::vector<pthread_t> thread_handles(threads);
    std::vector<ThreadArgs> thread_args(threads);
    std::vector<double> execution_times(threads);
    
    double start_time = now_s();
    
    // Crear hilos
    for (int i = 0; i < threads; i++) {
        thread_args[i] = {i, ops_per_thread, read_percentage, 
                         execution_times.data(), map, map_type};
        pthread_create(&thread_handles[i], nullptr, worker_thread, &thread_args[i]);
    }
    
    // Esperar terminación
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_handles[i], nullptr);
    }
    
    double total_time = now_s() - start_time;
    
    // Recopilar estadísticas
    long total_reads, total_writes, total_collisions;
    if (map_type == 0) {
        total_reads = mutex_map->reads;
        total_writes = mutex_map->writes;
        total_collisions = mutex_map->collisions;
    } else {
        total_reads = rwlock_map->reads;
        total_writes = rwlock_map->writes;
        total_collisions = rwlock_map->collisions;
    }
    
    long total_ops = total_reads + total_writes;
    double throughput = total_ops / total_time;
    
    printf("Hilos: %d, Operaciones por hilo: %d\n", threads, ops_per_thread);
    printf("Tiempo total: %.4f segundos\n", total_time);
    printf("Lecturas: %ld, Escrituras: %ld\n", total_reads, total_writes);
    printf("Colisiones: %ld (%.2f%%)\n", total_collisions, 
           100.0 * total_collisions / total_writes);
    printf("Throughput: %.0f ops/segundo\n", throughput);
    
    // Calcular tiempo promedio por hilo
    double avg_thread_time = 0;
    for (int i = 0; i < threads; i++) {
        avg_thread_time += execution_times[i];
    }
    avg_thread_time /= threads;
    printf("Tiempo promedio por hilo: %.4f segundos\n", avg_thread_time);
    
    // Cleanup
    if (map_type == 0) {
        delete mutex_map;
    } else {
        delete rwlock_map;
    }
}

int main(int argc, char** argv) {
    int threads = (argc > 1) ? std::atoi(argv[1]) : 4;
    int ops_per_thread = (argc > 2) ? std::atoi(argv[2]) : 50000;
    
    printf("Laboratorio 6 - Práctica 3: Lectores/Escritores\n");
    printf("Configuración: %d hilos, %d operaciones por hilo\n", threads, ops_per_thread);
    
    // Probar diferentes proporciones de lectura/escritura
    std::vector<int> read_percentages = {90, 70, 50, 30, 10};
    
    for (int read_pct : read_percentages) {
        run_benchmark("MUTEX", 0, threads, ops_per_thread, read_pct);
        run_benchmark("RWLOCK", 1, threads, ops_per_thread, read_pct);
    }
    
    printf("\n=== ANÁLISIS ===\n");
    printf("- MUTEX: Exclusión mutua total, simple pero con alta contención\n");
    printf("- RWLOCK: Permite múltiples lectores concurrentes\n");
    printf("- RWLock es más eficiente cuando hay mayoría de lecturas (≥70%%)\n");
    printf("- Con muchas escrituras, el overhead de rwlock puede ser contraproducente\n");
    
    return 0;
}