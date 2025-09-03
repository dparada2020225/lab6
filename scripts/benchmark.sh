#!/bin/bash

# Universidad del Valle de Guatemala
# CC3086 Programación de Microprocesadores
# Laboratorio 6 - Benchmarking Automatizado

RESULTS_DIR="results"
RUNS=5  # Número de repeticiones por configuración

mkdir -p $RESULTS_DIR

echo "=============================================="
echo "BENCHMARKING AUTOMATIZADO - LAB06"
echo "Repeticiones por configuración: $RUNS"
echo "=============================================="

# Función para ejecutar benchmark y guardar resultados
run_benchmark() {
    local program=$1
    local config=$2
    local output_file=$3
    local description=$4
    
    echo "Benchmarking: $description"
    echo "Configuración: $config"
    echo "Archivo de salida: $output_file"
    echo "----------------------------------------"
    
    echo "# $description" > "$RESULTS_DIR/$output_file"
    echo "# Configuración: $config" >> "$RESULTS_DIR/$output_file"
    echo "# Fecha: $(date)" >> "$RESULTS_DIR/$output_file"
    echo "# Sistema: $(uname -a)" >> "$RESULTS_DIR/$output_file"
    echo "# CPU: $(lscpu | grep 'Model name' | cut -d':' -f2 | xargs)" >> "$RESULTS_DIR/$output_file"
    echo "" >> "$RESULTS_DIR/$output_file"
    
    for run in $(seq 1 $RUNS); do
        echo "  Ejecutión $run/$RUNS..."
        echo "=== RUN $run ===" >> "$RESULTS_DIR/$output_file"
        
        timeout 120 $program $config >> "$RESULTS_DIR/$output_file" 2>&1
        local exit_code=$?
        
        if [ $exit_code -eq 124 ]; then
            echo "TIMEOUT en run $run" >> "$RESULTS_DIR/$output_file"
        elif [ $exit_code -ne 0 ]; then
            echo "ERROR (código $exit_code) en run $run" >> "$RESULTS_DIR/$output_file"
        fi
        
        echo "" >> "$RESULTS_DIR/$output_file"
        sleep 1  # Pausa entre ejecuciones
    done
    
    echo "✅ Benchmark completado: $output_file"
    echo ""
}

# Compilar programas
echo "Compilando programas..."
make clean && make all

if [ $? -ne 0 ]; then
    echo "❌ Error en compilación"
    exit 1
fi

# BENCHMARK 1: Counter Race Conditions
echo "BENCHMARK 1: Counter Race Conditions"
echo "====================================="

# Diferentes números de hilos
for threads in 1 2 4 8; do
    iterations=1000000
    run_benchmark "./bin/p1_counter" "$threads $iterations 1" \
        "p1_counter_t${threads}.txt" \
        "P1 Counter: $threads hilos, $iterations iteraciones"
done

# Diferentes cargas de trabajo
for load in 500000 1000000 2000000; do
    threads=4
    run_benchmark "./bin/p1_counter" "$threads $load 1" \
        "p1_counter_load${load}.txt" \
        "P1 Counter: $threads hilos, $load iteraciones"
done

# BENCHMARK 2: Buffer Circular
echo "BENCHMARK 2: Buffer Circular"
echo "============================"

# Configuraciones balanceadas
configs=(
    "1 1 100000"
    "2 2 100000" 
    "4 4 50000"
    "2 1 100000"
    "1 2 100000"
    "4 2 50000"
    "2 4 50000"
)

for config in "${configs[@]}"; do
    producers=$(echo $config | cut -d' ' -f1)
    consumers=$(echo $config | cut -d' ' -f2)
    items=$(echo $config | cut -d' ' -f3)
    
    run_benchmark "./bin/p2_ring" "$config" \
        "p2_ring_p${producers}c${consumers}i${items}.txt" \
        "P2 Ring: ${producers}P/${consumers}C, $items items/producer"
