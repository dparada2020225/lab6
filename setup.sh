#!/bin/bash

# Universidad del Valle de Guatemala
# CC3086 Programación de Microprocesadores
# Laboratorio 6 - Script de Configuración Inicial

echo "=============================================="
echo "CONFIGURACIÓN INICIAL - LABORATORIO 6"
echo "Acceso a Recursos Compartidos con Pthreads"
echo "=============================================="

# Verificar sistema operativo
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "✅ Sistema: Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "✅ Sistema: macOS"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    echo "✅ Sistema: Windows (WSL/MSYS)"
else
    echo "⚠️  Sistema no reconocido: $OSTYPE"
fi

# Crear estructura de directorios
echo ""
echo "Creando estructura de directorios..."

directories=("bin" "data" "results" "docs")

for dir in "${directories[@]}"; do
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir"
        echo "  ✓ Creado: $dir/"
    else
        echo "  → Existe: $dir/"
    fi
done

# Hacer scripts ejecutables
echo ""
echo "Configurando permisos de scripts..."

scripts=("scripts/run_all.sh" "scripts/benchmark.sh" "scripts/analyze_results.py")

for script in "${scripts[@]}"; do
    if [ -f "$script" ]; then
        chmod +x "$script"
        echo "  ✓ Ejecutable: $script"
    else
        echo "  ⚠️  No encontrado: $script"
    fi
done

# Verificar dependencias del sistema
echo ""
echo "Verificando dependencias..."

# Compiladores
echo "Compiladores:"
if command -v gcc &> /dev/null; then
    gcc_version=$(gcc --version | head -n1)
    echo "  ✓ GCC: $gcc_version"
else
    echo "  ❌ GCC no encontrado"
fi

if command -v clang &> /dev/null; then
    clang_version=$(clang --version | head -n1)
    echo "  ✓ Clang: $clang_version"
else
    echo "  ❌ Clang no encontrado"
fi

# Make
if command -v make &> /dev/null; then
    make_version=$(make --version | head -n1)
    echo "  ✓ Make: $make_version"
else
    echo "  ❌ Make no encontrado"
fi

# Herramientas de debugging
echo ""
echo "Herramientas de debugging:"

if command -v gdb &> /dev/null; then
    gdb_version=$(gdb --version | head -n1)
    echo "  ✓ GDB: $gdb_version"
else
    echo "  ❌ GDB no encontrado"
fi

if command -v valgrind &> /dev/null; then
    valgrind_version=$(valgrind --version)
    echo "  ✓ Valgrind: $valgrind_version"
else
    echo "  ⚠️  Valgrind no encontrado (opcional)"
fi

# Python y dependencias para análisis
echo ""
echo "Python y análisis:"

if command -v python3 &> /dev/null; then
    python_version=$(python3 --version)
    echo "  ✓ Python3: $python_version"
    
    # Verificar pandas
    if python3 -c "import pandas" 2>/dev/null; then
        echo "  ✓ Pandas disponible"
    else
        echo "  ⚠️  Pandas no disponible (pip3 install pandas)"
    fi
    
    # Verificar matplotlib
    if python3 -c "import matplotlib" 2>/dev/null; then
        echo "  ✓ Matplotlib disponible"
    else
        echo "  ⚠️  Matplotlib no disponible (pip3 install matplotlib)"
    fi
else
    echo "  ❌ Python3 no encontrado"
fi

# Verificar soporte pthread
echo ""
echo "Verificando soporte pthread..."

cat > /tmp/pthread_test.cpp << 'EOF'
#include <pthread.h>
#include <iostream>
void* test_function(void* arg) {
    return nullptr;
}
int main() {
    pthread_t thread;
    if (pthread_create(&thread, nullptr, test_function, nullptr) == 0) {
        pthread_join(thread, nullptr);
        std::cout << "✓ Pthreads funcional" << std::endl;
        return 0;
    }
    std::cout << "❌ Error con pthreads" << std::endl;
    return 1;
}
EOF

if g++ -pthread /tmp/pthread_test.cpp -o /tmp/pthread_test 2>/dev/null && /tmp/pthread_test 2>/dev/null; then
    echo "  ✅ Soporte pthread verificado"
else
    echo "  ❌ Problema con soporte pthread"
fi

# Limpiar archivo temporal
rm -f /tmp/pthread_test.cpp /tmp/pthread_test

# Test de compilación básica
echo ""
echo "Probando compilación..."

if make clean > /dev/null 2>&1 && make all > /dev/null 2>&1; then
    echo "  ✅ Compilación exitosa"
    echo "  → Binarios disponibles en bin/"
else
    echo "  ❌ Error en compilación"
    echo "  → Revisar dependencias y Makefile"
fi

# Resumen final
echo ""
echo "=============================================="
echo "RESUMEN DE CONFIGURACIÓN"
echo "=============================================="

echo ""
echo "Estructura de proyecto:"
echo "  📁 include/     - Headers (.hpp)"
echo "  📁 src/         - Código fuente (.cpp)"
echo "  📁 scripts/     - Scripts de automatización"
echo "  📁 bin/         - Ejecutables compilados"
echo "  📁 data/        - Archivos de datos/logs"
echo "  📁 results/     - Resultados de benchmarks"
echo "  📁 docs/        - Documentación adicional"

echo ""
echo "Comandos principales:"
echo "  make all              - Compilar todo"
echo "  make clean            - Limpiar archivos"
echo "  ./scripts/run_all.sh  - Ejecutar todas las prácticas"
echo "  ./scripts/benchmark.sh - Benchmarks automatizados"

echo ""
echo "Para debugging:"
echo "  make tsan             - Compilar con ThreadSanitizer"
echo "  make asan             - Compilar con AddressSanitizer"
echo "  valgrind --tool=helgrind ./bin/programa"

echo ""
if [ -f "README.md" ]; then
    echo "📖 Consultar README.md para instrucciones detalladas"
else
    echo "⚠️  README.md no encontrado"
fi

echo ""
echo "✅ Configuración inicial completada"
echo "=============================================="