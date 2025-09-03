#!/bin/bash

# Universidad del Valle de Guatemala
# CC3086 Programación de Microprocesadores
# Laboratorio 6 - Script de Ejecución Completa

echo "=============================================="
echo "Lab06: Acceso a Recursos Compartidos Pthreads"
echo "=============================================="

# Crear directorios necesarios
mkdir -p bin data results

# Compilar todos los programas
echo "Compilando programas..."
make clean
make all

if [ $? -ne 0 ]; then
    echo "❌ Error en compilación"
    exit 1
fi

echo "✅ Compilación exitosa"
echo ""

# Función para ejecutar con timeout
run_with_timeout() {
    local cmd=$1
    local timeout=$2
    local name=$3
    
    echo "Ejecutando: $name"
    echo "Comando: $cmd"
    echo "Timeout: ${timeout}s"
    echo "----------------------------------------"
    
    timeout ${timeout} $cmd
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "⚠️  TIMEOUT: $name excedió ${timeout} segundos"
    elif [ $exit_code -ne 0 ]; then
        echo "❌ ERROR: $name terminó con código $exit_code"
    else
        echo "✅ ÉXITO: $name completado"
    fi
    
    echo ""
    echo "=========================================="
    echo ""
}

# PRÁCTICA 1: Race Conditions en Contador
echo "PRÁCTICA 1: Race Conditions en Contador"
echo "========================================"

echo "Probando con diferentes configuraciones..."
run_with_timeout "./bin/p1_counter 2 500000 1" 30 "P1: 2 hilos, 500k ops"
run_with_timeout "./bin/p1_counter 4 1000000 1" 60 "P1: 4 hilos, 1M ops"
run_with_timeout "./bin/p1_counter 8 500000 1" 60 "P1: 8 hilos, 500k ops"

# PRÁCTICA 2: Buffer Circular
echo "PRÁCTICA 2: Buffer Circular Productor/Consumidor"
echo "==============================================="

run_with_timeout "./bin/p2_ring 1 1 50000" 30 "P2: 1P/1C, 50k items"
run_with_timeout "./bin/p2_ring 2 2 100000" 45 "P2: 2P/2C, 100k items"
run_with_timeout "./bin/p2_ring 3 1 75000" 45 "P2: 3P/1C, 75k items"
run_with_timeout "./bin/p2_ring 1 3 75000" 45 "P2: 1P/3C, 75k items"

# PRÁCTICA 3: Lectores/Escritores
echo "PRÁCTICA 3: Lectores/Escritores"
echo "==============================="

run_with_timeout "./bin/p3_rw 4 25000" 60 "P3: 4 hilos, 25k ops"
run_with_timeout "./bin/p3_rw 6 20000" 60 "P3: 6 hilos, 20k ops"

# PRÁCTICA 4: Deadlock (con precaución)
echo "PRÁCTICA 4: Deadlock y Corrección"
echo "================================="

echo "⚠️  ADVERTENCIA: La demostración de deadlock puede colgarse"
echo "Ejecutando solo las soluciones seguras..."

run_with_timeout "./bin/p4_deadlock 2" 15 "P4: Solución con orden total"
run_with_timeout "./bin/p4_deadlock 3" 30 "P4: Solución con trylock"

# PRÁCTICA 5: Pipeline
echo "PRÁCTICA 5: Pipeline con Barreras"
echo "================================="

run_with_timeout "./bin/p5_pipeline" 120 "P5: Pipeline 3 etapas"

echo "=============================================="
echo "EJECUCIÓN COMPLETA TERMINADA"
echo "=============================================="
echo ""
echo "Archivos generados:"
echo "- data/pipeline.log (si P5 ejecutó correctamente)"
echo "- Binarios en bin/"
echo ""
echo "Para ejecutar prácticas individuales:"
echo "  ./bin/p1_counter [hilos] [iteraciones] [repeticiones]"
echo "  ./bin/p2_ring [productores] [consumidores] [items_por_productor]"
echo "  ./bin/p3_rw [hilos] [operaciones_por_hilo]"
echo "  ./bin/p4_deadlock [1=demo|2=orden|3=trylock|0=todo]"
echo "  ./bin/p5_pipeline"
echo ""
echo "Para versiones con sanitizers:"
echo "  make tsan  # ThreadSanitizer"
echo "  make asan  # AddressSanitizer"