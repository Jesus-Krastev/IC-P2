# Resumen apartado 2.3 (escalado, -O2, taskset -c 2, 1 calentamiento + 5 medidas, mediana)
Datos: resultados/escalado_*.csv; tablas completas: resultados/escalado_resumen_tablas.md; figuras: graficas/escalado_*.png.
Scripts: scripts/escalado.py (barridos), scripts/graficas_escalado.py (gráficas/tablas).
- Referencia (240 img, 768x768): 6,36 s mediana (6,29-6,77), checksum 1296a852f704a0a4 en las 5. NO son ~9 s como decía CLAUDE.md. ~26,5 ms/imagen.
- Reparto: esquinas 49,0 %, bordes 18,9 %, lectura 11,6 %, RGB 6,9 %, combinar 5,4 %, informe 3,8 %, gris 2,4 %, hist 2,0 %.
- Camino crítico estimado (salida del programa): 4,60 s de 6,36 s -> límite funcional 1,38x.
- Lote: lineal, 26,4 ms/imagen (ajuste por el origen); 2 img 31,5 ms/img -> >=20 img 26-27 ms/img.
- Resolución 128..3072: 768->27,7 ms/img, 3072->504 ms (18,2x con 16x píxeles); ns/px: 121 (128), 49 (512), 46 (1024), 50 (1536), 53 (3072). Coste fijo de lectura domina en pequeño; efecto caché +10-20 %.
- Kernel K=3..15: bordes = 2,14 + 0,1195*K^2 ms/img; total 25,7 -> 51,7 ms; bordes 12 % -> 56 %. K=11 y 15 mismo checksum.
- Moravec R=0..4: esquinas 2,6/13,3/34,4/65,6/110,7 ms/img ~ 0,93 + 1,345*(2R+1)^2; total 17,2 -> 125,3 ms.
- NMS 0..12: sin efecto en tiempo (26,9-27,8 ms/img) aunque esquinas 191280 -> 2600.
- Umbrales: efecto <=5 % (bordes) y 11 % (esquinas), solo en combinar/dibujado.
- Checksum estable en las 5 repeticiones de todos los puntos.
- Anomalías: primera ejecución en frío (20 img) 1,0 s vs 0,51 s después; repeat=40 tiene un máximo atípico (1,39 s vs mediana 1,10 s); gobernador powersave; otro agente midiendo en paralelo.
