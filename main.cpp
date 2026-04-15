#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

// 定義顏色結構
struct Color {
    unsigned char r, g, b;
};

// 定義 2D 座標點
struct Vec2i {
    int x, y;
};

// 計算三角形面積的兩倍 (用於判定點是否在三角形內)
float cross_product(Vec2i a, Vec2i b, Vec2i c) {
    return (float)(b.x - a.x) * (c.y - a.y) - (float)(b.y - a.y) * (c.x - a.x);
}

// 實心三角形填色函式
void draw_triangle(Vec2i v0, Vec2i v1, Vec2i v2, Color color, std::vector<Color>& image, int width, int height) {
    // 找出包圍盒 (Bounding Box)
    int min_x = std::max(0, std::min({ v0.x, v1.x, v2.x }));
    int max_x = std::min(width - 1, std::max({ v0.x, v1.x, v2.x }));
    int min_y = std::max(0, std::min({ v0.y, v1.y, v2.y }));
    int max_y = std::min(height - 1, std::max({ v0.y, v1.y, v2.y }));

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            Vec2i p = { x, y };
            float w0 = cross_product(v1, v2, p);
            float w1 = cross_product(v2, v0, p);
            float w2 = cross_product(v0, v1, p);

            // 判定點在三角形內
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    image[y * width + x] = color;
                }
            }
        }
    }
}

int main() {
    const int width = 500;
    const int height = 500;
    std::vector<Color> image(width * height, { 30, 30, 30 }); // 深灰背景

    // 畫一個粉紅色的三角形
    Vec2i v0 = { 250, 50 }, v1 = { 50, 400 }, v2 = { 450, 400 };
    draw_triangle(v0, v1, v2, { 255, 105, 180 }, image, width, height);

    // 輸出成 PPM 圖片檔
    std::ofstream ofs("result.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < width * height; i++) {
        ofs << image[i].r << image[i].g << image[i].b;
    }
    ofs.close();

    std::cout << "渲染成功！請到資料夾查看 result.ppm" << std::endl;
    return 0;
}