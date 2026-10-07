# Resumen apartados 2.4/2.5 (agente B)
Equipo: Ryzen 7 7730U, g++ 15.2.0, `taskset -c 6`, caso de referencia completo (240 imagenes), 5 ejecuciones (mediana). Checksum 1296a852f704a0a4 en TODAS (incluido -Ofast).
- -O0 40,15 s | -O1 8,23 | **-O2 6,56 (referencia)** | -O3 3,76 (1,74x vs O2; 10,7x vs O0) | -O3 -march=native 3,45 (1,90x) | -Ofast -march=native 3,31 (1,98x)
- -O3 -fno-tree-vectorize 7,61 s (peor que -O2): la ganancia de -O3 es todo vectorizacion. -O2 -ftree-vectorize ~ -O3 (tras repetir: ~3,7 s).
- -Os 12,35 s (rep movsb en resize). -funroll-loops / -mavx2 -mfma / -Ofast: dentro del ruido (~5%) de -O3.
- En -O2 GCC 15 vectoriza blur y min de Moravec; NO Sobel, umbral ni SSD de Moravec (necesitan versionado por alias). Esquinas 3193 ms (O2) -> 911 (O3).
- Antes/despues de la reestructuracion (24 imgs): O2 1780->588 ms (3,0x), O3 1617->343 (4,7x).
- perf no disponible (perf_event_paranoid=4). Deriva entre tandas 5-7 %.
Datos: resultados/compilacion_tiempos.csv, compilacion_resumen.csv, compilacion_antes_despues.csv, simd_conteo_*.csv, vectorizacion_*.txt. Scripts: scripts/compilacion.sh, compilacion_analisis.py, antes_despues.sh, simd_conteo.py.
