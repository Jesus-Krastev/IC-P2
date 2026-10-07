# Resumen agente D: grifos y estimaciones (secciones 8, 9, 10)

Script: `scripts/grifos_y_estimaciones.py` (lee `resultados/referencia_final_salida.txt`; salida completa en `resultados/grifos_estimaciones_salida.txt`).
Referencia oficial: mediana 5,67 s (5,63-5,95); ejecucion representativa 5,664 s, 23,6 ms/imagen, 42,4 img/s, checksum 1296a852f704a0a4.

## Grifos (exacto)
| Caso | T | S | E |
|---|---|---|---|
| 8+40 | 20/3 h = 6 h 40 min | 6/5 | 3/5 |
| 8+8 | 4 h | 2 | 1 |
| 40+40 | 20 h | 2 | 1 |
| 8+40+40 | 40/7 h = 5 h 43 min | 7/5 | 7/15 |

## Medido
Esquinas 48,8 %, bordes 19,1 %, lectura 11,5 %, RGB 6,9 %, combinar 5,4 %, informe 3,8 %, gris 2,4 %, histograma+Otsu 2,0 %. Limite funcional 1,39x.
f1 = 3,88 % (informe+muestras), f2 = 15,42 % (+lectura).

## ESTIMADO (Amdahl)
p=2/4/8/16(SMT): f1 -> 1,93 / 3,58 / 6,29 / 7,41 ; f2 -> 1,73 / 2,73 / 3,85 / 4,19. Limites 25,8x y 6,5x. Pipeline 1 hebra lectora <= 8,7x.
GPU g=20: 11,5x (f1) / 5,1x (f2). Cluster (t_fijo supuesto 0,25 s) 2/4/8/16 nodos: 7,0 / 8,9 / 10,2 / 11,1x. 8 hilos: 5,66 s -> ~0,90 s.
SUPUESTOS: SMT +25 %, t_fijo, PCIe 12 GB/s, g, r.
