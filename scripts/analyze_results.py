#!/usr/bin/env python3

"""
Universidad del Valle de Guatemala
CC3086 Programación de Microprocesadores
Laboratorio 6 - Análisis de Resultados de Benchmarks

Analiza los resultados de los benchmarks y genera estadísticas
"""

import os
import re
import sys
import statistics
from collections import defaultdict
import matplotlib.pyplot as plt
import pandas as pd

def extract_throughput(content):
    """Extrae valores de throughput de los resultados"""
    throughput_pattern = r'Throughput:\s+(\d+(?:\.\d+)?)\s+(?:ops/sec|elementos/segundo|items/segundo)'
    matches = re.findall(throughput_pattern, content, re.IGNORECASE)
    return [float(m) for m in matches]

def extract_time(content):
    """Extrae tiempos de ejecución de los resultados"""
    time_patterns = [
        r'Tiempo total:\s+(\d+\.\d+)\s+segundos',
        r'completado en\s+(\d+\.\d+)\s+segundos',
        r'Tiempo:\s+(\d+\.\d+)\s+segundos'
    ]
    
    times = []
    for pattern in time_patterns:
        matches = re.findall(pattern, content)
        times.extend([float(m) for m in matches])
    
    return times

def extract_operations(content):
    """Extrae número de operaciones completadas"""
    op_patterns = [
        r'total=(\d+)',
        r'Resultado:\s+(\d+)',
        r'Items procesados:\s+(\d+)',
        r'Operaciones completadas:\s+(\d+)'
    ]
    
    operations = []
    for pattern in op_patterns:
        matches = re.findall(pattern, content)
        operations.extend([int(m) for m in matches])
    
    return operations

def analyze_file(filepath):
    """Analiza un archivo de resultados individual"""
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Separar runs
        runs = content.split('=== RUN ')
        if len(runs) < 2:  # No hay runs separados
            runs = [content]
        else:
            runs = runs[1:]  # Remover header
        
        results = {
            'throughput': [],
            'time': [],
            'operations': [],
            'errors': 0,
            'timeouts': 0
        }
        
        for run_content in runs:
            if 'TIMEOUT' in run_content:
                results['timeouts'] += 1
                continue
            if 'ERROR' in run_content:
                results['errors'] += 1
                continue
                
            # Extraer métricas
            throughput = extract_throughput(run_content)
            time = extract_time(run_content)
            operations = extract_operations(run_content)
            
            results['throughput'].extend(throughput)
            results['time'].extend(time)
            results['operations'].extend(operations)
        
        return results
        
    except Exception as e:
        print(f"Error procesando {filepath}: {e}")
        return None

def calculate_stats(values):
    """Calcula estadísticas básicas"""
    if not values:
        return None
    
    return {
        'count': len(values),
        'mean': statistics.mean(values),
        'median': statistics.median(values),
        'std': statistics.stdev(values) if len(values) > 1 else 0,
        'min': min(values),
        'max': max(values)
    }

