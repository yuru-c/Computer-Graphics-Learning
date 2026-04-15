#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath> // 為了 cos 和 sin

// --- 3D 數學結構 ---
struct Vec3f {
    float x, y, z;
};

// --- 模型讀取類別 ---
class Model {
public:
    std::vector<Vec3f> verts_;
    Model(std::string filename) {
        std::ifstream in(filename.c_str());
        if (in.fail()) {
            create_test_cube(filename);
            in.open(filename.c_str());
        }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.compare(0, 2, "v ")) {
                std::istringstream s(line.substr(2));
                Vec3f v; s >> v.x >> v.y >> v.z;
                verts_.push_back(v);
            }
        }
    }
    void create_test_cube(std::string filename) {
        std::ofstream out(filename.c_str());
        out << "v -0.5 -0.5 0.5\nv 0.5 -0.5 0.5\nv 0.5 0.5 0.5\nv -0.5 0.5 0.5\n"
            << "v -0.5 -0.5 -0.5\nv 0.5 -0.5 -0.5\nv 0.5 0.5 -0.5\nv -0.5 0.5 -0.5\n";
        out.close();
    }
};

int main() {
    const int width = 500;
    const int height = 500;
    Model model("cube.obj");

    if (model.verts_.empty()) {
        std::cerr << "讀取失敗！" << std::endl;
        return 1;
    }

    std::vector<unsigned char> framebuffer(width * height * 3, 0);

    // 設定旋轉角度（讓 8 個點不要重疊）
    float angleX = 0.4f;
    float angleY = 0.6f;

    for (int i = 0; i < (int)model.verts_.size(); i++) {
        Vec3f v = model.verts_[i];

        // 1. 繞 Y 軸旋轉
        float nx = v.x * cos(angleY) + v.z * sin(angleY);
        float nz = -v.x * sin(angleY) + v.z * cos(angleY);

        // 2. 繞 X 軸旋轉
        float ny = v.y * cos(angleX) - nz * sin(angleX);
        float rz = v.y * sin(angleX) + nz * cos(angleX); // 最終旋轉後的 Z（此處投影暫不用）

        // 3. 投影到 2D 平面 (加上放大倍率 zoom)
        float zoom = 150.0f;
        int x = (int)(nx * zoom + width / 2.0f);
        int y = (int)(height / 2.0f - ny * zoom);

        // 4. 畫點 (為了讓點明顯一點，我們畫 3x3 的小方塊代表一個頂點)
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                int px = x + ox;
                int py = y + oy;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    int idx = (py * width + px) * 3;
                    framebuffer[idx] = 255; framebuffer[idx + 1] = 255; framebuffer[idx + 2] = 255;
                }
            }
        }
    }

    std::ofstream ofs("result.ppm");
    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < width * height * 3; i += 3) {
        ofs << (int)framebuffer[i] << " " << (int)framebuffer[i + 1] << " " << (int)framebuffer[i + 2] << "\n";
    }
    ofs.close();

    std::cout << "完成！讀取頂點數: " << model.verts_.size() << std::endl;
    std::cout << "請打開 result.ppm 查看旋轉後的 8 個頂點。" << std::endl;

    return 0;
}