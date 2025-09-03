#!/bin/bash

# Universidad del Valle de Guatemala
# CC3086 Programación de Microprocesadores
# Laboratorio 6 - Instalación de Dependencias

echo "=============================================="
echo "INSTALACIÓN DE DEPENDENCIAS - LAB06"
echo "=============================================="

# Detectar distribución
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
else
    echo "❌ No se puede detectar la distribución"
    exit 1
fi

echo "Sistema detectado: $OS"

# Actualizar repositorios
echo ""
echo "Actualizando repositorios..."
if [[ "$OS" == *"Ubuntu"* ]] || [[ "$OS" == *"Debian"* ]]; then
    sudo apt update
elif [[ "$OS" == *"CentOS"* ]] || [[ "$OS" == *"Red Hat"* ]]; then
    sudo yum update -y
else
    echo "⚠️  Distribución no soportada automáticamente"
    echo "Instala manualmente: build-essential, gdb, valgrind, python3, pip3"
    exit 1
fi

# Instalar herramientas de compilación
echo ""
echo "Instalando herramientas de compilación..."
if [[ "$OS" == *"Ubuntu"* ]] || [[ "$OS" == *"Debian"* ]]; then
    sudo apt install -y build-essential clang gdb valgrind python3 python3-pip
elif [[ "$OS" == *"CentOS"* ]] || [[ "$OS" == *"Red Hat"* ]]; then
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y clang gdb valgrind python3 python3-pip
fi

if [ $? -ne 0 ]; then
    echo "❌ Error instalando herramientas básicas"
    exit 1
fi

echo "✅ Herramientas de compilación instaladas"

# Instalar dependencias de Python
echo ""
echo "Instalando dependencias de Python..."

# Intentar instalar pandas
echo "Instalando pandas..."
if pip3 install pandas --user; then
    echo "✅ Pandas instalado"
else
    echo "⚠️  Error instalando pandas (continuando...)"
fi

# Intentar instalar matplotlib
echo "Instalando matplotlib..."
if pip3 install matplotlib --user; then
    echo "✅ Matplotlib instalado"
else
    echo "⚠️  Error instalando matplotlib (continuando...)"
fi

# Verificar instalación
echo ""
echo "=============================================="
echo "VERIFICANDO INSTALACIÓN"
echo "=============================================="

echo "Compiladores:"
gcc --version | head -n1
clang --version | head -n1

echo ""
echo "Herramientas de debugging:"
gdb --version | head -n1
valgrind --version

echo ""
echo "Python y módulos:"
python3 --version
python3 -c "import pandas; print('✅ Pandas disponible')" 2>/dev/null || echo "❌ Pandas no disponible"
python3 -c "import matplotlib; print('✅ Matplotlib disponible')" 2>/dev/null || echo "❌ Matplotlib no disponible"

echo ""
echo "=============================================="
echo "INSTALACIÓN COMPLETADA"
echo "=============================================="
echo ""
echo "Puedes proceder con:"
echo "  make all"
echo "  ./scripts/run_all.sh"