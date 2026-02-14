#pragma once

#include "math.hpp"
#include "obj_parser.hpp"
#include "texture.hpp"
#include <vector>
#include <algorithm>
#include <limits>

// Screen pixel (RGB 0-1, depth; use intensity for grayscale when no texture)
struct Pixel {
    double r, g, b;
    double intensity;
    double depth;
    bool hasColor;

    Pixel() : r(0), g(0), b(0), intensity(0), depth(std::numeric_limits<double>::max()), hasColor(false) {}
};

class Renderer {
public:
    int width, height;
    std::vector<Pixel> framebuffer;
    std::vector<double> zBuffer;
    
    Vec3 lightDir;
    Mat4 viewProj;
    
    Renderer(int w, int h) : width(w), height(h) {
        framebuffer.resize(w * h);
        zBuffer.resize(w * h, std::numeric_limits<double>::max());
        lightDir = Vec3(0.5, 0.5, 1.0).normalized();
    }
    
    void clear() {
        std::fill(framebuffer.begin(), framebuffer.end(), Pixel());
        std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<double>::max());
    }
    
    // Convert NDC (-1,1) to screen coordinates
    bool ndcToScreen(double ndcX, double ndcY, double ndcZ, int& sx, int& sy, double& sz) const {
        if (ndcZ < -1 || ndcZ > 1) return false;
        sx = static_cast<int>((ndcX + 1.0) * 0.5 * width);
        sy = static_cast<int>((1.0 - ndcY) * 0.5 * height);
        sz = ndcZ;
        return sx >= 0 && sx < width && sy >= 0 && sy < height;
    }
    
    // Check if point is inside triangle using barycentric coordinates
    bool insideTriangle(double x, double y,
        double x0, double y0, double x1, double y1, double x2, double y2,
        double& w0, double& w1, double& w2) const {
        double denom = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (std::abs(denom) < 1e-10) return false;
        
        w1 = ((x - x0) * (y2 - y0) - (x2 - x0) * (y - y0)) / denom;
        w2 = ((x1 - x0) * (y - y0) - (x - x0) * (y1 - y0)) / denom;
        w0 = 1.0 - w1 - w2;
        
        return w0 >= 0 && w1 >= 0 && w2 >= 0;
    }
    
    void rasterizeTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2,
                           const Vec3& n0, const Vec3& n1, const Vec3& n2,
                           double u0, double v0, double u1, double v1, double u2, double v2,
                           const Texture* tex) {
        // Project to NDC
        Vec3 sp0 = viewProj.transformPoint(p0);
        Vec3 sp1 = viewProj.transformPoint(p1);
        Vec3 sp2 = viewProj.transformPoint(p2);
        
        // Back-face culling (check cross product in NDC)
        double cross = (sp1.x - sp0.x) * (sp2.y - sp0.y) - (sp2.x - sp0.x) * (sp1.y - sp0.y);
        if (cross <= 0) return;
        
        // Compute normal (for lighting)
        Vec3 faceNormal = (p1 - p0).cross(p2 - p0).normalized();
        double shade = std::max(0.0, faceNormal.dot(lightDir));
        shade = 0.3 + 0.7 * shade;  // ambient + diffuse
        
        // Screen space bounding box (NDC [-1,1] -> screen)
        int minX = std::max(0, static_cast<int>(std::floor((std::min({sp0.x, sp1.x, sp2.x}) + 1.0) * 0.5 * width)));
        int maxX = std::min(width - 1, static_cast<int>(std::ceil((std::max({sp0.x, sp1.x, sp2.x}) + 1.0) * 0.5 * width)));
        int minY = std::max(0, static_cast<int>(std::floor((1.0 - std::max({sp0.y, sp1.y, sp2.y})) * 0.5 * height)));
        int maxY = std::min(height - 1, static_cast<int>(std::ceil((1.0 - std::min({sp0.y, sp1.y, sp2.y})) * 0.5 * height)));
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double px = (x + 0.5) / width * 2.0 - 1.0;
                double py = 1.0 - (y + 0.5) / height * 2.0;
                
                double w0, w1, w2;
                if (!insideTriangle(px, py, sp0.x, sp0.y, sp1.x, sp1.y, sp2.x, sp2.y, w0, w1, w2))
                    continue;
                
                double z = w0 * sp0.z + w1 * sp1.z + w2 * sp2.z;
                int idx = y * width + x;
                if (z < zBuffer[idx]) {
                    zBuffer[idx] = z;
                    framebuffer[idx].depth = z;
                    framebuffer[idx].intensity = shade;
                    if (tex && tex->width > 0) {
                        double u = w0 * u0 + w1 * u1 + w2 * u2;
                        double v = w0 * v0 + w1 * v1 + w2 * v2;
                        tex->sample(u, v, framebuffer[idx].r, framebuffer[idx].g, framebuffer[idx].b);
                        framebuffer[idx].r *= shade;
                        framebuffer[idx].g *= shade;
                        framebuffer[idx].b *= shade;
                        framebuffer[idx].hasColor = true;
                    } else {
                        framebuffer[idx].hasColor = false;
                    }
                }
            }
        }
    }
    
    void render(const Mesh& mesh) {
        clear();
        
        const Texture* tex = mesh.texture.width > 0 ? &mesh.texture : nullptr;
        for (const auto& tri : mesh.triangles) {
            Vec3 p0 = mesh.vertices[tri.v0];
            Vec3 p1 = mesh.vertices[tri.v1];
            Vec3 p2 = mesh.vertices[tri.v2];

            Vec3 n0, n1, n2;
            if (tri.n0 >= 0 && tri.n1 >= 0 && tri.n2 >= 0) {
                n0 = mesh.normals[tri.n0];
                n1 = mesh.normals[tri.n1];
                n2 = mesh.normals[tri.n2];
            } else {
                Vec3 n = (p1 - p0).cross(p2 - p0).normalized();
                n0 = n1 = n2 = n;
            }

            double u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0;
            if (tri.t0 >= 0 && tri.t1 >= 0 && tri.t2 >= 0 && tri.t0 < (int)mesh.texCoords.size() &&
                tri.t1 < (int)mesh.texCoords.size() && tri.t2 < (int)mesh.texCoords.size()) {
                u0 = mesh.texCoords[tri.t0].u; v0 = mesh.texCoords[tri.t0].v;
                u1 = mesh.texCoords[tri.t1].u; v1 = mesh.texCoords[tri.t1].v;
                u2 = mesh.texCoords[tri.t2].u; v2 = mesh.texCoords[tri.t2].v;
            }

            rasterizeTriangle(p0, p1, p2, n0, n1, n2, u0, v0, u1, v1, u2, v2, tex);
        }
    }
};
