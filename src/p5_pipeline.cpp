/**
 * Universidad del Valle de Guatemala
 * CC3086 Programación de Microprocesadores
 * Laboratorio 6 - Práctica 5: Pipeline con Barreras
 * 
 * Implementa un pipeline de tres etapas sincronizadas con pthread_barrier_t
 * Usa pthread_once_t para inicialización única compartida
 */

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <cstring>
#include "../include/timing.hpp"

constexpr int TICKS = 1000;
constexpr int BUFFER_SIZE = 100;

// Variables globales compartidas
static pthread_barrier_t sync_barrier;
static pthread_once_t once_flag = PTHREAD_ONCE_INIT;
static bool pipeline_shutdown = false;

// Buffers entre etapas
static int buffer_gen_to_filter[BUFFER_SIZE];
static int buffer_filter_to_reduce[BUFFER_SIZE];
static int processed_count = 0;

// Estadísticas por etapa
struct StageStats {
    long items_processed = 0;
    double total_time = 0.0;
    double min_time = 1000.0;
    double max_time = 0.0;
    int stage_id = 0;
};

static StageStats stats[3];
static FILE* log_file = nullptr;

/**
 * Inicialización única compartida - llamada por pthread_once
 * Simula apertura de archivos, reserva de buffers, etc.
 */
static void init_shared_resources() {
    printf("[INIT] Inicializando recursos compartidos (pthread_once)...\n");
    
    // Abrir archivo de log
    log_file = fopen("data/pipeline.log", "w");
    if (log_file) {
        fprintf(log_file, "Pipeline Log - Inicio: %.2f\n", now_s());
        fflush(log_file);
    }
    
    // Inicializar buffers
    memset(buffer_gen_to_filter, 0, sizeof(buffer_gen_to_filter));
    memset(buffer_filter_to_reduce, 0, sizeof(buffer_filter_to_reduce));
    
    // Inicializar estadísticas
    for (int i = 0; i < 3; i++) {
        stats[i].stage_id = i + 1;
    }
    
    printf("[INIT] ✓ Recursos inicializados\n");
}

/**
 * Etapa 1: Generador de datos
 * Produce números secuenciales para el pipeline
 */
void* stage_generator(void* arg) {
    long stage_id = reinterpret_cast<long>(arg);
    printf("[STAGE %ld] Generador iniciado\n", stage_id);
    
    // Inicialización única
    pthread_once(&once_flag, init_shared_resources);
    
    double stage_start = now_s();
    
    for (int tick = 0; tick < TICKS && !pipeline_shutdown; tick++) {
        double tick_start = now_s();
        
        // TRABAJO DE GENERACIÓN
        // Generar batch de datos
        for (int i = 0; i < BUFFER_SIZE; i++) {
            buffer_gen_to_filter[i] = tick * BUFFER_SIZE + i;
        }
        
        // Simular trabajo computacional
        int sum = 0;
        for (int i = 0; i < 1000; i++) {
            sum += i * tick;
        }
        
        stats[0].items_processed += BUFFER_SIZE;
        
        double tick_time = now_s() - tick_start;
        stats[0].total_time += tick_time;
        if (tick_time < stats[0].min_time) stats[0].min_time = tick_time;
        if (tick_time > stats[0].max_time) stats[0].max_time = tick_time;
        
        // Log periódico
        if (log_file && tick % 100 == 0) {
            fprintf(log_file, "[GEN] Tick %d completado en %.4f ms\n", 
                    tick, tick_time * 1000);
            fflush(log_file);
        }
        
        // SINCRONIZACIÓN: Esperar a que todas las etapas completen el tick
        printf("[GEN] Tick %d completado, esperando sincronización...\n", tick);
        pthread_barrier_wait(&sync_barrier);
        
        if (tick % 100 == 0) {
            printf("[GEN] Progreso: %d/%d ticks (%.1f%%)\n", 
                   tick, TICKS, 100.0 * tick / TICKS);
        }
    }
    
    double stage_end = now_s();
    printf("[STAGE %ld] Generador terminado en %.4f segundos\n", 
           stage_id, stage_end - stage_start);
    
    return nullptr;
}

/**
 * Etapa 2: Filtro/Transformación
 * Procesa los datos del generador
 */
