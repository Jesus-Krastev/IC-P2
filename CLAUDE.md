# CLAUDE.md — Práctica 2: Estudio y propuesta de paralelización de una aplicación

**Asignatura:** Ingeniería de los Computadores — Curso 2026-27
**Equipo:** Jesús (jesuskrastev@gmail.com) y Pablo
**Enunciado original:** [`Práctica 2.pdf`](./Práctica%202.pdf) (en esta misma carpeta)

## ⚠️ Instrucción obligatoria para Claude

**Antes de ayudar con cualquier tarea de esta práctica** —elegir la aplicación, escribir o
revisar código, diseñar experimentos, generar gráficas, redactar la memoria, resolver la
Tarea 3, preparar la entrega, etc.— **lee primero `Práctica 2.pdf`** en esta carpeta.

Motivo: este `CLAUDE.md` es un resumen operativo hecho por Claude para organizar el trabajo,
**no el enunciado oficial**. Puede quedar desactualizado o simplificar matices importantes
(fechas, requisitos exactos, formato de entrega). Ante cualquier duda o discrepancia, **manda
el PDF**.

Después de leer el PDF:
1. Revisa la sección **"Checklist de progreso"** para saber qué está hecho y qué falta.
2. **Actualiza el checklist** (marca `[x]`, añade detalles en "Notas y decisiones") en cuanto
   se complete, cambie o se decida algo relevante. Este archivo debe reflejar siempre el
   estado real del proyecto, para que Jesús, Pablo o una futura sesión de Claude puedan
   retomarlo sin releer todo el historial de chat.
3. Si Jesús o Pablo piden trabajar en "su parte", consulta la sección **"Reparto de
   tareas"** para saber qué le corresponde a cada uno.

---

## 1. Resumen de la práctica

Hay que elegir una aplicación secuencial (en C o C++) con coste computacional apreciable,
**estudiarla a fondo y proponer —sin implementarla todavía— cómo se podría paralelizar**.
La implementación paralela real se hará en una práctica posterior; **aquí solo se analiza y
se propone**.

Objetivos concretos del enunciado:
- Elegir o diseñar un problema con carga computacional real (caso de prueba de referencia de
  segundos a decenas de segundos de ejecución secuencial; nada de micro/nanosegundos).
- Identificar las tareas principales del programa y las dependencias entre ellas (grafo de
  dependencias).
- Distinguir paralelismo de **datos** (misma operación aplicada a partes distintas de los
  datos) y paralelismo **funcional** (tareas distintas que podrían solaparse si sus
  dependencias lo permiten).
- Medir cómo escala el tiempo de ejecución al variar el tamaño/complejidad del problema.
- Medir el efecto de varias opciones de compilación de GCC y buscar evidencias de
  autovectorización (instrucciones SIMD).
- Proponer una arquitectura paralela razonada (multinúcleo, GPU, clúster, sistema
  heterogéneo...) y estimar qué ganancia y eficiencia cabría esperar.
- Resolver 4 problemas teóricos de ganancia/eficiencia (los "grifos llenando un depósito",
  Tarea 3 del enunciado).

## 2. Qué hay que entregar

- Una **memoria estructurada** con 11 apartados obligatorios (ver checklist más abajo).
- El **código fuente** de la aplicación secuencial (propio o adaptado, citando la
  procedencia y las modificaciones si se parte de un proyecto externo).
- Un **Makefile** que soporte, como mínimo:
  - `make` → compila el proyecto.
  - `make run` → ejecuta la aplicación con los parámetros del caso de prueba de referencia.
  - `make clean` → elimina los archivos generados por la compilación.
- Entrega vía **UACloud**, antes de la sesión de la semana del **12 de octubre de 2026**.
  ⚠️ El grupo de **viernes** entrega más tarde por el festivo del 9 de octubre — confirmar la
  fecha y hora exactas en la actividad de entrega de UACloud.
- Hay **3 sesiones de prácticas** para todo el trabajo. Al final de la **2ª sesión** hay que
  comunicar en Moodle/UACloud el título, una breve descripción del problema elegido y la
  lista definitiva de integrantes del equipo.
- El trabajo debe ser original; si se reutiliza código o fuentes externas, hay que citarlas y
  explicar qué se ha aportado o modificado (normas de integridad académica de la UA).

## 3. Estructura sugerida del repositorio

Ahora mismo la carpeta solo tiene el PDF. Se sugiere ir organizando así (ajustar si la app
elegida lo pide):

```
Practica 2/
├── Práctica 2.pdf     # enunciado oficial (no modificar)
├── CLAUDE.md           # este archivo
├── src/                # código fuente C/C++
├── Makefile             # make / make run / make clean
├── resultados/          # csv/salidas de mediciones, logs de compilación, asm generado
├── graficas/             # scripts (gnuplot/python/...) y figuras generadas
└── memoria/              # fuente de la memoria + export final
```

## 4. Reparto de tareas

El reparto está hecho directamente en el **checklist** (sección 5): cada punto lleva la
etiqueta **(Jesús)**, **(Pablo)** o **(Jesús + Pablo)** indicando quién es responsable. Pensado
para que la carga sea equivalente; las decisiones que afectan a todo el trabajo (elección de
la app, interpretación final, propuesta de arquitectura, conclusiones) van etiquetadas como
conjuntas. Si prefieren repartirlo de otra forma, que se lo digan a Claude y que actualice las
etiquetas.

## 5. Checklist de progreso

Ir marcando según se complete cada punto.

### Preparación
- [x] Elegir la aplicación a paralelizar (Jesús + Pablo) — pipeline de procesamiento de
      imágenes (detección de bordes tipo Sobel/Canny simplificado); ver detalles en
      "Notas y decisiones"
