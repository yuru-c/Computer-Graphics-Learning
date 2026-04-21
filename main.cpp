#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// --- 3D 座標點 ---
struct Vec3f { float x, y, z; };

// --- 簡化版 Model (為了讓你直接跑) ---
struct SimpleCube {
    std::vector<Vec3f> verts = {
        {-0.5,-0.5, 0.5}, {0.5,-0.5, 0.5}, {0.5, 0.5, 0.5}, {-0.5, 0.5, 0.5},
        {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5, 0.5,-0.5}, {-0.5, 0.5,-0.5}
    };
    std::vector<std::vector<int>> faces = {
        {0,1,2,3}, {4,5,6,7}, {0,1,5,4}, {2,3,7,6}, {1,2,6,5}, {0,3,7,4}
    };
};

// --- 你之前重構的 transform 函式 ---
Vec3f transform(Vec3f v, float angleX, float angleY) {
    float x = v.x, y = v.y, z = v.z;
    float cY = cos(angleY), sY = sin(angleY);
    float nx = x * cY + z * sY, nz = -x * sY + z * cY;
    float cX = cos(angleX), sX = sin(angleX);
    float ny = y * cX - nz * sX, nz2 = y * sX + nz * cX;
    return { nx, ny, nz2 };
}

// --- 畫線函式 ---
void draw_line(int x0, int y0, int x1, int y1, std::vector<unsigned char>& fb, int w, int h) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
    float steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return;
    float xInc = dx / steps, yInc = dy / steps, x = (float)x0, y = (float)y0;
    for (int i = 0; i <= (int)steps; i++) {
        if (x >= 0 && x < w && y >= 0 && y < h) {
            int idx = ((int)y * w + (int)x) * 3;
            fb[idx] = fb[idx + 1] = fb[idx + 2] = 255; // 畫白色
        }
        x += xInc; y += yInc;
    }
}

int main() {
    const int W = 600, H = 600;
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(W, H, "3D Cube Animation", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    SimpleCube cube;
    std::vector<unsigned char> framebuffer(W * H * 3, 0);
    float angleX = 0.5f, angleY = 0.0f;

    // --- 實時渲染迴圈 ---
    while (!glfwWindowShouldClose(window)) {
        angleY += 0.015f; // 控制旋轉速度
        std::fill(framebuffer.begin(), framebuffer.end(), 0); // 每幀清空畫面

        // 1. 幾何階段
        std::vector<std::pair<int, int>> projected;
        for (auto& v_raw : cube.verts) {
            Vec3f v = transform(v_raw, angleX, angleY);
            // 投影到螢幕座標
            projected.push_back({ (int)(v.x * 250 + W / 2), (int)(H / 2 - v.y * 250) });
        }

        // 2. 光柵化階段
        for (auto& face : cube.faces) {
            for (int j = 0; j < 4; j++) {
                auto p0 = projected[face[j]];
                auto p1 = projected[face[(j + 1) % 4]];
                draw_line(p0.first, p0.second, p1.first, p1.second, framebuffer, W, H);
            }
        }

        // 3. 將像素矩陣推送到視窗
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, framebuffer.data());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}