void* stage_filter(void* arg) {
    long stage_id = reinterpret_cast<long>(arg);
    printf("[STAGE %ld] Filtro iniciado\n", stage_id);
    
    // Inicialización única
    pthread_once(&once_flag, init_shared_resources);
    
    double stage_start = now_s();
    
    for (int tick = 0; tick < TICKS && !pipeline_shutdown; tick++) {
        double tick_start = now_s();
        
        // TRABAJO DE FILTRADO
        // Leer del buffer anterior y procesar
        int valid_items = 0;
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int value = buffer_gen_to_filter[i];
            
            // Filtro: solo números pares y múltiplos de 3
            if (value % 2 == 0 && value % 3 == 0) {
                buffer_filter_to_reduce[valid_items] = value * 2;  // Transformación
                valid_items++;
            }
        }
        
        // Simular trabajo adicional
        for (int i = 0; i < 500; i++) {
            volatile int temp = i * valid_items;  // Prevenir optimización
            (void)temp;
        }
        
        stats[1].items_processed += valid_items;
        
        double tick_time = now_s() - tick_start;
        stats[1].total_time += tick_time;
        if (tick_time < stats[1].min_time) stats[1].min_time = tick_time;
        if (tick_time > stats[1].max_time) stats[1].max_time = tick_time;
        
        // Log periódico
        if (log_file && tick % 100 == 0) {
            fprintf(log_file, "[FILTER] Tick %d: %d items válidos en %.4f ms\n", 
                    tick, valid_items, tick_time * 1000);
            fflush(log_file);
        }
        
        // SINCRONIZACIÓN
        printf("[FILTER] Tick %d: %d items procesados, sincronizando...\n", 
               tick, valid_items);
        pthread_barrier_wait(&sync_barrier);
    }
    
    double stage_end = now_s();
    printf("[STAGE %ld] Filtro terminado en %.4f segundos\n", 
           stage_id, stage_end - stage_start);
    
    return nullptr;
}

/**
 * Etapa 3: Reducer/Agregador
 * Consume y agrega los datos filtrados
 */
void* stage_reducer(void* arg) {
    long stage_id = reinterpret_cast<long>(arg);
    printf("[STAGE %ld] Reducer iniciado\n", stage_id);
    
    // Inicialización única
    pthread_once(&once_flag, init_shared_resources);
    
    double stage_start = now_s();
    long accumulated_sum = 0;
    
    for (int tick = 0; tick < TICKS && !pipeline_shutdown; tick++) {
        double tick_start = now_s();
        
        // TRABAJO DE REDUCCIÓN
        // Procesar datos del filtro
        long tick_sum = 0;
        int items_count = 0;
        
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int value = buffer_filter_to_reduce[i];
            if (value > 0) {  // Solo procesar valores válidos
                tick_sum += value;
                items_count++;
            }
        }
        
        accumulated_sum += tick_sum;
        processed_count += items_count;
        
        // Simular trabajo de escritura/persistencia
        if (tick_sum > 0) {
            usleep(100);  // Simular I/O
        }
        
        stats[2].items_processed += items_count;
        
        double tick_time = now_s() - tick_start;
        stats[2].total_time += tick_time;
        if (tick_time < stats[2].min_time) stats[2].min_time = tick_time;
        if (tick_time > stats[2].max_time) stats[2].max_time = tick_time;
        
        // Log periódico
        if (log_file && tick % 100 == 0) {
            fprintf(log_file, "[REDUCE] Tick %d: suma=%ld, items=%d en %.4f ms\n", 
                    tick, tick_sum, items_count, tick_time * 1000);
            fflush(log_file);
        }
        
        // SINCRONIZACIÓN
        printf("[REDUCE] Tick %d: suma=%ld, total acumulado=%ld\n", 
               tick, tick_sum, accumulated_sum);
        pthread_barrier_wait(&sync_barrier);
    }
    
    double stage_end = now_s();
    printf("[STAGE %ld] Reducer terminado en %.4f segundos\n", 
           stage_id, stage_end - stage_start);
    printf("[REDUCE] Suma total acumulada: %ld\n", accumulated_sum);
    
    return nullptr;
}

void cleanup_resources() {
    if (log_file) {
        fprintf(log_file, "Pipeline terminado: %.2f\n", now_s());
        fclose(log_file);
    }
    pthread_barrier_destroy(&sync_barrier);
}

