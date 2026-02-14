#pragma once

#include <cmath>
#include <algorithm>

// 3D vector
struct Vec3 {
    double x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(double s) const { return Vec3(x / s, y / s, z / s); }
    
    double dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    double length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const {
        double len = length();
        return len > 1e-10 ? (*this) / len : Vec3(0, 0, 0);
    }
};

// 4x4 matrix (column-major)
struct Mat4 {
    double m[16];
    
    Mat4() {
        for (int i = 0; i < 16; i++) m[i] = 0;
        m[0] = m[5] = m[10] = m[15] = 1;
    }
    
    static Mat4 identity() { return Mat4(); }
    
    static Mat4 perspective(double fov, double aspect, double near, double far) {
        Mat4 mat;
        double tanHalfFov = std::tan(fov * 0.5 * 3.14159265 / 180.0);
        mat.m[0] = 1.0 / (aspect * tanHalfFov);
        mat.m[5] = 1.0 / tanHalfFov;
        mat.m[10] = -(far + near) / (far - near);
        mat.m[11] = -1.0;
        mat.m[14] = -(2.0 * far * near) / (far - near);
        mat.m[15] = 0;
        return mat;
    }
    
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);
        
        Mat4 mat;
        mat.m[0] = r.x; mat.m[4] = r.y; mat.m[8]  = r.z;
        mat.m[1] = u.x; mat.m[5] = u.y; mat.m[9]  = u.z;
        mat.m[2] = -f.x; mat.m[6] = -f.y; mat.m[10] = -f.z;
        mat.m[12] = -r.dot(eye);
        mat.m[13] = -u.dot(eye);
        mat.m[14] = f.dot(eye);
        return mat;
    }
    
    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                r.m[col * 4 + row] = 0;
                for (int k = 0; k < 4; k++)
                    r.m[col * 4 + row] += m[k * 4 + row] * b.m[col * 4 + k];
            }
        }
        return r;
    }
    
    static Mat4 scale(double s) {
        Mat4 mat;
        mat.m[0] = mat.m[5] = mat.m[10] = s;
        return mat;
    }
    
    static Mat4 translate(const Vec3& t) {
        Mat4 mat;
        mat.m[12] = t.x; mat.m[13] = t.y; mat.m[14] = t.z;
        return mat;
    }
    
    static Mat4 rotateX(double rad) {
        double c = std::cos(rad), s = std::sin(rad);
        Mat4 mat;
        mat.m[5] = c; mat.m[6] = s;
        mat.m[9] = -s; mat.m[10] = c;
        return mat;
    }
    
    static Mat4 rotateY(double rad) {
        double c = std::cos(rad), s = std::sin(rad);
        Mat4 mat;
        mat.m[0] = c; mat.m[2] = -s;
        mat.m[8] = s; mat.m[10] = c;
        return mat;
    }
    
    Vec3 transformPoint(const Vec3& p) const {
        double w = m[3]*p.x + m[7]*p.y + m[11]*p.z + m[15];
        if (std::abs(w) < 1e-10) w = 1e-10;
        return Vec3(
            (m[0]*p.x + m[4]*p.y + m[8]*p.z + m[12]) / w,
            (m[1]*p.x + m[5]*p.y + m[9]*p.z + m[13]) / w,
            (m[2]*p.x + m[6]*p.y + m[10]*p.z + m[14]) / w
        );
    }
};
