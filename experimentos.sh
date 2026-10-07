#!/usr/bin/env bash
# experimentos.sh - Mediciones de los apartados 2.3, 2.4 y 2.5 de la Practica 2.
#
# Uso (desde la carpeta del proyecto, donde estan Makefile, src/ e images/):
#   bash experimentos.sh
#
# Variables opcionales:
#   CXX=g++-14 bash experimentos.sh     compilador a usar (en el lab: g++)
#   REPS=3                              repeticiones por medida (se toma la mediana)
#   IMG=leon.ppm                        imagen de images/ que se usa
#
# Genera la carpeta resultados/ con CSV, logs e informes de vectorizacion,
# e imprime al final las tablas listas para pegar en la memoria.

set -u
# Formato numerico con punto decimal: con un locale espanol, awk escribe 0,304 y
# rompe los CSV separados por comas.
export LC_ALL=C
CXX="${CXX:-g++}"
REPS="${REPS:-3}"
IMG="${IMG:-leon.ppm}"
RES_LIST="${RES_LIST:-512 1024 1536 2048}"   # lados de imagen cuadrada (2.3)
RES_REPEAT="${RES_REPEAT:-2}"                 # imagenes por medida en el barrido de resolucion
N_LIST="${N_LIST:-1 2 4 8}"                   # numero de imagenes (2.3)
N_RES="${N_RES:-1024}"                        # resolucion fija del barrido de imagenes
REF_REPEAT="${REF_REPEAT:-4}"                 # caso de referencia: IMG x REF_REPEAT, resolucion nativa

BASE_FLAGS="-std=c++17 -Wall -Wextra"
SRC="src/main.cpp src/pipeline.cpp"
OUT=resultados
mkdir -p "$OUT/bin" "$OUT/logs" "$OUT/informes"

if [ ! -f "images/$IMG" ]; then
    echo "No existe images/$IMG. Ejecutad el script desde la carpeta del proyecto." >&2
    exit 1
fi

if "$CXX" --version 2>/dev/null | grep -qi clang; then
    IS_CLANG=1
    echo "AVISO: '$CXX' es Clang (en macOS, g++ es Apple Clang). El enunciado pide GCC;"
    echo "       para el apartado 2.4 ejecutad este script en el Linux del laboratorio."
else
    IS_CLANG=0
fi
echo "Compilador: $("$CXX" --version | head -1)"
echo "Maquina:    $(uname -sm)"
echo

# compila con unos flags; devuelve 1 si el compilador no los acepta
build() { # $1 = nombre, $2 = flags
    "$CXX" $BASE_FLAGS $2 -o "$OUT/bin/$1" $SRC 2> "$OUT/logs/build_$1.txt"
}

# ejecuta REPS veces y escribe la mediana del TOTAL en segundos
median_total() { # $1 = binario, resto = argumentos
    local bin=$1; shift
    local vals=""
    # una ejecucion de calentamiento que se descarta (cache de disco, frecuencia de la CPU)
    "$bin" "$@" --save-samples 0 --output-dir "$OUT/tmp" > /dev/null
    for _ in $(seq "$REPS"); do
        local ms
        ms=$("$bin" "$@" --save-samples 0 --output-dir "$OUT/tmp" | awk '/TOTAL/ {print $5}')
        vals="$vals$ms"$'\n'
    done
    printf '%s' "$vals" | sort -n | sed -n "$(( (REPS + 1) / 2 ))p" | awk '{printf "%.3f", $1 / 1000}'
}

# ---------------------------------------------------------------------
# 2.3 Tamano del problema (compilacion de referencia, sin -O)
# ---------------------------------------------------------------------
echo "== 2.3 Tamano del problema =="
build O0 "" || { echo "Fallo al compilar, ver $OUT/logs/build_O0.txt"; exit 1; }

echo "resolucion,mpx_por_imagen,imagenes,tiempo_s,s_por_mpx" > "$OUT/escalado_resolucion.csv"
for r in $RES_LIST; do
    t=$(median_total "$OUT/bin/O0" --images "$IMG" --repeat "$RES_REPEAT" --resize-width "$r" --resize-height "$r")
    mpx=$(awk -v r="$r" 'BEGIN {printf "%.2f", r * r / 1e6}')
    spm=$(awk -v t="$t" -v m="$mpx" -v n="$RES_REPEAT" 'BEGIN {printf "%.3f", t / (m * n)}')
    echo "${r}x${r},$mpx,$RES_REPEAT,$t,$spm" >> "$OUT/escalado_resolucion.csv"
    echo "  ${r}x${r}: $t s"
done

echo "imagenes,resolucion,tiempo_s,s_por_imagen" > "$OUT/escalado_imagenes.csv"
for n in $N_LIST; do
    t=$(median_total "$OUT/bin/O0" --images "$IMG" --repeat "$n" --resize-width "$N_RES" --resize-height "$N_RES")
    spi=$(awk -v t="$t" -v n="$n" 'BEGIN {printf "%.3f", t / n}')
    echo "$n,${N_RES}x${N_RES},$t,$spi" >> "$OUT/escalado_imagenes.csv"
    echo "  $n imagenes: $t s"
done
echo

# ---------------------------------------------------------------------
# 2.4 / 2.5 Opciones de compilacion (caso de referencia)
# ---------------------------------------------------------------------
echo "== 2.4 Opciones de compilacion (caso de referencia: $IMG x $REF_REPEAT, resolucion nativa) =="
NAMES="O0 O1 O2 O3 O3_native Ofast_native"
flags_of() {
    case $1 in
        O0) echo "" ;;
        O1) echo "-O1" ;;
        O2) echo "-O2" ;;
        O3) echo "-O3" ;;
        O3_native) echo "-O3 -march=native" ;;
        Ofast_native) echo "-Ofast -march=native" ;;
    esac
}

