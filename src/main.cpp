#include "image.hpp"
#include "pipeline.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

using Clock = std::chrono::steady_clock;
static double elapsed_ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

struct Config {
    // Lote: lista fija de archivos dentro de images_dir. Se repite
    // `repeat` veces para alcanzar un tamano de lote comodo de medir
    // (el enunciado pide que el caso de referencia dure del orden de
    // segundos). Editad esta lista para que coincida con vuestras
    // imagenes reales, o pasadla con --images.
    std::vector<std::string> image_files = {"leon.ppm"};
    std::string images_dir = "images";
    int repeat = 4;

    // Redimensionado opcional tras la carga (0 = desactivado, se
    // mantiene la resolucion nativa del archivo). Es vuestro parametro
    // de escalado del problema para el apartado 2.3 ahora que las
    // imagenes ya no se generan con resolucion arbitraria.
    int resize_width = 0;
    int resize_height = 0;

    // Rama de bordes
    int blur_kernel = 5;
    double blur_sigma = 1.2;
    int edge_thresh = 60;
    // Rama de esquinas (Moravec)
    int corner_window = 1;
    long corner_thresh = 2000;
    int corner_nms = 6;
    // General
    int save_samples = 2;
    std::string output_dir = "output";
};

static void print_usage(const char* prog) {
    std::printf(
        "Uso: %s [opciones]\n"
        "  --images \"a.ppm,b.ppm\"  lista de archivos separados por coma (sustituye la lista por defecto)\n"
        "  --images-dir DIR        carpeta donde buscar los archivos (def. images)\n"
        "  --repeat N              veces que se repite la lista para formar el lote (def. 4)\n"
        "  --resize-width W        redimensiona a este ancho (0 = tamano nativo, def. 0)\n"
        "  --resize-height H       redimensiona a este alto (0 = tamano nativo, def. 0)\n"
        "  --blur-kernel K         kernel gaussiano, impar (def. 5)\n"
        "  --blur-sigma S          sigma del filtro gaussiano (def. 1.2)\n"
        "  --edge-thresh T         umbral fijo para el gradiente Sobel (def. 60)\n"
        "  --corner-window R       radio de ventana de Moravec (def. 1)\n"
        "  --corner-thresh T       umbral de 'cornerness' (def. 2000)\n"
        "  --corner-nms R          radio de supresion de no-maximos (def. 6)\n"
        "  --save-samples N        num. de imagenes de las que guardar salidas (def. 2)\n"
        "  --output-dir DIR        carpeta de salida (def. output)\n",
        prog);
}

static std::vector<std::string> split_csv(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

static Config parse_args(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "Falta valor para %s\n", name); std::exit(1); }
            return argv[++i];
        };
        if (arg == "--images") cfg.image_files = split_csv(next("--images"));
        else if (arg == "--images-dir") cfg.images_dir = next("--images-dir");
        else if (arg == "--repeat") cfg.repeat = std::stoi(next("--repeat"));
        else if (arg == "--resize-width") cfg.resize_width = std::stoi(next("--resize-width"));
        else if (arg == "--resize-height") cfg.resize_height = std::stoi(next("--resize-height"));
        else if (arg == "--blur-kernel") cfg.blur_kernel = std::stoi(next("--blur-kernel"));
        else if (arg == "--blur-sigma") cfg.blur_sigma = std::stod(next("--blur-sigma"));
        else if (arg == "--edge-thresh") cfg.edge_thresh = std::stoi(next("--edge-thresh"));
        else if (arg == "--corner-window") cfg.corner_window = std::stoi(next("--corner-window"));
        else if (arg == "--corner-thresh") cfg.corner_thresh = std::stol(next("--corner-thresh"));
        else if (arg == "--corner-nms") cfg.corner_nms = std::stoi(next("--corner-nms"));
        else if (arg == "--save-samples") cfg.save_samples = std::stoi(next("--save-samples"));
        else if (arg == "--output-dir") cfg.output_dir = next("--output-dir");
        else if (arg == "-h" || arg == "--help") { print_usage(argv[0]); std::exit(0); }
        else { std::fprintf(stderr, "Argumento desconocido: %s\n", arg.c_str()); print_usage(argv[0]); std::exit(1); }
    }
    return cfg;
}

static long count_nonzero(const Image& img) {
    long n = 0;
    for (uint8_t v : img.data) if (v > 0) ++n;
    return n;
}