- [ ] Comentar la elección con el profesor/a (Jesús + Pablo) — **pendiente, hacerlo antes de
      avanzar mucho más en el análisis**, tal como pide el PDF
- [ ] Definir el caso de prueba de referencia (parámetros + tiempo objetivo: segundos a
      decenas de segundos) (Jesús + Pablo)
- [ ] Comunicar título + descripción + integrantes en Moodle/UACloud (fin 2ª sesión) (Jesús + Pablo)

### Código
- [ ] Implementación secuencial en C/C++ (propia o adaptada, citando procedencia) (Jesús + Pablo)
- [ ] Ejecución no interactiva (parámetros al inicio, resultados al terminar) (Jesús + Pablo)
- [ ] Makefile: `make`, `make run`, `make clean` (Jesús)

### Tarea 2 — Análisis de la aplicación secuencial
- [ ] 2.1 Grafo de dependencias entre tareas (Jesús)
- [ ] 2.2 Estudio de datos y accesos a memoria (caché, accesos compartidos) (Pablo)
- [ ] 2.3 Escalado: tiempo de ejecución vs. tamaño del problema (gráficas) (Jesús)
- [ ] 2.4 Opciones de compilación + evidencias de autovectorización (SIMD) (Pablo)
- [ ] 2.5 Comparación de configuraciones de compilación (gráficas + ganancia) (Pablo)
- [ ] 2.6 Interpretación conjunta del rendimiento (Jesús + Pablo)

### Tarea 3 — Problemas de los grifos
- [ ] Problema 1 (grifo 8h + grifo 40h a la vez) (Jesús)
- [ ] Problema 2 (dos grifos de 8h) (Jesús)
- [ ] Problema 3 (dos grifos de 40h) (Pablo)
- [ ] Problema 4 (grifo 8h + dos grifos de 40h) (Pablo)

### Tarea 4 — Memoria final (11 apartados obligatorios)
- [ ] 1. Objetivos y descripción del problema elegido (Jesús)
- [ ] 2. Procedencia del código y modificaciones realizadas (Pablo)
- [ ] 3. Descripción de la aplicación y del caso de prueba de referencia (Jesús)
- [ ] 4. Grafo de dependencias y análisis de paralelismo de datos/funcional (Jesús)
- [ ] 5. Estudio de los datos y accesos a memoria (Pablo)
- [ ] 6. Metodología de medición, tablas, gráficas y análisis del rendimiento secuencial (Jesús)
- [ ] 7. Opciones de compilación probadas y evidencias de autovectorización (Pablo)
- [ ] 8. Justificación de por qué la aplicación es buena candidata a paralelización (Jesús + Pablo)
- [ ] 9. Propuesta razonada de arquitectura(s) paralela(s) (Jesús + Pablo)
- [ ] 10. Resolución de los problemas de la Tarea 3 (Jesús + Pablo)
- [ ] 11. Conclusiones y bibliografía (Jesús + Pablo)

### Entrega
- [ ] Empaquetar memoria + código fuente + Makefile (Jesús)
- [ ] Confirmar fecha/hora y formato exactos en la actividad de UACloud (ojo grupo viernes) (Jesús + Pablo)
- [ ] Entregar en UACloud (Jesús)

## 6. Notas y decisiones

*(Ir rellenando aquí a medida que se tomen decisiones importantes. Claude debe añadir una
línea cada vez que se fije algo relevante: aplicación elegida y por qué, caso de prueba de
referencia, entorno de medición, versión de GCC, etc.)*

- **Aplicación elegida (2026-09-23):** pipeline de procesamiento de imágenes — cadena de
  filtros tipo detección de bordes (escala de grises → suavizado gaussiano → gradiente
  Sobel → umbralización, similar a un Canny simplificado). Motivo: dentro de las ideas
  barajadas (N-cuerpos, este pipeline, difusión de calor/Jacobi, ray tracing, alineamiento
  de secuencias), esta fue la elegida por dar el grafo de dependencias más claro para 2.1
  (etapas con semántica distinta, no solo "más de lo mismo") y el análisis de accesos a
  memoria/caché más rico para 2.2 (convolución con ventanas deslizantes 2D, gestión de
  bordes, posible cache blocking).
  ⚠️ **Importante para el diseño:** para que haya paralelismo funcional real (no solo de
  datos) el caso de prueba de referencia debe procesar **un lote de varias imágenes o un
  vídeo (varios frames)**, no una imagen suelta — con una sola imagen la cadena de etapas
  es puramente secuencial y no hay nada que solape.
  - Implementación de E/S de imagen: usar `stb_image.h` / `stb_image_write.h` (single-header,
    dominio público/MIT) — citar la procedencia en el apartado 2 de la memoria.
  - **Pendiente todavía:** comentar la elección con el profesor/a (ver checklist), y definir
    los parámetros exactos del caso de prueba de referencia (resolución, nº de imágenes/frames,
    kernels usados).
- Caso de prueba de referencia: _pendiente_ (definir tamaño de imagen + nº de frames/lote una
  vez confirmada la elección con el profesor)
- Entorno de compilación/medición (CPU, versión de GCC, flags base): _pendiente_
- Control de versiones: repositorio git conectado a
  [github.com/Jesus-Krastev/IC-P2](https://github.com/Jesus-Krastev/IC-P2) (rama `main`).

## 7. Recursos útiles (citados en el enunciado)

- Documentación oficial de GCC, `gcc --help=optimizers` y `gcc --help=target` (o sus
  equivalentes en `g++`) para las opciones de compilación y autovectorización.
- Jornadas SARTECO (jornadassarteco.org) y, en particular, las Jornadas de Paralelismo y las
  Jornadas de Computación Empotrada y Reconfigurable, como fuente de ideas de aplicaciones.
