#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

// --- 3D 基礎結構 ---
struct Vec3f {
    float x, y, z;
    Vec3f operator-(const Vec3f& v) const { return { x - v.x, y - v.y, z - v.z }; }
};

struct Point {
    int x, y;
};

// --- 向量運算：外積 ---
Vec3f cross_product(Vec3f a, Vec3f b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// --- 向量運算：正規化 ---
Vec3f normalize(Vec3f v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (len > 0) ? Vec3f{ v.x / len, v.y / len, v.z / len } : v;
}

// --- 核心：實體三角形填色函式 (Scanline Fill) ---
void fill_triangle(Point p0, Point p1, Point p2, std::vector<unsigned char>& fb, int w, int h, unsigned char color) {
    // 1. 頂點依 Y 座標排序 (p0 最上, p2 最下)
    if (p0.y > p1.y) std::swap(p0, p1);
    if (p0.y > p2.y) std::swap(p0, p2);
    if (p1.y > p2.y) std::swap(p1, p2);

    int total_height = p2.y - p0.y;
    if (total_height == 0) return;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > p1.y - p0.y || p1.y == p0.y;
        int segment_height = second_half ? p2.y - p1.y : p1.y - p0.y;
        if (segment_height == 0) continue;

        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? p1.y - p0.y : 0)) / segment_height;

        // 計算左右兩側的 X 邊界
        int ax = (int)(p0.x + (p2.x - p0.x) * alpha);
        int bx = second_half ?
            (int)(p1.x + (p2.x - p1.x) * beta) :
            (int)(p0.x + (p1.x - p0.x) * beta);

        if (ax > bx) std::swap(ax, bx);

        // 填滿水平線段
        int py = p0.y + i;
        if (py < 0 || py >= h) continue;

        for (int px = ax; px <= bx; px++) {
            if (px >= 0 && px < w) {
                int idx = (py * w + px) * 3;
                fb[idx] = fb[idx + 1] = fb[idx + 2] = color;
            }
        }
    }
}

// --- 旋轉轉換 ---
Vec3f transform(Vec3f v, float angleX, float angleY) {
    float x = v.x, y = v.y, z = v.z;
    // Y 軸旋轉
    float cY = std::cos(angleY), sY = std::sin(angleY);
    float nx = x * cY + z * sY, nz = -x * sY + z * cY;
    // X 軸旋轉
    float cX = std::cos(angleX), sX = std::sin(angleX);
    float ny = y * cX - nz * sX, nz2 = y * sX + nz * cX;
    return { nx, ny, nz2 };
}

int main() {
    const int W = 600, H = 600;
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(W, H, "3D Software Renderer - Solid Shading", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // [解決閃爍關鍵] 啟用垂直同步，鎖定 FPS
    glfwSwapInterval(1);

    // 立方體數據 (12個三角形面)
    std::vector<Vec3f> verts = {
        {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5},
        {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {-0.5,0.5,-0.5}
    };
    std::vector<std::vector<int>> faces = {
        {0,1,2}, {0,2,3}, {4,7,6}, {4,6,5}, {0,4,5}, {0,5,1},
        {1,5,6}, {1,6,2}, {2,6,7}, {2,7,3}, {4,0,3}, {4,3,7}
    };

    std::vector<unsigned char> framebuffer(W * H * 3, 0);
    float angleX = 0.5f, angleY = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        angleY += 0.015f; // 動畫速度
        std::memset(framebuffer.data(), 0, framebuffer.size()); // 快速清空背景

        std::vector<Point> projected;
        std::vector<Vec3f> transformed;
        for (auto& v_raw : verts) {
            Vec3f v = transform(v_raw, angleX, angleY);
            transformed.push_back(v);
            projected.push_back({ (int)(v.x * 250 + W / 2), (int)(H / 2 - v.y * 250) });
        }

        for (auto& f : faces) {
            Vec3f v0 = transformed[f[0]], v1 = transformed[f[1]], v2 = transformed[f[2]];

            // 1. 計算法向量與背面剔除
            Vec3f normal = normalize(cross_product(v1 - v0, v2 - v0));

            // 2. 簡單光照 (假設光源在 0, 0, 1)
            float intensity = normal.z;

            if (intensity > 0) {
                // 給一點基礎環境光，並放大光照對比度
                unsigned char c = (unsigned char)(std::min(1.0f, std::max(0.1f, intensity)) * 255);
                fill_triangle(projected[f[0]], projected[f[1]], projected[f[2]], framebuffer, W, H, c);
            }
        }

        // 繪製到螢幕
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, framebuffer.data());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}