# Resumen agente C (secciones 4 y 5) - numeros clave
Binario: g++ -std=c++17 -Wall -Wextra -O2 -o resultados/bin/agC_O2 src/main.cpp src/pipeline.cpp ; ejecucion con taskset -c 10, caso de referencia, --save-samples 0 --output-dir /tmp/agC.
Datos brutos: grafo_ref_run{1,2,3}.txt (total 5,93 / 6,30 / 6,77 s; checksum 1296a852f704a0a4 en las 3), grafo_subetapas_run{1,2,3}.txt (scripts/agC_subetapas.cpp), grafo_time_v.txt.
Porcentajes (media 3 runs, 6336 ms): T1 12,5 | T2 6,7 | T3 2,3 | T4 18,6 (blur 64,6% / sobel 29,3% / umbral 6,1% de T4) | T5 2,0 | T6 48,6 (SSD 97,6%, NMS 2,4%) | T7 5,5 | T8 3,7.
Camino critico T1-T3-T6-T7-T8 = 72,7 % -> ganancia funcional max 1,37-1,38x.
Amdahl (estimado, f=8,6% = read_ppm 4,9% + informe 3,7%): p=8 -> 5,0x; p=16 -> 7,0x; inf -> 11,6x.
Pipeline entre imagenes: <= 1,14-1,19x.
Memoria: 17 asignaciones/imagen, 11,22 MB (10,7 MiB) reservados y puestos a cero a 768^2; puesta a cero de 576 KiB = ~20 us; RSS max 18 852 kB; 7486 fallos de pagina menores (~31/imagen).
Tamanos: original 1,69 MiB; 1 canal 576 KiB; score 2,25 MiB; acc 6 KiB. Nativo leon: 12,4 MiB RGB, score 16,56 MiB > L3.
perf: no disponible (perf_event_paranoid=4).
Nativo 10 imagenes: 1,135 s (8,8 img/s).