done

# BENCHMARK 3: Lectores/Escritores
echo "BENCHMARK 3: Lectores/Escritores"
echo "================================"

# Diferentes números de hilos
for threads in 2 4 6 8; do
    operations=50000
    run_benchmark "./bin/p3_rw" "$threads $operations" \
        "p3_rw_t${threads}.txt" \
        "P3 RW Lock: $threads hilos, $operations ops/hilo"
done

# BENCHMARK 4: Deadlock Solutions (solo soluciones seguras)
echo "BENCHMARK 4: Deadlock Solutions"
echo "==============================="

run_benchmark "./bin/p4_deadlock" "2" \
    "p4_deadlock_ordered.txt" \
    "P4 Deadlock: Solución con orden total"

run_benchmark "./bin/p4_deadlock" "3" \
    "p4_deadlock_trylock.txt" \
    "P4 Deadlock: Solución con trylock"

# BENCHMARK 5: Pipeline
echo "BENCHMARK 5: Pipeline"
echo "===================="

run_benchmark "./bin/p5_pipeline" "" \
    "p5_pipeline.txt" \
    "P5 Pipeline: 3 etapas con barreras"

# Generar resumen
echo "=============================================="
echo "GENERANDO RESUMEN DE RESULTADOS"
echo "=============================================="

SUMMARY_FILE="$RESULTS_DIR/benchmark_summary.txt"
echo "RESUMEN DE BENCHMARKS - LAB06" > $SUMMARY_FILE
echo "Fecha: $(date)" >> $SUMMARY_FILE
echo "Sistema: $(uname -a)" >> $SUMMARY_FILE
echo "CPU: $(lscpu | grep 'Model name' | cut -d':' -f2 | xargs)" >> $SUMMARY_FILE
echo "Repeticiones por configuración: $RUNS" >> $SUMMARY_FILE
echo "" >> $SUMMARY_FILE

echo "ARCHIVOS GENERADOS:" >> $SUMMARY_FILE
echo "==================" >> $SUMMARY_FILE
ls -la $RESULTS_DIR/*.txt >> $SUMMARY_FILE

echo "" >> $SUMMARY_FILE
echo "ESTADÍSTICAS RÁPIDAS:" >> $SUMMARY_FILE
echo "====================" >> $SUMMARY_FILE

# Contar éxitos y fallos
total_files=$(ls $RESULTS_DIR/p*.txt 2>/dev/null | wc -l)
echo "Total de archivos de benchmark: $total_files" >> $SUMMARY_FILE

# Buscar timeouts y errores
timeout_count=$(grep -l "TIMEOUT" $RESULTS_DIR/p*.txt 2>/dev/null | wc -l)
error_count=$(grep -l "ERROR" $RESULTS_DIR/p*.txt 2>/dev/null | wc -l)

echo "Archivos con TIMEOUT: $timeout_count" >> $SUMMARY_FILE
echo "Archivos con ERROR: $error_count" >> $SUMMARY_FILE

if [ $timeout_count -gt 0 ]; then
    echo "Archivos con timeout:" >> $SUMMARY_FILE
    grep -l "TIMEOUT" $RESULTS_DIR/p*.txt 2>/dev/null >> $SUMMARY_FILE
fi

if [ $error_count -gt 0 ]; then
    echo "Archivos con errores:" >> $SUMMARY_FILE
    grep -l "ERROR" $RESULTS_DIR/p*.txt 2>/dev/null >> $SUMMARY_FILE
fi

echo "" >> $SUMMARY_FILE
echo "Para análisis detallado, revisar archivos individuales en $RESULTS_DIR/" >> $SUMMARY_FILE

echo "✅ Benchmarking completado"
echo "📁 Resultados guardados en: $RESULTS_DIR/"
echo "📄 Resumen general en: $SUMMARY_FILE"
echo ""
echo "Archivos generados:"
ls -la $RESULTS_DIR/*.txt