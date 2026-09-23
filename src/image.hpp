#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>

// Imagen en formato "planar por pixel": para cada pixel se guardan sus
// `channels` bytes consecutivos (interleaved), en orden fila a fila.
// Es la organización más simple y la que usaremos para todo el pipeline;
// se comenta explícitamente porque el apartado 2.2 de la práctica (estudio
// de datos y accesos a memoria) os pedirá razonar sobre este layout.
struct Image {
    int width = 0;
    int height = 0;
    int channels = 1; // 1 = escala de grises, 3 = RGB
    std::vector<uint8_t> data;

    Image() = default;

    Image(int w, int h, int c)
        : width(w), height(h), channels(c),
          data(static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c), 0) {}

    inline size_t index(int x, int y, int c = 0) const {
        return (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x))
                   * static_cast<size_t>(channels)
               + static_cast<size_t>(c);
    }

    inline uint8_t& at(int x, int y, int c = 0) { return data[index(x, y, c)]; }
    inline uint8_t at(int x, int y, int c = 0) const { return data[index(x, y, c)]; }

    // Acceso con "clamp" en los bordes (replica el píxel del borde).
    // Lo usan las convoluciones (blur, Sobel) para no salirse del array.
    inline uint8_t clamped(int x, int y, int c = 0) const {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= width) x = width - 1;
        if (y >= height) y = height - 1;
        return at(x, y, c);
    }
};