void print_statistics() {
    printf("\n" "=" * 60 "\n");
    printf("ESTADÍSTICAS DEL PIPELINE\n");
    printf("=" * 60 "\n");
    
    const char* stage_names[] = {"GENERADOR", "FILTRO", "REDUCER"};
    
    for (int i = 0; i < 3; i++) {
        printf("\n--- %s ---\n", stage_names[i]);
        printf("Items procesados: %ld\n", stats[i].items_processed);
        printf("Tiempo total: %.4f segundos\n", stats[i].total_time);
        
        if (stats[i].items_processed > 0) {
            double avg_time = stats[i].total_time / TICKS * 1000;  // ms por tick
            double throughput = stats[i].items_processed / stats[i].total_time;
            
            printf("Tiempo promedio por tick: %.4f ms\n", avg_time);
            printf("Tiempo mínimo por tick: %.4f ms\n", stats[i].min_time * 1000);
            printf("Tiempo máximo por tick: %.4f ms\n", stats[i].max_time * 1000);
            printf("Throughput: %.0f items/segundo\n", throughput);
        }
    }
    
    printf("\n--- PIPELINE COMPLETO ---\n");
    printf("Ticks completados: %d\n", TICKS);
    printf("Items finales procesados: %d\n", processed_count);
    
    // Calcular eficiencia del filtro
    if (stats[0].items_processed > 0) {
        double filter_efficiency = 100.0 * stats[1].items_processed / stats[0].items_processed;
        printf("Eficiencia del filtro: %.2f%% (%ld/%ld)\n", 
               filter_efficiency, stats[1].items_processed, stats[0].items_processed);
    }
}

int main(int argc, char** argv) {
    printf("Laboratorio 6 - Práctica 5: Pipeline con Barreras\n");
    
    // Crear directorio de datos si no existe
    system("mkdir -p data");
    
    // Inicializar barrera para 3 etapas
    if (pthread_barrier_init(&sync_barrier, nullptr, 3) != 0) {
        perror("Error inicializando barrera");
        return 1;
    }
    
    printf("Configuración: Pipeline de 3 etapas, %d ticks\n", TICKS);
    printf("Etapas: Generador -> Filtro -> Reducer\n\n");
    
    // Crear hilos para cada etapa
    pthread_t stage_threads[3];
    double pipeline_start = now_s();
    
    // Etapa 1: Generador
    if (pthread_create(&stage_threads[0], nullptr, stage_generator, 
                       reinterpret_cast<void*>(1)) != 0) {
        perror("Error creando hilo generador");
        cleanup_resources();
        return 1;
    }
    
    // Etapa 2: Filtro
    if (pthread_create(&stage_threads[1], nullptr, stage_filter, 
                       reinterpret_cast<void*>(2)) != 0) {
        perror("Error creando hilo filtro");
        cleanup_resources();
        return 1;
    }
    
    // Etapa 3: Reducer
    if (pthread_create(&stage_threads[2], nullptr, stage_reducer, 
                       reinterpret_cast<void*>(3)) != 0) {
        perror("Error creando hilo reducer");
        cleanup_resources();
        return 1;
    }
    
    printf("✓ Pipeline iniciado, procesando...\n\n");
    
    // Esperar que todas las etapas terminen
    for (int i = 0; i < 3; i++) {
        if (pthread_join(stage_threads[i], nullptr) != 0) {
            perror("Error esperando terminación de hilo");
        }
    }
    
    double pipeline_end = now_s();
    double total_pipeline_time = pipeline_end - pipeline_start;
    
    printf("\n✓ Pipeline completado en %.4f segundos\n", total_pipeline_time);
    
    // Imprimir estadísticas detalladas
    print_statistics();
    
    printf("\n=== ANÁLISIS DEL PIPELINE ===\n");
    printf("- Sincronización: pthread_barrier_t coordina los ticks\n");
    printf("- Inicialización: pthread_once_t garantiza init única\n");
    printf("- Throughput limitado por la etapa más lenta\n");
    printf("- Barreras vs colas: menos latencia, pero sincronización estricta\n");
    
    // Verificar que el log file se creó
    if (log_file) {
        printf("- Log detallado guardado en: data/pipeline.log\n");
    }
    
    cleanup_resources();
    return 0;
}