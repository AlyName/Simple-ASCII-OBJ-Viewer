#pragma once

#include "math.hpp"
#include "texture.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

struct Vec2 {
    double u, v;
    Vec2() : u(0), v(0) {}
    Vec2(double u, double v) : u(u), v(v) {}
};

struct Triangle {
    int v0, v1, v2;
    int n0, n1, n2;
    int t0, t1, t2;
    Triangle() : v0(0), v1(0), v2(0), n0(-1), n1(-1), n2(-1), t0(-1), t1(-1), t2(-1) {}
};

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;
    std::vector<Triangle> triangles;
    Texture texture;

    static std::string dirOf(const std::string& path) {
        size_t p = path.find_last_of("/\\");
        return p == std::string::npos ? "" : path.substr(0, p + 1);
    }

    static std::string baseName(const std::string& path) {
        size_t p = path.find_last_of("/\\");
        std::string name = p == std::string::npos ? path : path.substr(p + 1);
        size_t dot = name.find_last_of('.');
        return dot == std::string::npos ? name : name.substr(0, dot);
    }

    bool loadMtl(const std::string& mtlPath, std::string& outMapKd) {
        std::ifstream f(mtlPath);
        if (!f.is_open()) return false;
        std::string line, curMapKd;
        while (std::getline(f, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "map_Kd") {
                iss >> curMapKd;
                if (!curMapKd.empty()) outMapKd = curMapKd;
            }
        }
        return !outMapKd.empty();
    }

    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << path << std::endl;
            return false;
        }

        std::string objDir = dirOf(path);
        std::string mtlPath;
        std::string mapKdPath;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                double x, y, z;
                if (iss >> x >> y >> z)
                    vertices.push_back(Vec3(x, y, z));
            } else if (prefix == "vt") {
                double u, v;
                if (iss >> u >> v)
                    texCoords.push_back(Vec2(u, v));
            } else if (prefix == "vn") {
                double x, y, z;
                if (iss >> x >> y >> z)
                    normals.push_back(Vec3(x, y, z).normalized());
            } else if (prefix == "mtllib") {
                iss >> mtlPath;
            } else if (prefix == "f") {
                std::vector<int> vindices, tindices, nindices;
                std::string token;
                while (iss >> token) {
                    int vi = -1, ti = -1, ni = -1;
                    size_t slash1 = token.find('/');
                    if (slash1 == std::string::npos) {
                        vi = std::stoi(token);
                    } else {
                        vi = std::stoi(token.substr(0, slash1));
                        size_t slash2 = token.find('/', slash1 + 1);
                        if (slash2 == std::string::npos) {
                            if (slash1 + 1 < token.size())
                                ti = std::stoi(token.substr(slash1 + 1));
                        } else {
                            if (slash1 + 1 < slash2)
                                ti = std::stoi(token.substr(slash1 + 1, slash2 - slash1 - 1));
                            if (slash2 + 1 < token.size())
                                ni = std::stoi(token.substr(slash2 + 1));
                        }
                    }
                    vindices.push_back(vi > 0 ? vi - 1 : static_cast<int>(vertices.size()) + vi);
                    if (ti > 0) tindices.push_back(ti - 1);
                    else if (ti < 0 && !texCoords.empty()) tindices.push_back(static_cast<int>(texCoords.size()) + ti);
                    if (ni > 0) nindices.push_back(ni - 1);
                    else if (ni < 0 && !normals.empty()) nindices.push_back(static_cast<int>(normals.size()) + ni);
                }

                for (size_t i = 1; i + 1 < vindices.size(); i++) {
                    Triangle tri;
                    tri.v0 = vindices[0];
                    tri.v1 = vindices[i];
                    tri.v2 = vindices[i + 1];
                    tri.n0 = nindices.size() > 0 ? nindices[0] : -1;
                    tri.n1 = nindices.size() > i ? nindices[i] : -1;
                    tri.n2 = nindices.size() > i + 1 ? nindices[i + 1] : -1;
                    tri.t0 = tindices.size() > 0 ? tindices[0] : -1;
                    tri.t1 = tindices.size() > i ? tindices[i] : -1;
                    tri.t2 = tindices.size() > i + 1 ? tindices[i + 1] : -1;
                    triangles.push_back(tri);
                }
            }
        }

        if (mtlPath.empty())
            mtlPath = baseName(path) + ".mtl";

        std::string fullMtl = objDir + mtlPath;
        if (loadMtl(fullMtl, mapKdPath)) {
            if (!texture.load(mapKdPath)) {
                size_t sep = mapKdPath.find_last_of("/\\");
                std::string fname = sep == std::string::npos ? mapKdPath : mapKdPath.substr(sep + 1);
                texture.load(objDir + fname);
            }
        }

        // std::cout << "Loaded OBJ: " << vertices.size() << " vertices, "
        //           << texCoords.size() << " UVs, " << triangles.size() << " triangles";
        // std::cout << texture.data.size() << std::endl;
        // if (texture.width > 0)
        //     std::cout << ", texture " << texture.width << "x" << texture.height;
        // std::cout << std::endl;

        return !vertices.empty() && !triangles.empty();
    }
};
