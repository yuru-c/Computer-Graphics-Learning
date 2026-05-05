#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

struct Vec3f {
    float x, y, z;
    // 向量減法
    Vec3f operator-(const Vec3f& v) const { return { x - v.x, y - v.y, z - v.z }; }
};

struct Color { float r, g, b; };

// 重心座標計算
void barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p, float& u, float& v, float& w) {
    float det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(det) < 1e-6) { u = v = w = -1; return; }
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

// 向量正規化
Vec3f normalize(Vec3f v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (len > 0) ? Vec3f{ v.x / len, v.y / len, v.z / len } : v;
}

// 向量點積
float dot(Vec3f a, Vec3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 3D 旋轉轉換
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
    GLFWwindow* window = glfwCreateWindow(W, H, "3D Engine: Lighting & Perspective", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 立方體頂點與顏色
    std::vector<Vec3f> verts = {
        {-0.5,-0.5,0.5}, {0.5,-0.5,0.5}, {0.5,0.5,0.5}, {-0.5,0.5,0.5},
        {-0.5,-0.5,-0.5}, {0.5,-0.5,-0.5}, {0.5,0.5,-0.5}, {-0.5,0.5,-0.5}
    };
    std::vector<Color> vertColors = {
        {255,100,100}, {100,255,100}, {100,100,255}, {255,255,100},
        {255,100,255}, {100,255,255}, {255,255,255}, {150,150,150}
    };
    std::vector<std::vector<int>> faces = {
        {0,1,2}, {0,2,3}, {4,7,6}, {4,6,5}, {0,4,5}, {0,5,1},
        {1,5,6}, {1,6,2}, {2,6,7}, {2,7,3}, {4,0,3}, {4,3,7}
    };

    std::vector<unsigned char> frameBuffer(W * H * 3, 0);
    std::vector<float> depthBuffer(W * H, -std::numeric_limits<float>::max());

    // 定義光源方向 (從攝影機右上方射向物體)
    Vec3f light_dir = normalize({ 0.5f, 0.5f, 1.0f });

    float angleX = 0.5f, angleY = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        angleY += 0.015f;
        std::fill(frameBuffer.begin(), frameBuffer.end(), 20); // 暗深色背景
        std::fill(depthBuffer.begin(), depthBuffer.end(), -std::numeric_limits<float>::max());

        for (auto& f : faces) {
            Vec3f t_verts[3], p_verts[3];
            Color colors[3];

            for (int i = 0; i < 3; i++) {
                t_verts[i] = transform(verts[f[i]], angleX, angleY);
                colors[i] = vertColors[f[i]];

                // 透視投影
                float z_offset = t_verts[i].z - 2.5f;
                float distortion = 1.0f / (0.1f - z_offset);
                p_verts[i] = { t_verts[i].x * distortion * 500 + W / 2, H / 2 - t_verts[i].y * distortion * 500, t_verts[i].z };
            }

            // --- 核心：計算法向量 (Normal) ---
            Vec3f edge1 = t_verts[1] - t_verts[0];
            Vec3f edge2 = t_verts[2] - t_verts[0];
            Vec3f normal = normalize({
                edge1.y * edge2.z - edge1.z * edge2.y,
                edge1.z * edge2.x - edge1.x * edge2.z,
                edge1.x * edge2.y - edge1.y * edge2.x
                });

            // --- 核心：計算光照強度 (Diffuse) ---
            float intensity = dot(normal, light_dir);
            if (intensity < 0) intensity = 0; // 背光面
            float lighting = intensity * 0.85f + 0.15f; // 加上 Ambient (環境光)

            // Bounding Box Rasterization
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
                            // 將光照強度乘到顏色上
                            frameBuffer[fb_idx] = (unsigned char)((u * colors[0].r + v * colors[1].r + w * colors[2].r) * lighting);
                            frameBuffer[fb_idx + 1] = (unsigned char)((u * colors[0].g + v * colors[1].g + w * colors[2].g) * lighting);
                            frameBuffer[fb_idx + 2] = (unsigned char)((u * colors[0].g + v * colors[2].b + w * colors[2].b) * lighting);
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