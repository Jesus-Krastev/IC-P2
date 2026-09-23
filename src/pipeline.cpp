#include "pipeline.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------
// Etapa 1: generación sintética de imagen (RGB)
// ---------------------------------------------------------------------
// Genera un fondo en degradado suave y le superpone un conjunto de formas
// geométricas (círculos y rectángulos) de color solido. Las formas dan
// bordes reales y nítidos que Sobel + umbralización puedan detectar con
// claridad (un degradado puro, sin formas, apenas tiene gradiente).
// Todo es determinista a partir de `seed`.
Image generate_synthetic_image(int width, int height, unsigned seed) {
    Image img(width, height, 3);

    auto lcg = [](unsigned& state) -> unsigned {
        state = state * 1664525u + 1013904223u;
        return state;
    };
    unsigned state = seed;
    auto rnd01 = [&]() { return static_cast<double>(lcg(state) % 100000) / 100000.0; };

    auto clamp8 = [](double v) -> uint8_t {
        if (v < 0.0) v = 0.0;
        if (v > 255.0) v = 255.0;
        return static_cast<uint8_t>(v);
    };

    // Fondo: degradado suave (dos colores interpolados según la posición).
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double fx = static_cast<double>(x) / width;
            double fy = static_cast<double>(y) / height;
            double t = 0.5 * (fx + fy);
            double r = 30.0 + 60.0 * t;
            double g = 40.0 + 50.0 * (1.0 - t);
            double b = 80.0 + 70.0 * std::sin(t * 3.14159);
            img.at(x, y, 0) = clamp8(r);
            img.at(x, y, 1) = clamp8(g);
            img.at(x, y, 2) = clamp8(b);
        }
    }

    // Formas de color solido con bordes nítidos.
    int num_shapes = 14;
    for (int s = 0; s < num_shapes; ++s) {
        double cx = rnd01() * width;
        double cy = rnd01() * height;
        double size = (0.08 + rnd01() * 0.18) * std::min(width, height);
        uint8_t cr = static_cast<uint8_t>(rnd01() * 255);
        uint8_t cg = static_cast<uint8_t>(rnd01() * 255);
        uint8_t cb = static_cast<uint8_t>(rnd01() * 255);
        bool is_circle = rnd01() < 0.5;

        int x0 = static_cast<int>(std::max(0.0, cx - size));
        int x1 = static_cast<int>(std::min(static_cast<double>(width - 1), cx + size));
        int y0 = static_cast<int>(std::max(0.0, cy - size));
        int y1 = static_cast<int>(std::min(static_cast<double>(height - 1), cy + size));

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                bool inside;
                if (is_circle) {
                    double dx = x - cx, dy = y - cy;
                    inside = (dx * dx + dy * dy) <= (size * size);
                } else {
                    inside = true; // ya estamos dentro del bounding box del rectángulo
                }
                if (inside) {
                    img.at(x, y, 0) = cr;
                    img.at(x, y, 1) = cg;
                    img.at(x, y, 2) = cb;
                }
            }
        }
    }

    // Ruido pseudoaleatorio determinista de baja amplitud, para que el
    // suavizado gaussiano tenga un efecto visible ademas de limpiar ruido.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double noise = static_cast<double>(lcg(state) % 13) - 6.0;
            img.at(x, y, 0) = clamp8(img.at(x, y, 0) + noise);
            img.at(x, y, 1) = clamp8(img.at(x, y, 1) + noise);
            img.at(x, y, 2) = clamp8(img.at(x, y, 2) + noise);
        }
    }

    return img;
}