def generate_report(results_dir):
    """Genera reporte completo de análisis"""
    
    if not os.path.exists(results_dir):
        print(f"❌ Directorio {results_dir} no existe")
        return
    
    # Buscar archivos de resultados
    result_files = [f for f in os.listdir(results_dir) if f.startswith('p') and f.endswith('.txt')]
    
    if not result_files:
        print(f"❌ No se encontraron archivos de resultados en {results_dir}")
        return
    
    print("="*60)
    print("ANÁLISIS DE RESULTADOS - LABORATORIO 6")
    print("="*60)
    
    all_results = {}
    
    for filename in sorted(result_files):
        filepath = os.path.join(results_dir, filename)
        print(f"\nAnalizando: {filename}")
        print("-" * 40)
        
        results = analyze_file(filepath)
        if results is None:
            continue
            
        all_results[filename] = results
        
        # Mostrar estadísticas
        if results['errors'] > 0:
            print(f"⚠️  Errores: {results['errors']}")
        if results['timeouts'] > 0:
            print(f"⚠️  Timeouts: {results['timeouts']}")
        
        # Throughput
        if results['throughput']:
            stats = calculate_stats(results['throughput'])
            print(f"📊 Throughput (ops/seg):")
            print(f"   Promedio: {stats['mean']:.0f} ± {stats['std']:.0f}")
            print(f"   Rango: {stats['min']:.0f} - {stats['max']:.0f}")
        
        # Tiempo
        if results['time']:
            stats = calculate_stats(results['time'])
            print(f"⏱️  Tiempo (segundos):")
            print(f"   Promedio: {stats['mean']:.4f} ± {stats['std']:.4f}")
            print(f"   Rango: {stats['min']:.4f} - {stats['max']:.4f}")
        
        # Operaciones
        if results['operations']:
            stats = calculate_stats(results['operations'])
            print(f"🔢 Operaciones completadas:")
            print(f"   Promedio: {stats['mean']:.0f}")
            print(f"   Rango: {stats['min']:.0f} - {stats['max']:.0f}")
    
    # Generar comparativas por práctica
    print("\n" + "="*60)
    print("COMPARATIVAS POR PRÁCTICA")
    print("="*60)
    
    practices = defaultdict(list)
    for filename, results in all_results.items():
        practice = filename.split('_')[0]  # p1, p2, etc.
        practices[practice].append((filename, results))
    
    for practice, practice_results in practices.items():
        print(f"\n--- {practice.upper()} ---")
        
        # Comparar throughput entre configuraciones
        throughput_comparison = []
        for filename, results in practice_results:
            if results['throughput']:
                avg_throughput = statistics.mean(results['throughput'])
                config = filename.replace('.txt', '').replace(f'{practice}_', '')
                throughput_comparison.append((config, avg_throughput))
        
        if throughput_comparison:
            throughput_comparison.sort(key=lambda x: x[1], reverse=True)
            print("🏆 Ranking por throughput:")
            for i, (config, throughput) in enumerate(throughput_comparison[:3]):
                print(f"   {i+1}. {config}: {throughput:.0f} ops/seg")
    
    # Guardar CSV para análisis posterior
    csv_data = []
    for filename, results in all_results.items():
        if results['throughput'] and results['time']:
            csv_data.append({
                'archivo': filename,
                'practica': filename.split('_')[0],
                'throughput_promedio': statistics.mean(results['throughput']),
                'tiempo_promedio': statistics.mean(results['time']),
                'throughput_std': statistics.stdev(results['throughput']) if len(results['throughput']) > 1 else 0,
                'tiempo_std': statistics.stdev(results['time']) if len(results['time']) > 1 else 0,
                'errores': results['errors'],
                'timeouts': results['timeouts']
            })
    
    if csv_data:
        df = pd.DataFrame(csv_data)
        csv_path = os.path.join(results_dir, 'analysis_summary.csv')
        df.to_csv(csv_path, index=False)
        print(f"\n📊 Datos guardados en: {csv_path}")
    
    print("\n" + "="*60)
    print("ANÁLISIS COMPLETADO")
    print("="*60)

def plot_results(results_dir):
    """Genera gráficos de los resultados (requiere matplotlib)"""
    csv_path = os.path.join(results_dir, 'analysis_summary.csv')
    
    if not os.path.exists(csv_path):
        print("❌ Ejecutar análisis primero para generar CSV")
        return
    
    try:
        df = pd.read_csv(csv_path)
        
        # Gráfico de throughput por práctica
        plt.figure(figsize=(12, 8))
        
        practices = df['practica'].unique()
        for practice in practices:
            practice_data = df[df['practica'] == practice]
            plt.bar(practice_data['archivo'], practice_data['throughput_promedio'], 
                   label=practice, alpha=0.7)
        
        plt.title('Throughput por Configuración')
        plt.xlabel('Configuración')
        plt.ylabel('Throughput (ops/seg)')
        plt.xticks(rotation=45, ha='right')
        plt.legend()
        plt.tight_layout()
        
        plot_path = os.path.join(results_dir, 'throughput_comparison.png')
        plt.savefig(plot_path)
        print(f"📈 Gráfico guardado en: {plot_path}")
        plt.close()
        
    except ImportError:
        print("⚠️  matplotlib no disponible, omitiendo gráficos")
    except Exception as e:
        print(f"❌ Error generando gráficos: {e}")

if __name__ == "__main__":
    results_dir = "results"
    
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
    
    generate_report(results_dir)
    
    # Intentar generar gráficos
    try:
        plot_results(results_dir)
    except:
        pass  # Silently fail if plotting libraries not available