echo "configuracion,flags,tiempo_s,ganancia_vs_O0,moravec_ms,bordes_ms" > "$OUT/compilacion.csv"
T0=""
for name in $NAMES; do
    f=$(flags_of "$name")
    if ! build "$name" "$f"; then
        echo "  $name: el compilador no acepta '$f' (se omite)"
        continue
    fi
    t=$(median_total "$OUT/bin/$name" --images "$IMG" --repeat "$REF_REPEAT")
    "$OUT/bin/$name" --images "$IMG" --repeat "$REF_REPEAT" --save-samples 0 --output-dir "$OUT/tmp" > "$OUT/logs/run_$name.txt"
    mor=$(awk -F: '/esquinas/ {split($2, a, " "); print a[1]}' "$OUT/logs/run_$name.txt")
    bor=$(awk -F: '/bordes/ {split($2, a, " "); print a[1]}' "$OUT/logs/run_$name.txt")
    [ -z "$T0" ] && T0=$t
    g=$(awk -v a="$T0" -v b="$t" 'BEGIN {printf "%.2f", a / b}')
    echo "$name,${f:-(sin -O)},$t,$g,$mor,$bor" >> "$OUT/compilacion.csv"
    echo "  $name (${f:-sin -O}): $t s  ->  ganancia $g"
done
echo

# Comprobacion: -Ofast reordena coma flotante; las salidas deben coincidir con -O0
if [ -x "$OUT/bin/Ofast_native" ]; then
    "$OUT/bin/O0" --images "$IMG" --repeat 1 --save-samples 1 --output-dir "$OUT/salida_O0" > /dev/null
    "$OUT/bin/Ofast_native" --images "$IMG" --repeat 1 --save-samples 1 --output-dir "$OUT/salida_Ofast" > /dev/null
    if diff -rq "$OUT/salida_O0" "$OUT/salida_Ofast" > "$OUT/logs/diff_O0_Ofast.txt"; then
        echo "Salidas de -O0 y -Ofast identicas."
    else
        echo "Salidas de -O0 y -Ofast DISTINTAS, ver $OUT/logs/diff_O0_Ofast.txt"
    fi
fi

# Informe de vectorizacion de pipeline.cpp
if [ "$IS_CLANG" = 1 ]; then
    "$CXX" $BASE_FLAGS -O3 -c src/pipeline.cpp -o "$OUT/bin/pipeline.o" \
        -Rpass=loop-vectorize -Rpass-missed=loop-vectorize 2> "$OUT/informes/vectorizacion.txt"
    NVEC=$(grep -c "remark: vectorized" "$OUT/informes/vectorizacion.txt")
else
    "$CXX" $BASE_FLAGS -O3 -march=native -c src/pipeline.cpp -o "$OUT/bin/pipeline.o" \
        -fopt-info-vec-optimized="$OUT/informes/vec_optimizados.txt"
    "$CXX" $BASE_FLAGS -O3 -march=native -c src/pipeline.cpp -o "$OUT/bin/pipeline.o" \
        -fopt-info-vec-missed="$OUT/informes/vec_no_optimizados.txt"
    NVEC=$(grep -c "optimized" "$OUT/informes/vec_optimizados.txt")
    objdump -d --no-show-raw-insn "$OUT/bin/O3_native" > "$OUT/informes/ensamblador_O3_native.txt" 2>/dev/null
fi
echo "Bucles vectorizados en pipeline.cpp (-O3): $NVEC  (detalle en $OUT/informes/)"
echo

# ---------------------------------------------------------------------
# Tablas para pegar en la memoria
# ---------------------------------------------------------------------
echo "================ TABLAS PARA LA MEMORIA ================"
echo
echo "| Resolucion | Mpx por imagen | Imagenes | Tiempo total (s) | Tiempo por Mpx (s) |"
echo "| --- | --- | --- | --- | --- |"
tail -n +2 "$OUT/escalado_resolucion.csv" | awk -F, '{printf "| %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5}'
echo
echo "| Imagenes (${N_RES}x${N_RES}) | Tiempo total (s) | Tiempo por imagen (s) |"
echo "| --- | --- | --- |"
tail -n +2 "$OUT/escalado_imagenes.csv" | awk -F, '{printf "| %s | %s | %s |\n", $1, $3, $4}'
echo
echo "| Configuracion | Tiempo total (s) | Ganancia vs -O0 | Moravec (ms) | Bordes (ms) |"
echo "| --- | --- | --- | --- | --- |"
tail -n +2 "$OUT/compilacion.csv" | awk -F, '{printf "| %s | %s | %s | %s | %s |\n", $2, $3, $4, $5, $6}'
echo
echo "Desglose por rama del caso de referencia sin -O:"
grep -E "Lectura|Rama|Conversion|Combinacion|TOTAL" "$OUT/logs/run_O0.txt"
echo
if [ "$IS_CLANG" = 0 ]; then
    echo "Bucles vectorizados por linea de pipeline.cpp (-O3 -march=native):"
    grep -oE "[A-Za-z_.-]+\.(cpp|hpp|h|tcc):[0-9]+" "$OUT/informes/vec_optimizados.txt" | sort | uniq -c
    echo
fi
echo "Pegad todo lo que aparece desde 'Compilador:' hasta aqui para completar los apartados 6 y 7."
rm -rf "$OUT/tmp"