int main(int argc, char** argv) {
    Config cfg = parse_args(argc, argv);
    mkdir(cfg.output_dir.c_str(), 0755);

    if (cfg.image_files.empty()) {
        std::fprintf(stderr, "No hay imagenes que procesar (lista vacia).\n");
        return 1;
    }

    // Construye la lista completa de rutas a procesar: la lista de
    // archivos repetida `repeat` veces.
    std::vector<std::string> batch_paths;
    for (int r = 0; r < cfg.repeat; ++r)
        for (const auto& name : cfg.image_files)
            batch_paths.push_back(cfg.images_dir + "/" + name);

    std::printf("=== Pipeline de analisis de imagen (secuencial, 4 ramas) ===\n");
    std::printf("Lote: %zu imagenes (%zu archivo(s) x %d repeticion(es)) desde '%s/'\n",
                batch_paths.size(), cfg.image_files.size(), cfg.repeat, cfg.images_dir.c_str());
    if (cfg.resize_width > 0 && cfg.resize_height > 0) {
        std::printf("Redimensionado a %dx%d\n", cfg.resize_width, cfg.resize_height);
    } else {
        std::printf("Resolucion nativa de cada archivo (sin redimensionar)\n");
    }

    double t_lectura = 0, t_gris = 0;
    double t_rgbstats = 0;
    double t_bordes = 0;
    double t_histotsu = 0;
    double t_esquinas = 0;
    double t_combinar = 0;

    auto t_total_start = Clock::now();
    int saved_count = 0;

    for (size_t i = 0; i < batch_paths.size(); ++i) {
        auto t0 = Clock::now();
        Image original;
        try {
            original = read_ppm(batch_paths[i]);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Error leyendo '%s': %s\n", batch_paths[i].c_str(), e.what());
            return 1;
        }
        if (cfg.resize_width > 0 && cfg.resize_height > 0) {
            original = resize_image(original, cfg.resize_width, cfg.resize_height);
        }
        auto t1 = Clock::now();
        t_lectura += elapsed_ms(t0, t1);

        auto ra0 = Clock::now();
        RGBStats stats = compute_rgb_stats(original);
        auto ra1 = Clock::now();
        t_rgbstats += elapsed_ms(ra0, ra1);

        auto tg0 = Clock::now();
        Image gray = to_grayscale(original);
        auto tg1 = Clock::now();
        t_gris += elapsed_ms(tg0, tg1);

        auto rb0 = Clock::now();
        Image blurred = gaussian_blur(gray, cfg.blur_kernel, cfg.blur_sigma);
        Image gradient = sobel_gradient(blurred);
        Image edges = threshold(gradient, static_cast<uint8_t>(cfg.edge_thresh));
        auto rb1 = Clock::now();
        t_bordes += elapsed_ms(rb0, rb1);

        auto rc0 = Clock::now();
        Histogram hist = compute_histogram(gray);
        uint8_t otsu_t = otsu_threshold(hist, static_cast<long>(gray.width) * gray.height);
        Image otsu_binary = threshold(gray, otsu_t);
        auto rc1 = Clock::now();
        t_histotsu += elapsed_ms(rc0, rc1);

        auto rd0 = Clock::now();
        Image corners = detect_corners_moravec(gray, cfg.corner_window, cfg.corner_thresh, cfg.corner_nms);
        auto rd1 = Clock::now();
        t_esquinas += elapsed_ms(rd0, rd1);

        auto rf0 = Clock::now();
        Image annotated = compose_annotated(original, edges, corners);
        long edge_pixels = count_nonzero(edges);
        long corner_pixels = count_nonzero(corners);
        auto rf1 = Clock::now();
        t_combinar += elapsed_ms(rf0, rf1);

        if (saved_count < cfg.save_samples) {
            std::string base = batch_paths[i];
            size_t slash = base.find_last_of('/');
            std::string filename = (slash == std::string::npos) ? base : base.substr(slash + 1);
            std::string p = cfg.output_dir + "/" + std::to_string(saved_count) + "_" + filename + "_";

            write_pnm(original, p + "0_original.ppm");
            write_pnm(gray, p + "1_gray.pgm");
            write_pnm(blurred, p + "2_blur.pgm");
            write_pnm(gradient, p + "3_sobel.pgm");
            write_pnm(edges, p + "4_edges.pgm");
            write_pnm(otsu_binary, p + "5_otsu.pgm");
            write_pnm(corners, p + "6_corners.pgm");
            write_pnm(annotated, p + "7_annotated.ppm");

            FILE* f = std::fopen((p + "8_report.txt").c_str(), "w");
            if (f) {
                std::fprintf(f, "Informe - %s\n", batch_paths[i].c_str());
                std::fprintf(f, "Estadisticas RGB:\n");
                const char* names[3] = {"R", "G", "B"};
                for (int c = 0; c < 3; ++c) {
                    std::fprintf(f, "  %s: media=%.2f  desv.tipica=%.2f\n", names[c], stats.mean[c], stats.stddev[c]);
                }
                std::fprintf(f, "Umbral de Otsu (sobre intensidad de gris): %d\n", otsu_t);
                std::fprintf(f, "Pixeles de borde (umbral fijo=%d): %ld (%.2f%%)\n",
                            cfg.edge_thresh, edge_pixels, 100.0 * edge_pixels / (original.width * original.height));
                std::fprintf(f, "Esquinas detectadas (Moravec): %ld\n", corner_pixels);
                std::fclose(f);
            }
            ++saved_count;
        }
    }

    auto t_total_end = Clock::now();
    double total_ms = elapsed_ms(t_total_start, t_total_end);
    double branch_sum = t_lectura + t_rgbstats + t_gris + t_bordes + t_histotsu + t_esquinas + t_combinar;

    std::printf("\n--- Tiempos acumulados por rama (todas las imagenes) ---\n");
    std::printf("  Lectura (+ resize si aplica)   : %9.2f ms\n", t_lectura);
    std::printf("  Rama RGB stats                 : %9.2f ms\n", t_rgbstats);
    std::printf("  Conversion a grises (comun)    : %9.2f ms\n", t_gris);
    std::printf("  Rama bordes (blur+sobel+umbr.) : %9.2f ms\n", t_bordes);
    std::printf("  Rama histograma + Otsu         : %9.2f ms\n", t_histotsu);
    std::printf("  Rama esquinas (Moravec)        : %9.2f ms\n", t_esquinas);
    std::printf("  Combinacion final (fan-in)     : %9.2f ms\n", t_combinar);
    std::printf("---------------------------------------------------------\n");
    std::printf("  Suma de ramas                  : %9.2f ms\n", branch_sum);
    std::printf("  TOTAL (reloj real)             : %9.2f ms (%.3f s)\n", total_ms, total_ms / 1000.0);
    std::printf("  Imagenes/segundo               : %9.2f\n", batch_paths.size() / (total_ms / 1000.0));
    std::printf("\nMuestras guardadas en '%s/' (%d imagenes x 8 salidas)\n",
                cfg.output_dir.c_str(), saved_count);

    return 0;
}
