#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

extern "C" unsigned char* stbi_load(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels);
extern "C" void stbi_image_free(void* retval_from_stbi_load);

struct Texture {
    std::vector<unsigned char> data;
    int width = 0, height = 0, channels = 0;

    bool load(const std::string& path) {
        int w, h, n;
        unsigned char* img = stbi_load(path.c_str(), &w, &h, &n, 3);
        if (!img) return false;
        width = w;
        height = h;
        channels = 3;
        data.assign(img, img + w * h * 3);
        stbi_image_free(img);
        return true;
    }

    void sample(double u, double v, double& r, double& g, double& b) const {
        if (data.empty()) {
            r = g = b = 0.5;
            return;
        }
        u = std::fmod(u, 1.0);
        v = std::fmod(v, 1.0);
        if (u < 0) u += 1.0;
        if (v < 0) v += 1.0;
        v = 1.0 - v;

        int x = static_cast<int>(u * width) % width;
        int y = static_cast<int>(v * height) % height;
        if (x < 0) x += width;
        if (y < 0) y += height;

        int idx = (y * width + x) * 3;
        r = data[idx] / 255.0;
        g = data[idx + 1] / 255.0;
        b = data[idx + 2] / 255.0;
    }
};
