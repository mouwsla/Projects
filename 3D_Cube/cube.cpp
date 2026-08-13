/**
 * @file cube.cpp
 * @brief 3D Cube in term
 * @author tobymayr
 */
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t running = 1;

/**
 * @brief Структура вектора или точки в пространстве
 */
struct Vec3 {
    float x;
    float y;
    float z;
};

/**
 * @brief Останавливает анимацию после сигнала от системы
 * @param[in] signal Номер полученного сигнала
 */
void stopAnimation([[maybe_unused]] int signal) {
    running = 0;
}

/**
 * @brief Приводит длину вектора к единице, сохраняя его направление
 * @param[in] value Исходный вектор
 * @return Вектор длиной в одну единицу
 */
Vec3 normalized(Vec3 value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    return {value.x / length, value.y / length, value.z / length};
}

/**
 * @brief Считает скалярное произведение двух векторов
 * @param[in] lhs Первый вектор
 * @param[in] rhs Второй вектор
 * @return Скалярное произведение векторов
 */
float dot(const Vec3& lhs, const Vec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * @brief Поворачивает точку вокруг трёх осей
 * @param[in] point Исходная точка
 * @param[in] angleX Угол поворота вокруг X в радианах
 * @param[in] angleY Угол поворота вокруг Y в радианах
 * @param[in] angleZ Угол поворота вокруг Z в радианах
 * @return Новое положение точки после поворота
 */
Vec3 rotate(Vec3 point, float angleX, float angleY, float angleZ) {
    const float sinX = std::sin(angleX);
    const float cosX = std::cos(angleX);
    const float sinY = std::sin(angleY);
    const float cosY = std::cos(angleY);
    const float sinZ = std::sin(angleZ);
    const float cosZ = std::cos(angleZ);

    // Сначала поворачиваем вокруг X, затем вокруг Y и Z
    point = {point.x, point.y * cosX - point.z * sinX,
             point.y * sinX + point.z * cosX};
    point = {point.x * cosY + point.z * sinY, point.y,
             -point.x * sinY + point.z * cosY};
    return {point.x * cosZ - point.y * sinZ,
            point.x * sinZ + point.y * cosZ, point.z};
}

/**
 * @brief Узнаёт текущий размер окна терминала
 * @return Количество столбцов и строк
 */
std::pair<int, int> terminalSize() {
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 && size.ws_row > 0) {
        return {size.ws_col, size.ws_row};
    }
    return {80, 24};
}

/**
 * @brief Готовит терминал к анимации и возвращает настройки при выходе
 */
class TerminalGuard {
public:
    TerminalGuard() {
        if (tcgetattr(STDIN_FILENO, &originalSettings) == 0) {
            termios settings = originalSettings;
            settings.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            settings.c_cc[VMIN] = 0;
            settings.c_cc[VTIME] = 0;
            inputConfigured = tcsetattr(STDIN_FILENO, TCSANOW, &settings) == 0;
        }

        std::cout << "\x1b[2J\x1b[H\x1b[?25l" << std::flush;
    }

    ~TerminalGuard() {
        if (inputConfigured) {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
        }

        std::cout << "\x1b[?25h\x1b[0m\n" << std::flush;
    }

    TerminalGuard(const TerminalGuard&) = delete;
    TerminalGuard& operator=(const TerminalGuard&) = delete;

private:
    termios originalSettings{};
    bool inputConfigured = false;
};

/**
 * @brief Хранит настройки и углы поворота куба
 */
struct CubeState {
    float angleX = 0.0F;
    float angleY = 0.0F;
    float angleZ = 0.0F;
    float manualAngleX = 0.0F;
    float manualAngleY = 0.0F;
    bool colorsEnabled = false;
    bool visualEnabled = false;
    int escapeState = 0;
};

/**
 * @brief Поворачивает куб в ручном режиме по клавишам WASD
 * @param[in] key Нажатая клавиша
 * @param[in,out] state Текущее состояние куба
 */
void rotateFromKey(char key, CubeState& state) {
    if (!state.visualEnabled) {
        return;
    }

    constexpr float rotationStep = 0.12F;

    switch (key) {
        case 'w':
        case 'W':
            state.manualAngleX += rotationStep;
            break;
        case 's':
        case 'S':
            state.manualAngleX -= rotationStep;
            break;
        case 'a':
        case 'A':
            state.manualAngleY += rotationStep;
            break;
        case 'd':
        case 'D':
            state.manualAngleY -= rotationStep;
            break;
        default:
            break;
    }
}

/**
 * @brief Поворачивает куб в ручном режиме по стрелкам
 * @param[in] key Код нажатой стрелки
 * @param[in,out] state Текущее состояние куба
 */
void rotateFromArrow(char key, CubeState& state) {
    if (!state.visualEnabled) {
        return;
    }

    constexpr float rotationStep = 0.12F;

    switch (key) {
        case 'A':
            state.manualAngleX += rotationStep;
            break;
        case 'B':
            state.manualAngleX -= rotationStep;
            break;
        case 'C':
            state.manualAngleY -= rotationStep;
            break;
        case 'D':
            state.manualAngleY += rotationStep;
            break;
        default:
            break;
    }
}

/**
 * @brief Обрабатывает одну клавишу из терминала
 * @param[in] key Нажатая клавиша
 * @param[in,out] state Текущее состояние куба
 */
void handleKey(char key, CubeState& state) {
    if (state.escapeState == 1) {
        state.escapeState = key == '[' ? 2 : 0;
        return;
    }

    if (state.escapeState == 2) {
        rotateFromArrow(key, state);
        state.escapeState = 0;
        return;
    }

    if (key == '\x1b') {
        state.escapeState = 1;
    } else if (key == 'c' || key == 'C') {
        state.colorsEnabled = !state.colorsEnabled;
    } else if (key == 'v' || key == 'V') {
        state.visualEnabled = !state.visualEnabled;
    } else {
        rotateFromKey(key, state);
    }
}

