#include "image.hpp"
#include "pipeline.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

using Clock = std::chrono::steady_clock;
static double elapsed_ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

struct Config {
    int num_images = 40;
    int width = 768;
    int height = 768;
    int kernel_size = 7;
    double sigma = 1.5;
    int thresh = 60;
    int save_samples = 2;
    unsigned seed = 12345;
    std::string output_dir = "output";
};

static void print_usage(const char* prog) {
    std::printf(
        "Uso: %s [opciones]\n"
        "  --num-images N     numero de imagenes del lote (def. 40)\n"
        "  --width W          ancho de cada imagen (def. 768)\n"
        "  --height H         alto de cada imagen (def. 768)\n"
        "  --kernel-size K    tamano del kernel gaussiano, impar (def. 7)\n"
        "  --sigma S          sigma del filtro gaussiano (def. 1.5)\n"
        "  --threshold T      umbral de binarizacion, 0-255 (def. 60)\n"
        "  --save-samples N   num. de imagenes de las que guardar cada etapa (def. 2)\n"
        "  --seed S           semilla del generador sintetico (def. 12345)\n"
        "  --output-dir DIR   carpeta de salida (def. output)\n",
        prog);
}

static Config parse_args(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Falta valor para %s\n", name);
                std::exit(1);
            }
            return argv[++i];
        };
        if (arg == "--num-images") cfg.num_images = std::stoi(next("--num-images"));
        else if (arg == "--width") cfg.width = std::stoi(next("--width"));
        else if (arg == "--height") cfg.height = std::stoi(next("--height"));
        else if (arg == "--kernel-size") cfg.kernel_size = std::stoi(next("--kernel-size"));
        else if (arg == "--sigma") cfg.sigma = std::stod(next("--sigma"));
        else if (arg == "--threshold") cfg.thresh = std::stoi(next("--threshold"));
        else if (arg == "--save-samples") cfg.save_samples = std::stoi(next("--save-samples"));
        else if (arg == "--seed") cfg.seed = static_cast<unsigned>(std::stoul(next("--seed")));
        else if (arg == "--output-dir") cfg.output_dir = next("--output-dir");
        else if (arg == "-h" || arg == "--help") { print_usage(argv[0]); std::exit(0); }
        else { std::fprintf(stderr, "Argumento desconocido: %s\n", arg.c_str()); print_usage(argv[0]); std::exit(1); }
    }
    return cfg;
}

int main(int argc, char** argv) {
    Config cfg = parse_args(argc, argv);
    mkdir(cfg.output_dir.c_str(), 0755); // no falla si ya existe; ignoramos el valor de retorno

    std::printf("=== Pipeline de deteccion de bordes (secuencial) ===\n");
    std::printf("Lote: %d imagenes de %dx%d | kernel=%d sigma=%.2f umbral=%d\n",
                cfg.num_images, cfg.width, cfg.height, cfg.kernel_size, cfg.sigma, cfg.thresh);

    // Acumuladores de tiempo por etapa (utiles para el apartado 2.1/2.6:
    // que etapa domina el tiempo total de ejecucion).
    double t_gen = 0, t_gray = 0, t_blur = 0, t_sobel = 0, t_thresh = 0;

    auto t_total_start = Clock::now();

    for (int i = 0; i < cfg.num_images; ++i) {
        auto t0 = Clock::now();
        Image original = generate_synthetic_image(cfg.width, cfg.height, cfg.seed + static_cast<unsigned>(i));
        auto t1 = Clock::now();

        Image gray = to_grayscale(original);
        auto t2 = Clock::now();

        Image blurred = gaussian_blur(gray, cfg.kernel_size, cfg.sigma);
        auto t3 = Clock::now();

        Image gradient = sobel_gradient(blurred);
        auto t4 = Clock::now();

        Image edges = threshold(gradient, static_cast<uint8_t>(cfg.thresh));
        auto t5 = Clock::now();

        t_gen += elapsed_ms(t0, t1);
        t_gray += elapsed_ms(t1, t2);
        t_blur += elapsed_ms(t2, t3);
        t_sobel += elapsed_ms(t3, t4);
        t_thresh += elapsed_ms(t4, t5);

        if (i < cfg.save_samples) {
            write_pnm(original, cfg.output_dir + "/img" + std::to_string(i) + "_0_original.ppm");
            write_pnm(gray, cfg.output_dir + "/img" + std::to_string(i) + "_1_gray.pgm");
            write_pnm(blurred, cfg.output_dir + "/img" + std::to_string(i) + "_2_blur.pgm");
            write_pnm(gradient, cfg.output_dir + "/img" + std::to_string(i) + "_3_sobel.pgm");
            write_pnm(edges, cfg.output_dir + "/img" + std::to_string(i) + "_4_edges.pgm");
        }
    }

    auto t_total_end = Clock::now();
    double total_ms = elapsed_ms(t_total_start, t_total_end);

    std::printf("\n--- Tiempos acumulados por etapa (todas las imagenes) ---\n");
    std::printf("  1. Generacion sintetica : %8.2f ms\n", t_gen);
    std::printf("  2. Escala de grises     : %8.2f ms\n", t_gray);
    std::printf("  3. Suavizado gaussiano  : %8.2f ms\n", t_blur);
    std::printf("  4. Gradiente Sobel      : %8.2f ms\n", t_sobel);
    std::printf("  5. Umbralizacion        : %8.2f ms\n", t_thresh);
    std::printf("--------------------------------------------------------\n");
    std::printf("  TOTAL                   : %8.2f ms (%.3f s)\n", total_ms, total_ms / 1000.0);
    std::printf("  Imagenes/segundo        : %8.2f\n", cfg.num_images / (total_ms / 1000.0));
    std::printf("\nMuestras guardadas en '%s/' (%d imagenes x 5 etapas)\n",
                cfg.output_dir.c_str(), cfg.save_samples);

    return 0;
}
