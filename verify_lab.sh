#!/bin/bash

echo "=============================================="
echo "VERIFICACIÓN RÁPIDA - LABORATORIO 6"
echo "=============================================="

# Compilar solo lo esencial
echo "1. Compilando programas..."
make clean
make all

if [ $? -eq 0 ]; then
    echo "✅ Compilación exitosa"
else
    echo "❌ Error de compilación"
    exit 1
fi

# Verificar binarios
echo ""
echo "2. Verificando binarios..."
for prog in p1_counter p2_ring p3_rw p4_deadlock p5_pipeline; do
    if [ -x "bin/$prog" ]; then
        echo "  ✅ bin/$prog"
    else
        echo "  ❌ bin/$prog faltante"
    fi
done

# Pruebas rápidas
echo ""
echo "3. Pruebas funcionales rápidas..."

echo -n "  P1 (Counter): "
if timeout 5 ./bin/p1_counter 2 1000 1 >/dev/null 2>&1; then
    echo "✅"
else
    echo "❌"
fi

echo -n "  P2 (Ring): "
if timeout 5 ./bin/p2_ring 1 1 1000 >/dev/null 2>&1; then
    echo "✅"
else
    echo "❌"
fi

echo -n "  P3 (RW): "
if timeout 5 ./bin/p3_rw 2 1000 >/dev/null 2>&1; then
    echo "✅"
else
    echo "❌"
fi

echo -n "  P4 (Deadlock - orden): "
if timeout 5 ./bin/p4_deadlock 2 >/dev/null 2>&1; then
    echo "✅"
else
    echo "❌"
fi

echo -n "  P5 (Pipeline): "
if timeout 10 ./bin/p5_pipeline >/dev/null 2>&1; then
    echo "✅"
else
    echo "❌"
fi

echo ""
echo "=============================================="
echo "VERIFICACIÓN COMPLETADA"
echo "=============================================="