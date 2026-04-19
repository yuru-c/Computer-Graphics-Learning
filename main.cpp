#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

// --- 3D 數學結構 ---
struct Vec3f {
    float x, y, z;
};

// --- 模型讀取類別 ---
class Model {
public:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int>> faces_;

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
            else if (!line.compare(0, 2, "f ")) {
                std::vector<int> f;
                int vertex_index;
                std::istringstream s(line.substr(2));
                std::string temp;
                while (s >> temp) {
                    // 只抓第一個數字，處理 1/1/1 或 1 這種格式
                    size_t slash = temp.find('/');
                    int v_idx = std::stoi(temp.substr(0, slash));
                    f.push_back(v_idx - 1);
                }
                faces_.push_back(f);
            }
        }
    }

    void create_test_cube(std::string filename) {
        std::ofstream out(filename.c_str());
        out << "v -0.5 -0.5 0.5\nv 0.5 -0.5 0.5\nv 0.5 0.5 0.5\nv -0.5 0.5 0.5\n"
            << "v -0.5 -0.5 -0.5\nv 0.5 -0.5 -0.5\nv 0.5 0.5 -0.5\nv -0.5 0.5 -0.5\n";
        out << "f 1 2 3 4\nf 5 6 7 8\nf 1 2 6 5\nf 2 3 7 6\nf 3 4 8 7\nf 4 1 5 8\n";
        out.close();
    }
};

// --- 畫線函式 ---
void line(int x0, int y0, int x1, int y1, std::vector<unsigned char>& framebuffer, int width, int height) {
    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return;

    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = (float)x0;
    float y = (float)y0;

    for (int i = 0; i <= (int)steps; i++) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int idx = ((int)y * width + (int)x) * 3;
            framebuffer[idx] = 255;
            framebuffer[idx + 1] = 255;
            framebuffer[idx + 2] = 255;
        }
        x += xInc;
        y += yInc;
    }
}

int main() {
    const int width = 500;
    const int height = 500;
    Model model("cube.obj");

    if (model.verts_.empty()) {
        std::cerr << "讀取失敗！" << std::endl;
        return 1;
    }

    std::vector<unsigned char> framebuffer(width * height * 3, 0);

    // 旋轉角度
    float angleX = 0.5f;
    float angleY = 0.8f;
    float zoom = 150.0f;

    for (int i = 0; i < (int)model.faces_.size(); i++) {
        const std::vector<int>& face = model.faces_[i];
        for (int j = 0; j < (int)face.size(); j++) {
            // 取出 A 點與 B 點
            Vec3f v0_raw = model.verts_[face[j]];
            Vec3f v1_raw = model.verts_[face[(j + 1) % face.size()]];

            // 旋轉投影 A 點
            float nx0 = v0_raw.x * cos(angleY) + v0_raw.z * sin(angleY);
            float nz0 = -v0_raw.x * sin(angleY) + v0_raw.z * cos(angleY);
            float ny0 = v0_raw.y * cos(angleX) - nz0 * sin(angleX);
            int x0 = (int)(nx0 * zoom + width / 2.0f);
            int y0 = (int)(height / 2.0f - ny0 * zoom);

            // 旋轉投影 B 點
            float nx1 = v1_raw.x * cos(angleY) + v1_raw.z * sin(angleY);
            float nz1 = -v1_raw.x * sin(angleY) + v1_raw.z * cos(angleY);
            float ny1 = v1_raw.y * cos(angleX) - nz1 * sin(angleX);
            int x1 = (int)(nx1 * zoom + width / 2.0f);
            int y1 = (int)(height / 2.0f - ny1 * zoom);

            line(x0, y0, x1, y1, framebuffer, width, height);
        }
    }

    std::ofstream ofs("result.ppm");
    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < width * height * 3; i += 3) {
        ofs << (int)framebuffer[i] << " " << (int)framebuffer[i + 1] << " " << (int)framebuffer[i + 2] << "\n";
    }
    ofs.close();

    std::cout << "完成！請手動刪除舊 cube.obj 後再執行。" << std::endl;
    return 0;
}