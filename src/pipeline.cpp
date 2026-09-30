#include "pipeline.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------
// Etapa 1: lectura de imagen real desde disco (PNM binario P5/P6)
// ---------------------------------------------------------------------
// Lee un token de la cabecera PNM, saltando espacios en blanco y
// comentarios (los que empiezan por '#' hasta fin de linea), tal como
// exige el estandar del formato.
static std::string read_pnm_token(FILE* f) {
    int c;
    for (;;) {
        c = std::fgetc(f);
        if (c == EOF) throw std::runtime_error("PNM: fin de archivo inesperado en la cabecera");
        if (c == '#') { while (c != '\n' && c != EOF) c = std::fgetc(f); continue; }
        if (!std::isspace(c)) break;
    }
    std::string tok;
    tok.push_back(static_cast<char>(c));
    for (;;) {
        c = std::fgetc(f);
        if (c == EOF || std::isspace(c)) break;
        tok.push_back(static_cast<char>(c));
    }
    return tok;
}

Image read_ppm(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("No se pudo abrir '" + path + "' para lectura");

    std::string magic = read_pnm_token(f);
    int channels;
    if (magic == "P6") channels = 3;
    else if (magic == "P5") channels = 1;
    else {
        std::fclose(f);
        throw std::runtime_error(path + ": formato PNM no soportado ('" + magic + "'), se esperaba P5 o P6");
    }

    int width = std::stoi(read_pnm_token(f));
    int height = std::stoi(read_pnm_token(f));
    int maxval = std::stoi(read_pnm_token(f));
    if (maxval != 255) {
        std::fclose(f);
        throw std::runtime_error(path + ": solo se soporta maxval=255 (tiene " + std::to_string(maxval) + ")");
    }
    // Nota: el caracter separador entre el token de maxval y los datos
    // binarios ya ha sido consumido dentro de read_pnm_token (su bucle
    // de lectura de token se detiene tras leer, no antes de leer, el
    // primer caracter de espacio en blanco que encuentra).

    Image img(width, height, channels);
    size_t expected = img.data.size();
    size_t got = std::fread(img.data.data(), 1, expected, f);
    std::fclose(f);
    if (got != expected) {
        throw std::runtime_error(path + ": los bytes leidos (" + std::to_string(got) +
                                  ") no coinciden con width*height*channels (" + std::to_string(expected) + ")");
    }
    return img;
}

