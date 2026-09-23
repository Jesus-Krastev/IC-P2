CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
SRC := src/main.cpp src/pipeline.cpp
HDR := src/image.hpp src/pipeline.hpp
BIN := pipeline
OUTDIR := output

# Parametros del caso de prueba de referencia (Tarea 2: documentad estos
# valores y el tiempo obtenido en la memoria).
NUM_IMAGES := 20
WIDTH := 768
HEIGHT := 768
KERNEL := 5
SIGMA := 1.2
THRESH := 60

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

run: all
	./$(BIN) --num-images $(NUM_IMAGES) --width $(WIDTH) --height $(HEIGHT) \
		--kernel-size $(KERNEL) --sigma $(SIGMA) --threshold $(THRESH) \
		--save-samples 2 --output-dir $(OUTDIR)

clean:
	rm -f $(BIN)
	rm -rf $(OUTDIR)
