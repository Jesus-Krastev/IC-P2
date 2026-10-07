CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
SRC := src/main.cpp src/pipeline.cpp
HDR := src/image.hpp src/pipeline.hpp
BIN := pipeline
OUTDIR := output
IMAGES_DIR := images

# Parametros del caso de prueba de referencia (Tarea 2: documentad estos
# valores y el tiempo obtenido en la memoria).
# IMAGES: nombres de los .ppm de images/, separados por coma y SIN espacios.
IMAGES := leon.ppm
REPEAT := 4
RESIZE_WIDTH := 0
RESIZE_HEIGHT := 0
BLUR_KERNEL := 5
BLUR_SIGMA := 1.2
EDGE_THRESH := 60
CORNER_WINDOW := 1
CORNER_THRESH := 2000
CORNER_NMS := 6

.PHONY: all run clean experimentos

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

run: all
	./$(BIN) --images "$(IMAGES)" --images-dir $(IMAGES_DIR) --repeat $(REPEAT) \
		--resize-width $(RESIZE_WIDTH) --resize-height $(RESIZE_HEIGHT) \
		--blur-kernel $(BLUR_KERNEL) --blur-sigma $(BLUR_SIGMA) --edge-thresh $(EDGE_THRESH) \
		--corner-window $(CORNER_WINDOW) --corner-thresh $(CORNER_THRESH) --corner-nms $(CORNER_NMS) \
		--save-samples 2 --output-dir $(OUTDIR)

clean:
	rm -f $(BIN)
	rm -rf $(OUTDIR) resultados

# Mediciones de los apartados 2.3-2.5 (varios minutos)
experimentos:
	bash experimentos.sh
