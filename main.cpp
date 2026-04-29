#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

struct Vec3f { float x, y, z; };
struct Color { float r, g, b; };

// 重心座標插值函式 (保持不變)
void barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p, float& u, float& v, float& w) {
    float det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(det) < 1e-6) { u = v = w = -1; return; }
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

// 旋轉轉換
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
    GLFWwindow* window = glfwCreateWindow(W, H, "Perspective Projection Cube", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::vector<Vec3f> verts = {
        {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5},
        {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {-0.5,0.5,-0.5}
    };
    std::vector<Color> vertColors = {
        {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0},
        {255,0,255}, {0,255,255}, {255,255,255}, {128,128,128}
    };
    std::vector<std::vector<int>> faces = {
        {0,1,2}, {0,2,3}, {4,7,6}, {4,6,5}, {0,4,5}, {0,5,1},
        {1,5,6}, {1,6,2}, {2,6,7}, {2,7,3}, {4,0,3}, {4,3,7}
    };

    std::vector<unsigned char> frameBuffer(W * H * 3, 0);
    std::vector<float> depthBuffer(W * H, -std::numeric_limits<float>::max());

    float angleX = 0.5f, angleY = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        angleY += 0.015f;
        std::fill(frameBuffer.begin(), frameBuffer.end(), 0);
        std::fill(depthBuffer.begin(), depthBuffer.end(), -std::numeric_limits<float>::max());

        for (auto& f : faces) {
            Vec3f t_verts[3];
            Vec3f p_verts[3];
            Color colors[3];

            for (int i = 0; i < 3; i++) {
                t_verts[i] = transform(verts[f[i]], angleX, angleY);
                colors[i] = vertColors[f[i]];

                // --- 關鍵修改：透視投影 (Perspective Projection) ---
                // 我們把物體往後移 2 個單位 (z - 2)，避免它貼在鏡頭上
                float z_offset = t_verts[i].z - 2.0f;
                // 透視除法：x 和 y 除以 z 的絕對值
                // 距離越遠 (z 越小)，除數越大，座標越靠近中心
                float distortion = 1.0f / (0.1f - z_offset);

                p_verts[i] = {
                    t_verts[i].x * distortion * 400 + W / 2,
                    H / 2 - t_verts[i].y * distortion * 400,
                    t_verts[i].z // 深度測試仍使用原始變換後的 z
                };
            }

            // Bounding Box & Rasterization (與之前邏輯相同)
            int minX = std::max(0, (int)std::min({ p_verts[0].x, p_verts[1].x, p_verts[2].x }));
            int maxX = std::min(W - 1, (int)std::max({ p_verts[0].x, p_verts[1].x, p_verts[2].x }));
            int minY = std::max(0, (int)std::min({ p_verts[0].y, p_verts[1].y, p_verts[2].y }));
            int maxY = std::min(H - 1, (int)std::max({ p_verts[0].y, p_verts[1].y, p_verts[2].y }));

            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    float u, v, w;
                    barycentric(p_verts[0], p_verts[1], p_verts[2], { (float)x, (float)y, 0 }, u, v, w);
                    if (u >= 0 && v >= 0 && w >= 0) {
                        float z = u * p_verts[0].z + v * p_verts[1].z + w * p_verts[2].z;
                        int idx = y * W + x;
                        if (z > depthBuffer[idx]) {
                            depthBuffer[idx] = z;
                            int fb_idx = idx * 3;
                            frameBuffer[fb_idx] = (unsigned char)(u * colors[0].r + v * colors[1].r + w * colors[2].r);
                            frameBuffer[fb_idx + 1] = (unsigned char)(u * colors[0].g + v * colors[1].g + w * colors[2].g);
                            frameBuffer[fb_idx + 2] = (unsigned char)(u * colors[0].b + v * colors[1].b + w * colors[2].b);
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