// ---------------------------------------------------------------------
// Redimensionado (vecino mas cercano)
// ---------------------------------------------------------------------
Image resize_image(const Image& img, int new_width, int new_height) {
    if (new_width <= 0 || new_height <= 0) throw std::runtime_error("resize_image: dimensiones invalidas");
    Image out(new_width, new_height, img.channels);
    for (int y = 0; y < new_height; ++y) {
        int sy = static_cast<int>(static_cast<long long>(y) * img.height / new_height);
        if (sy >= img.height) sy = img.height - 1;
        for (int x = 0; x < new_width; ++x) {
            int sx = static_cast<int>(static_cast<long long>(x) * img.width / new_width);
            if (sx >= img.width) sx = img.width - 1;
            for (int c = 0; c < img.channels; ++c) {
                out.at(x, y, c) = img.at(sx, sy, c);
            }
        }
    }
    return out;
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
// Rama: estadisticas por canal RGB (reduccion simple)
// ---------------------------------------------------------------------
RGBStats compute_rgb_stats(const Image& rgb) {
    if (rgb.channels != 3) throw std::runtime_error("compute_rgb_stats espera 3 canales");
    RGBStats stats{};
    long n = static_cast<long>(rgb.width) * rgb.height;
    double sum[3] = {0.0, 0.0, 0.0};
    double sumsq[3] = {0.0, 0.0, 0.0};

    for (int y = 0; y < rgb.height; ++y) {
        for (int x = 0; x < rgb.width; ++x) {
            for (int c = 0; c < 3; ++c) {
                double v = rgb.at(x, y, c);
                sum[c] += v;
                sumsq[c] += v * v;
            }
        }
    }
    for (int c = 0; c < 3; ++c) {
        double mean = sum[c] / n;
        double var = sumsq[c] / n - mean * mean;
        if (var < 0.0) var = 0.0; // solo por redondeo numerico
        stats.mean[c] = mean;
        stats.stddev[c] = std::sqrt(var);
    }
    return stats;
}

// ---------------------------------------------------------------------
// Rama: histograma + Otsu (segmentacion automatica sobre la intensidad)
// ---------------------------------------------------------------------
Histogram compute_histogram(const Image& gray) {
    if (gray.channels != 1) throw std::runtime_error("compute_histogram espera 1 canal");
    Histogram hist{};
    hist.fill(0);
    for (int y = 0; y < gray.height; ++y)
        for (int x = 0; x < gray.width; ++x)
            hist[gray.at(x, y, 0)]++;
    return hist;
}

uint8_t otsu_threshold(const Histogram& hist, long total_pixels) {
    // Metodo de Otsu: recorre los 256 umbrales posibles y se queda con el
    // que maximiza la varianza entre clases (fondo vs. primer plano).
    double sum_total = 0.0;
    for (int i = 0; i < 256; ++i) sum_total += static_cast<double>(i) * hist[i];

    double sum_bg = 0.0;
    long w_bg = 0;
    double best_variance = -1.0;
    int best_thresh = 0;

    for (int t = 0; t < 256; ++t) {
        w_bg += hist[t];
        if (w_bg == 0) continue;
        long w_fg = total_pixels - w_bg;
        if (w_fg == 0) break;

        sum_bg += static_cast<double>(t) * hist[t];
        double mean_bg = sum_bg / static_cast<double>(w_bg);
        double mean_fg = (sum_total - sum_bg) / static_cast<double>(w_fg);
        double diff = mean_bg - mean_fg;

        double between = static_cast<double>(w_bg) * static_cast<double>(w_fg) * diff * diff;
        if (between > best_variance) {
            best_variance = between;
            best_thresh = t;
        }
    }
    return static_cast<uint8_t>(best_thresh);
}

// ---------------------------------------------------------------------
// Rama: deteccion de esquinas (Moravec, precursor simplificado de Harris)
// ---------------------------------------------------------------------
Image detect_corners_moravec(const Image& gray, int window_radius,
                              long response_threshold, int nms_radius) {
    if (gray.channels != 1) throw std::runtime_error("detect_corners_moravec espera 1 canal");
    int w = gray.width, h = gray.height;

    // 4 direcciones principales de desplazamiento de la ventana.
    static const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    std::vector<long> score(static_cast<size_t>(w) * h, 0);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            long min_ssd = -1;
            for (const auto& d : dirs) {
                long ssd = 0;
                for (int wy = -window_radius; wy <= window_radius; ++wy) {
                    for (int wx = -window_radius; wx <= window_radius; ++wx) {
                        int p1 = gray.clamped(x + wx, y + wy);
                        int p2 = gray.clamped(x + wx + d[0], y + wy + d[1]);
                        int diff = p1 - p2;
                        ssd += static_cast<long>(diff) * diff;
                    }
                }
                if (min_ssd < 0 || ssd < min_ssd) min_ssd = ssd;
            }
            score[static_cast<size_t>(y) * w + x] = min_ssd;
        }
    }

    // Umbralizacion + supresion de no-maximos local (evita "nubes" de
    // esquinas detectadas alrededor del mismo punto real).
    Image out(w, h, 1);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            long s = score[static_cast<size_t>(y) * w + x];
            if (s < response_threshold) continue;

            bool is_max = true;
            for (int ny = -nms_radius; ny <= nms_radius && is_max; ++ny) {
                for (int nx = -nms_radius; nx <= nms_radius; ++nx) {
                    if (nx == 0 && ny == 0) continue;
                    int xx = x + nx, yy = y + ny;
                    if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                    if (score[static_cast<size_t>(yy) * w + xx] > s) { is_max = false; break; }
                }
            }
            if (is_max) out.at(x, y, 0) = 255;
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Combinacion final (fan-in): bordes en rojo, esquinas como circulos verdes
// ---------------------------------------------------------------------
Image compose_annotated(const Image& base_rgb, const Image& edge_map, const Image& corner_map) {
    if (base_rgb.channels != 3) throw std::runtime_error("compose_annotated espera base RGB");
    Image out = base_rgb; // copia
    int w = out.width, h = out.height;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (edge_map.at(x, y, 0) > 0) {
                out.at(x, y, 0) = 255;
                out.at(x, y, 1) = 0;
                out.at(x, y, 2) = 0;
            }
        }
    }

    const int radius = 3;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (corner_map.at(x, y, 0) == 0) continue;
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (dx * dx + dy * dy > radius * radius) continue;
                    int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                    out.at(xx, yy, 0) = 0;
                    out.at(xx, yy, 1) = 255;
                    out.at(xx, yy, 2) = 0;
                }
            }
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
