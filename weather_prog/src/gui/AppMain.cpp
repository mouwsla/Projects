#include "aviwx/gui/MainWindow.h"

#include <QApplication>
#include <QDir>

#include <filesystem>
#include <iostream>

namespace {

void printUsage(const char* program)
{
    std::cerr << "Usage:\n"
              << "  " << program << " [scenario.json]\n"
              << "  " << program << " --map-scale 1m|200k [scenario.json]\n"
              << "  " << program << " --sxf-dir path/to/maps [scenario.json]\n"
              << "  " << program << " --screenshot output.png [--map-scale 1m|200k] [scenario.json]\n";
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("AviWx");
    QApplication::setOrganizationName("AviWx");

    bool screenshot_mode = false;
    std::filesystem::path screenshot_path;
    std::filesystem::path scenario_path;
    std::filesystem::path sxf_dir;
    std::string map_scale;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--screenshot") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            screenshot_mode = true;
            screenshot_path = argv[i];
            continue;
        }
        if (arg == "--map-scale") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            map_scale = argv[i];
            continue;
        }
        if (arg == "--sxf-dir") {
            if (++i >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            sxf_dir = argv[i];
            continue;
        }
        if (scenario_path.empty()) {
            scenario_path = arg;
            continue;
        }
        printUsage(argv[0]);
        return 2;
    }

    aviwx::gui::MainWindow window;
    window.resize(1440, 900);

    if (!scenario_path.empty()) {
        window.loadScenarioFile(scenario_path);
    }
    if (!map_scale.empty()) {
        window.loadSxfScale(map_scale);
    }
    if (!sxf_dir.empty()) {
        window.loadSxfFolderPath(sxf_dir);
    }

    if (screenshot_mode) {
        window.show();
        app.processEvents();
        if (!window.exportScreenshot(screenshot_path)) {
            return 1;
        }
        return 0;
    }

    window.show();
    return app.exec();
}
