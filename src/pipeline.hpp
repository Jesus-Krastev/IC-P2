#pragma once
#include "image.hpp"
#include <array>
#include <cstdint>
#include <string>

// Etapa 1: lectura de una imagen real desde disco, en formato PNM binario
// (P6 = RGB, P5 = escala de grises; maxval debe ser 255). Es el mismo
// formato que escribe write_pnm, asi que cualquier imagen convertida a
// .ppm/.pgm con esa cabecera es valida.
Image read_ppm(const std::string& path);

// Redimensionado por vecino mas cercano (simple, suficiente para variar
// el tamano del problema del apartado 2.3 sobre imagenes reales de
// resolucion fija).
Image resize_image(const Image& img, int new_width, int new_height);

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

// ---------------------------------------------------------------------
// Ramas independientes que parten de la lectura / de la escala de grises
// ---------------------------------------------------------------------

// Rama "estadisticas RGB": depende solo de la imagen original (no de gris).
struct RGBStats {
    double mean[3];
    double stddev[3];
};
RGBStats compute_rgb_stats(const Image& rgb);

// Rama "histograma + Otsu": el umbral optimo se calcula sobre la
// intensidad de gris (no sobre el gradiente), asi que da una segmentacion
// distinta y complementaria a la deteccion de bordes.
using Histogram = std::array<int, 256>;
Histogram compute_histogram(const Image& gray);
uint8_t otsu_threshold(const Histogram& hist, long total_pixels);

// Rama "esquinas": version simplificada de Harris (detector de Moravec).
// Para cada pixel, compara la ventana local con la misma ventana desplazada
// en 4 direcciones; el minimo de esas diferencias es la "cornerness".
// Sin gradientes ni tensor de estructura -> mucho mas simple de implementar
// que Harris completo, aunque conceptualmente es su antecesor directo.
Image detect_corners_moravec(const Image& gray, int window_radius,
                              long response_threshold, int nms_radius);

// Paso de combinacion (fan-in): superpone bordes (rojo) y esquinas
// (circulos verdes) sobre la imagen original en color.
Image compose_annotated(const Image& base_rgb, const Image& edge_map, const Image& corner_map);
