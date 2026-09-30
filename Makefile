CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
SRC := src/main.cpp src/pipeline.cpp
HDR := src/image.hpp src/pipeline.hpp
BIN := pipeline
OUTDIR := output
IMAGES_DIR := images

# Parametros del caso de prueba de referencia (Tarea 2: documentad estos
# valores y el tiempo obtenido en la memoria).
# IMPORTANT: sustituid IMAGES por los nombres reales de vuestros archivos
# .ppm dentro de la carpeta images/ (separados por coma, sin espacios).
IMAGES := leon.ppm, original.ppm
REPEAT := 12
RESIZE_WIDTH := 768
RESIZE_HEIGHT := 768
BLUR_KERNEL := 5
BLUR_SIGMA := 1.2
EDGE_THRESH := 60
CORNER_WINDOW := 1
CORNER_THRESH := 2000
CORNER_NMS := 6

.PHONY: all run clean

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
	rm -rf $(OUTDIR)
