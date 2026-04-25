#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

// --- 基礎結構 ---
struct Vec3f { float x, y, z; };
struct Color { float r, g, b; };

// --- 重心座標計算 (用於插值 Z 與 顏色) ---
void barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p, float& u, float& v, float& w) {
    // 計算三角形兩條邊向量
    float det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(det) < 1e-6) { u = v = w = -1; return; }
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

// --- 3D 旋轉轉換 ---
Vec3f transform(Vec3f v, float angleX, float angleY) {
    float x = v.x, y = v.y, z = v.z;
    float cY = cos(angleY), sY = sin(angleY);
    float nx = x * cY + z * sY, nz = -x * sY + z * cY;
    float cX = cos(angleX), sX = sin(angleX);
    float ny = y * cX - nz * sX, nz2 = y * sX + nz * cX;
    return { nx, ny, nz2 };
}

int main() {
    const int W = 600, H = 600;
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(W, H, "Z-Buffer Color Interpolation", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 垂直同步防閃爍

    // 立方體頂點
    std::vector<Vec3f> verts = {
        {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5},
        {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {-0.5,0.5,-0.5}
    };
    // 每個頂點分配一個固定顏色 (RGB)
    std::vector<Color> vertColors = {
        {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0},
        {255,0,255}, {0,255,255}, {255,255,255}, {128,128,128}
    };
    // 三角形面
    std::vector<std::vector<int>> faces = {
        {0,1,2}, {0,2,3}, {4,7,6}, {4,6,5}, {0,4,5}, {0,5,1},
        {1,5,6}, {1,6,2}, {2,6,7}, {2,7,3}, {4,0,3}, {4,3,7}
    };

    std::vector<unsigned char> frameBuffer(W * H * 3, 0);
    // Z-Buffer 初始化為負無限遠 (代表最遠處)
    std::vector<float> depthBuffer(W * H, -std::numeric_limits<float>::max());

    float angleX = 0.5f, angleY = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        angleY += 0.015f;
        std::fill(frameBuffer.begin(), frameBuffer.end(), 0);
        std::fill(depthBuffer.begin(), depthBuffer.end(), -std::numeric_limits<float>::max());

        // --- 渲染每個面 ---
        for (auto& f : faces) {
            Vec3f v0_raw = verts[f[0]], v1_raw = verts[f[1]], v2_raw = verts[f[2]];
            Color c0 = vertColors[f[0]], c1 = vertColors[f[1]], c2 = vertColors[f[2]];

            Vec3f t0 = transform(v0_raw, angleX, angleY);
            Vec3f t1 = transform(v1_raw, angleX, angleY);
            Vec3f t2 = transform(v2_raw, angleX, angleY);

            // 投影至螢幕 (並保留 z 用於深度測試)
            Vec3f p0 = { t0.x * 250 + W / 2, H / 2 - t0.y * 250, t0.z };
            Vec3f p1 = { t1.x * 250 + W / 2, H / 2 - t1.y * 250, t1.z };
            Vec3f p2 = { t2.x * 250 + W / 2, H / 2 - t2.y * 250, t2.z };

            // Bounding Box 優化渲染
            int minX = std::max(0, (int)std::min({ p0.x, p1.x, p2.x }));
            int maxX = std::min(W - 1, (int)std::max({ p0.x, p1.x, p2.x }));
            int minY = std::max(0, (int)std::min({ p0.y, p1.y, p2.y }));
            int maxY = std::min(H - 1, (int)std::max({ p0.y, p1.y, p2.y }));

            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    float u, v, w;
                    barycentric(p0, p1, p2, { (float)x, (float)y, 0 }, u, v, w);

                    if (u >= 0 && v >= 0 && w >= 0) {
                        // 1. 插值計算當前像素的深度 z
                        float z = u * p0.z + v * p1.z + w * p2.z;
                        int idx = y * W + x;

                        // 2. Z-Buffer 測試：如果當前像素離相機更近 (z 值較大)
                        if (z > depthBuffer[idx]) {
                            depthBuffer[idx] = z; // 更新深度

                            // 3. 顏色插值：將三個頂點顏色混合
                            int fb_idx = idx * 3;
                            frameBuffer[fb_idx] = (unsigned char)(u * c0.r + v * c1.r + w * c2.r);
                            frameBuffer[fb_idx + 1] = (unsigned char)(u * c0.g + v * c1.g + w * c2.g);
                            frameBuffer[fb_idx + 2] = (unsigned char)(u * c0.b + v * c1.b + w * c2.b);
                        }
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}