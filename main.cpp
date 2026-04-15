#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

struct Color {
    float r, g, b; // 改用 float 方便計算漸層
};

struct Vec2i {
    int x, y;
};

// 計算三角形重心座標的函式
// 這是圖學中最神聖的公式之一：它可以告訴你點 P 對於頂點 A, B, C 的權重 (u, v, w)
void barycentric(Vec2i a, Vec2i b, Vec2i c, Vec2i p, float& u, float& v, float& w) {
    float det = (float)((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

int main() {
    const int width = 500;
    const int height = 500;
    std::vector<unsigned char> frameBuffer(width * height * 3, 30); // 背景深灰

    // 定義三個頂點的位置
    Vec2i v0 = { 250, 50 }, v1 = { 50, 450 }, v2 = { 450, 450 };
    // 定義三個頂點的顏色：紅、綠、藍
    Color c0 = { 255, 0, 0 }, c1 = { 0, 255, 0 }, c2 = { 0, 0, 255 };

    // 找出包圍盒
    int minX = std::max(0, std::min({ v0.x, v1.x, v2.x }));
    int maxX = std::min(width - 1, std::max({ v0.x, v1.x, v2.x }));
    int minY = std::max(0, std::min({ v0.y, v1.y, v2.y }));
    int maxY = std::min(height - 1, std::max({ v0.y, v1.y, v2.y }));

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float u, v, w;
            barycentric(v0, v1, v2, { x, y }, u, v, w);

            // 如果 u, v, w 都大於 0，代表點在三角形內
            if (u >= 0 && v >= 0 && w >= 0) {
                // 核心關鍵：根據權重混合三種顏色
                int r = (int)(u * c0.r + v * c1.r + w * c2.r);
                int g = (int)(u * c0.g + v * c1.g + w * c2.g);
                int b = (int)(u * c0.b + v * c1.b + w * c2.b);

                int idx = (y * width + x) * 3;
                frameBuffer[idx] = (unsigned char)r;
                frameBuffer[idx + 1] = (unsigned char)g;
                frameBuffer[idx + 2] = (unsigned char)b;
            }
        }
    }

    std::ofstream ofs("result.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write((char*)frameBuffer.data(), frameBuffer.size());
    ofs.close();

    std::cout << "彩色渲染成功！請到 VS Code 查看 result.ppm" << std::endl;
    return 0;
}