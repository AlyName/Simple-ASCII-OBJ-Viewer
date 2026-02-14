#include "obj_parser.hpp"
#include "renderer.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

const float COLOR_FACTOR = 1.2;
const float BRIGHTNESS_FACTOR = 0.3;
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 60;

const char* intensityToChar(double i) {
    i = 1.0 - i;
    // static const std::string chars = " .:-+#%@";
    static const std::string chars = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/|()1{}[]?-_+~<>i!lI;:,. ";
    int index = static_cast<int>(i * (chars.size() - 1) + 0.5);
    if (index < 0) index = 0;
    if (index >= static_cast<int>(chars.size())) index = static_cast<int>(chars.size()) - 1;
    static char buf[2] = {0};
    buf[0] = chars[index];
    return buf;
}

std::string buildAsciiImage(const Renderer& renderer) {
    const std::string info_string = "[AD] Rotate, [WS] Zoom, [ESC] Exit";
    const int charWidth = 2, charHeight = 1;
    int outW = renderer.width / charWidth;
    int outH = renderer.height / charHeight;

    std::ostringstream oss;
    for (int y = 0; y < outH; y++) {
        for (int x = 0; x < outW; x++) {
            double sumR = 0, sumG = 0, sumB = 0, sumI = 0;
            int count = 0;
            bool anyColor = false;
            for (int dy = 0; dy < charHeight; dy++) {
                for (int dx = 0; dx < charWidth; dx++) {
                    int px = x * charWidth + dx;
                    int py = y * charHeight + dy;
                    if (px < renderer.width && py < renderer.height) {
                        int idx = py * renderer.width + px;
                        if (renderer.framebuffer[idx].depth < 1e10) {
                            if (renderer.framebuffer[idx].hasColor) {
                                sumR += renderer.framebuffer[idx].r;
                                sumG += renderer.framebuffer[idx].g;
                                sumB += renderer.framebuffer[idx].b;
                                anyColor = true;
                            } else {
                                sumI += renderer.framebuffer[idx].intensity;
                            }
                            count++;
                        }
                    }
                }
            }
            
            if (count == 0) {
                int r = 0, g = 0, b = 0, br = 0, bg = 0, bb = 0;
                double avg = 0;
                oss << "\033[48;2;" << br << ";" << bg << ";" << bb << "m\033[38;2;" << r << ";" << g << ";" << b << "m" << intensityToChar(avg);
            } else if (anyColor) {
                int r = std::min(255, std::max(0, static_cast<int>((sumR / count) * 255 * COLOR_FACTOR)));
                int g = std::min(255, std::max(0, static_cast<int>((sumG / count) * 255 * COLOR_FACTOR)));
                int b = std::min(255, std::max(0, static_cast<int>((sumB / count) * 255 * COLOR_FACTOR)));
                int br = std::min(255, std::max(0, static_cast<int>(r * BRIGHTNESS_FACTOR)));
                int bg = std::min(255, std::max(0, static_cast<int>(g * BRIGHTNESS_FACTOR)));
                int bb = std::min(255, std::max(0, static_cast<int>(b * BRIGHTNESS_FACTOR)));
                double avg = (r + g + b) / (255.0 * 3);
                oss << "\033[48;2;" << br << ";" << bg << ";" << bb << "m\033[38;2;" << r << ";" << g << ";" << b << "m" << intensityToChar(avg);
            } else {
                double avg = sumI / count;
                int level = std::max(0, std::min(5, static_cast<int>(avg * 5)));
                int gray = std::min(255, 232 + level * 4);
                int bgGray = std::max(232, gray - 12);
                oss << "\033[48;5;" << bgGray << "m\033[38;5;" << gray << "m" << intensityToChar(avg);
            }
        }
        oss << "\033[0m\n";
    }
    oss << "\033[0m\n" << info_string;
    return oss.str();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Windows: enable ANSI escape sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    std::string objPath;
    if (argc >= 2) {
        objPath = argv[1];
    } else {
        std::cout << "Enter OBJ file path: ";
        std::getline(std::cin, objPath);
    }

    Mesh mesh;
    if (!mesh.load(objPath)) {
        std::cerr << "Load failed" << std::endl;
        return 1;
    }

    Vec3 minV(1e30, 1e30, 1e30), maxV(-1e30, -1e30, -1e30);
    for (const auto& v : mesh.vertices) {
        minV.x = std::min(minV.x, v.x);
        minV.y = std::min(minV.y, v.y);
        minV.z = std::min(minV.z, v.z);
        maxV.x = std::max(maxV.x, v.x);
        maxV.y = std::max(maxV.y, v.y);
        maxV.z = std::max(maxV.z, v.z);
    }
    Vec3 center = (minV + maxV) * 0.5;
    double size = std::max({maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z});
    if (size < 1e-6) size = 1.0;
    double scale = 2.0 / size;

    Vec3 target(0, 0, 0);
    Vec3 up(0, 1, 0);
    Mat4 proj = Mat4::perspective(45, 1.5, 0.1, 100);
    Mat4 baseModel = Mat4::scale(scale) * Mat4::translate(Vec3(-center.x, -center.y, -center.z));

    int pixelW = std::max(1, SCREEN_WIDTH), pixelH = std::max(1, SCREEN_HEIGHT);
    Renderer renderer(pixelW, pixelH);
    const double rotSpeed = 0.05;
    const double zoomSpeed = 1.05;

    double rotX = 0, rotY = 0;
    double cameraDist = 3.0;

    {
        Vec3 eye(0, 0, cameraDist);
        Mat4 view = Mat4::lookAt(eye, target, up);
        Mat4 rot = Mat4::rotateY(rotY) * Mat4::rotateX(rotX);
        Mat4 model = rot * baseModel;
        renderer.viewProj = proj * view * model;
        renderer.render(mesh);
        std::cout << buildAsciiImage(renderer);
    }

#ifdef _WIN32
    while (true) {
        int c = _getch();
        if (c == 27) break;
        if (c == 'w' || c == 'W') cameraDist = std::max(0.2, cameraDist / zoomSpeed);
        if (c == 's' || c == 'S') cameraDist = std::min(50.0, cameraDist * zoomSpeed);
        if (c == 'a' || c == 'A') rotY += rotSpeed;
        if (c == 'd' || c == 'D') rotY -= rotSpeed;

        Vec3 eye(0, 0, cameraDist);
        Mat4 view = Mat4::lookAt(eye, target, up);
        Mat4 rot = Mat4::rotateY(rotY) * Mat4::rotateX(rotX);
        Mat4 model = rot * baseModel;
        renderer.viewProj = proj * view * model;
        renderer.render(mesh);

        std::cout << "\033[2J\033[H" << buildAsciiImage(renderer);
    }
#else
    struct termios oldT, newT;
    tcgetattr(STDIN_FILENO, &oldT);
    newT = oldT;
    newT.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newT);

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) continue;
        if (c == 27) break;
        if (c == 'w' || c == 'W') cameraDist = std::max(0.2, cameraDist / zoomSpeed);
        if (c == 's' || c == 'S') cameraDist = std::min(50.0, cameraDist * zoomSpeed);
        if (c == 'a' || c == 'A') rotY += rotSpeed;
        if (c == 'd' || c == 'D') rotY -= rotSpeed;

        Vec3 eye(0, 0, cameraDist);
        Mat4 view = Mat4::lookAt(eye, target, up);
        Mat4 rot = Mat4::rotateY(rotY) * Mat4::rotateX(rotX);
        Mat4 model = rot * baseModel;
        renderer.viewProj = proj * view * model;
        renderer.render(mesh);

        std::cout << "\033[2J\033[H" << buildAsciiImage(renderer);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldT);
#endif

    return 0;
}
