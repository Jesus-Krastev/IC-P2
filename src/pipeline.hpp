#pragma once
#include "image.hpp"
#include <cstdint>
#include <string>

// Etapa 1: generación sintética de una imagen RGB, determinista dada una
// semilla (para que el caso de prueba sea 100% reproducible sin depender
// de archivos externos). Combina ondas senoidales con ruido pseudoaleatorio.
Image generate_synthetic_image(int width, int height, unsigned seed);

// Etapa 2: conversión a escala de grises (fórmula de luminosidad estándar).
Image to_grayscale(const Image& rgb);

// Etapa 3: suavizado gaussiano mediante convolución 2D directa (no separada
// a propósito: así el bucle de convolución es el candidato "de libro" para
// el estudio de autovectorización del apartado 2.4).
Image gaussian_blur(const Image& gray, int kernel_size, double sigma);

// Etapa 4: gradiente de Sobel (Gx, Gy con kernels 3x3) -> magnitud,
// normalizada y saturada a [0,255].
Image sobel_gradient(const Image& blurred);

// Etapa 5: umbralización binaria (0 o 255) sobre la magnitud del gradiente.
Image threshold(const Image& gradient, uint8_t thresh);

// Utilidad: guarda una imagen (1 o 3 canales) en formato PPM/PGM binario
// (P5 para 1 canal, P6 para 3 canales). No requiere ninguna librería externa.
void write_pnm(const Image& img, const std::string& path);