/**
 * @brief Считывает все доступные нажатия без остановки анимации
 * @param[in,out] state Текущее состояние куба
 */
void handleInput(CubeState& state) {
    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(STDIN_FILENO, &readSet);
        timeval timeout{};

        if (select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout) <= 0) {
            break;
        }

        char key = '\0';
        if (read(STDIN_FILENO, &key, 1) != 1) {
            break;
        }

        handleKey(key, state);
    }
}

/**
 * @brief Описывает положение и направление одной грани куба
 */
struct Face {
    Vec3 center;
    Vec3 axisU;
    Vec3 axisV;
    Vec3 normal;
    int color;
};

/**
 * @brief Рисует один кадр вращающегося куба
 * @param[in] state Текущее состояние куба
 */
void renderFrame(const CubeState& state) {
    const auto [terminalWidth, terminalHeight] = terminalSize();
    const int width = std::clamp(terminalWidth, 20, 160);
    const int height = std::clamp(terminalHeight - 1, 10, 60);

    std::vector<char> pixels(static_cast<std::size_t>(width * height), ' ');
    std::vector<float> depthBuffer(static_cast<std::size_t>(width * height), 0.0F);
    std::vector<int> colorBuffer(static_cast<std::size_t>(width * height), 0);

    constexpr std::array<Face, 6> faces{{
        {{1, 0, 0},  {0, 1, 0}, {0, 0, 1}, {1, 0, 0},  31},
        {{-1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0}, 32},
        {{0, 1, 0},  {1, 0, 0}, {0, 0, 1}, {0, 1, 0},  34},
        {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, 33},
        {{0, 0, 1},  {1, 0, 0}, {0, 1, 0}, {0, 0, 1},  35},
        {{0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, 36},
    }};

    constexpr char shades[] = ".:-=+*#%@";
    constexpr int shadeCount = static_cast<int>(sizeof(shades) - 1);
    const Vec3 lightDirection = normalized({-0.5F, 0.8F, -1.0F});
    const float projectionScale = static_cast<float>(std::min(width, height * 2)) * 1.15F;

    for (const Face& face : faces) {
        Vec3 rotatedNormal = rotate(face.normal, state.angleX, state.angleY, state.angleZ);
        rotatedNormal = rotate(rotatedNormal, state.manualAngleX, state.manualAngleY, 0.0F);
        const float brightness = std::clamp(dot(rotatedNormal, lightDirection), 0.08F, 1.0F);
        const char shade = shades[static_cast<int>(brightness * (shadeCount - 1))];

        for (float u = -1.0F; u <= 1.001F; u += 0.045F) {
            for (float v = -1.0F; v <= 1.001F; v += 0.045F) {
                Vec3 point{
                    face.center.x + face.axisU.x * u + face.axisV.x * v,
                    face.center.y + face.axisU.y * u + face.axisV.y * v,
                    face.center.z + face.axisU.z * u + face.axisV.z * v,
                };
                point = rotate(point, state.angleX, state.angleY, state.angleZ);
                point = rotate(point, state.manualAngleX, state.manualAngleY, 0.0F);
                point.z += 5.0F;

                const float inverseDepth = 1.0F / point.z;
                const int screenX = static_cast<int>(width / 2.0F + point.x * projectionScale * inverseDepth);
                const int screenY = static_cast<int>(height / 2.0F - point.y * projectionScale * 0.52F * inverseDepth);

                if (screenX < 0 || screenX >= width || screenY < 1 || screenY >= height) {
                    continue;
                }

                const std::size_t index = static_cast<std::size_t>(screenY * width + screenX);
                if (inverseDepth > depthBuffer[index]) {
                    depthBuffer[index] = inverseDepth;
                    pixels[index] = shade;
                    colorBuffer[index] = state.colorsEnabled ? face.color : 0;
                }
            }
        }
    }

    const std::string title = std::string("C colors: ")
        + (state.colorsEnabled ? "ON" : "OFF")
        + " | V visual: " + (state.visualEnabled ? "ON" : "OFF")
        + " | WASD/arrows | Ctrl+C";
    for (std::size_t i = 0; i < title.size() && i < static_cast<std::size_t>(width); ++i) {
        pixels[i] = title[i];
    }

    std::string output;
    output.reserve(
        static_cast<std::size_t>((width + 16) * height)
    );

    output += "\x1b[H";

    int activeColor = -1;

    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            const std::size_t index =
                static_cast<std::size_t>(row * width + column);

            const int color = colorBuffer[index];

            if (color != activeColor) {
                output += "\x1b[" + std::to_string(color) + "m";
                activeColor = color;
            }

            output.push_back(pixels[index]);
        }

        output.push_back('\n');
    }

    output += "\x1b[0m";

    std::cout << output << std::flush;
}

} // namespace

/**
 * @brief Запускает анимацию куба и ждёт команды завершения
 * @return Код успешного завершения программы
 */
int main() {
    std::signal(SIGINT, stopAnimation);
    std::signal(SIGTERM, stopAnimation);

    TerminalGuard terminalGuard;
    auto previousFrame = std::chrono::steady_clock::now();
    CubeState state;

    while (running != 0) {
        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(now - previousFrame).count();
        previousFrame = now;

        handleInput(state);

        if (!state.visualEnabled) {
            state.angleX += deltaTime * 0.75F;
            state.angleY += deltaTime * 0.95F;
            state.angleZ += deltaTime * 0.35F;
        }

        renderFrame(state);
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    return 0;
}
