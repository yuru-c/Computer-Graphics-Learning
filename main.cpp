#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <limits>

struct Color {
    float r, g, b;
};

struct Vec3f {
    float x, y, z;
};

void barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p, float& u, float& v, float& w) {
    float det = (float)((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

int main() {
    const int width = 500;
    const int height = 500;

    std::vector<unsigned char> frameBuffer(width * height * 3, 30);

    // --- 新增：深度緩衝區 (Z-Buffer) ---
    // 預設每個像素的距離都是無限遠 (std::numeric_limits<float>::max())
    std::vector<float> depthBuffer(width * height, std::numeric_limits<float>::max());

    // 定義兩個三角形 (T1: 較近, T2: 較遠)
    // 注意 z 值：z 越小代表離相機越近
    Vec3f t1_v0 = { 200, 100, 0.2f }, t1_v1 = { 100, 400, 0.2f }, t1_v2 = { 400, 400, 0.2f };
    Color t1_c = { 255, 105, 180 }; // 粉紅色

    Vec3f t2_v0 = { 300, 50, 0.8f }, t2_v1 = { 150, 450, 0.8f }, t2_v2 = { 480, 450, 0.8f };
    Color t2_c = { 100, 200, 255 }; // 天藍色

    auto draw_tri = [&](Vec3f v0, Vec3f v1, Vec3f v2, Color col) {
        int minX = std::max(0, (int)std::min({ v0.x, v1.x, v2.x }));
        int maxX = std::min(width - 1, (int)std::max({ v0.x, v1.x, v2.x }));
        int minY = std::max(0, (int)std::min({ v0.y, v1.y, v2.y }));
        int maxY = std::min(height - 1, (int)std::max({ v0.y, v1.y, v2.y }));

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float u, v, w;
                barycentric(v0, v1, v2, { (float)x, (float)y, 0 }, u, v, w);

                if (u >= 0 && v >= 0 && w >= 0) {
                    
                    float z = u * v0.z + v * v1.z + w * v2.z;

                    int idx = y * width + x;
                    
                    if (z < depthBuffer[idx]) {
                        depthBuffer[idx] = z;
                                                
                        int f_idx = idx * 3;
                        frameBuffer[f_idx] = (unsigned char)col.r;
                        frameBuffer[f_idx + 1] = (unsigned char)col.g;
                        frameBuffer[f_idx + 2] = (unsigned char)col.b;
                    }
                }
            }
        }
        };

    // 先畫比較遠的 T2，再畫比較近的 T1
    draw_tri(t2_v0, t2_v1, t2_v2, t2_c);
    draw_tri(t1_v0, t1_v1, t1_v2, t1_c);

    std::ofstream ofs("result.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write((char*)frameBuffer.data(), frameBuffer.size());
    ofs.close();

    std::cout << "Z-Buffer 渲染成功！" << std::endl;
    return 0;
}