// ---------------------------------------------------------------------
// Etapa 2: escala de grises
// ---------------------------------------------------------------------
Image to_grayscale(const Image& rgb) {
    if (rgb.channels != 3) throw std::runtime_error("to_grayscale espera una imagen de 3 canales");
    Image out(rgb.width, rgb.height, 1);
    for (int y = 0; y < rgb.height; ++y) {
        for (int x = 0; x < rgb.width; ++x) {
            double r = rgb.at(x, y, 0);
            double g = rgb.at(x, y, 1);
            double b = rgb.at(x, y, 2);
            double lum = 0.299 * r + 0.587 * g + 0.114 * b;
            out.at(x, y, 0) = static_cast<uint8_t>(lum + 0.5);
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Etapa 3: suavizado gaussiano (convolución 2D directa)
// ---------------------------------------------------------------------
static std::vector<double> make_gaussian_kernel(int size, double sigma) {
    std::vector<double> kernel(static_cast<size_t>(size) * size);
    int half = size / 2;
    double sum = 0.0;
    for (int ky = -half; ky <= half; ++ky) {
        for (int kx = -half; kx <= half; ++kx) {
            double v = std::exp(-(kx * kx + ky * ky) / (2.0 * sigma * sigma));
            kernel[static_cast<size_t>(ky + half) * size + (kx + half)] = v;
            sum += v;
        }
    }
    for (auto& v : kernel) v /= sum; // normalizar para no alterar el brillo medio
    return kernel;
}

Image gaussian_blur(const Image& gray, int kernel_size, double sigma) {
    if (gray.channels != 1) throw std::runtime_error("gaussian_blur espera una imagen de 1 canal");
    if (kernel_size % 2 == 0) throw std::runtime_error("kernel_size debe ser impar");

    std::vector<double> kernel = make_gaussian_kernel(kernel_size, sigma);
    int half = kernel_size / 2;

    Image out(gray.width, gray.height, 1);
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            double acc = 0.0;
            for (int ky = -half; ky <= half; ++ky) {
                for (int kx = -half; kx <= half; ++kx) {
                    double w = kernel[static_cast<size_t>(ky + half) * kernel_size + (kx + half)];
                    acc += w * gray.clamped(x + kx, y + ky);
                }
            }
            out.at(x, y, 0) = static_cast<uint8_t>(acc + 0.5);
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Etapa 4: gradiente de Sobel
// ---------------------------------------------------------------------
Image sobel_gradient(const Image& blurred) {
    if (blurred.channels != 1) throw std::runtime_error("sobel_gradient espera una imagen de 1 canal");

    static const int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    static const int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    Image out(blurred.width, blurred.height, 1);
    for (int y = 0; y < blurred.height; ++y) {
        for (int x = 0; x < blurred.width; ++x) {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int px = blurred.clamped(x + kx, y + ky);
                    gx += Gx[ky + 1][kx + 1] * px;
                    gy += Gy[ky + 1][kx + 1] * px;
                }
            }
            double mag = std::sqrt(static_cast<double>(gx) * gx + static_cast<double>(gy) * gy);
            if (mag > 255.0) mag = 255.0;
            out.at(x, y, 0) = static_cast<uint8_t>(mag);
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Etapa 5: umbralización
// ---------------------------------------------------------------------
Image threshold(const Image& gradient, uint8_t thresh) {
    Image out(gradient.width, gradient.height, 1);
    for (int y = 0; y < gradient.height; ++y) {
        for (int x = 0; x < gradient.width; ++x) {
            out.at(x, y, 0) = (gradient.at(x, y, 0) >= thresh) ? 255 : 0;
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Utilidad de guardado (PGM/PPM binario, sin dependencias externas)
// ---------------------------------------------------------------------
void write_pnm(const Image& img, const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) throw std::runtime_error("No se pudo abrir " + path + " para escritura");

    if (img.channels == 1) {
        std::fprintf(f, "P5\n%d %d\n255\n", img.width, img.height);
    } else if (img.channels == 3) {
        std::fprintf(f, "P6\n%d %d\n255\n", img.width, img.height);
    } else {
        std::fclose(f);
        throw std::runtime_error("write_pnm solo soporta 1 o 3 canales");
    }
    std::fwrite(img.data.data(), 1, img.data.size(), f);
    std::fclose(f);
}
