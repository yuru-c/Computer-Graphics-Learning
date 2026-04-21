#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

// --- 1. 3D 數學結構 ---
struct Vec3f {
    float x, y, z;
};

// --- 2. 模型讀取類別 ---
class Model {
public:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int>> faces_;

    Model(std::string filename) {
        std::ifstream in(filename.c_str());
        if (in.fail()) {
            create_test_cube(filename); // 如果找不到檔案，自動生一個立方體
            in.open(filename.c_str());
        }
        std::string line;
        while (std::getline(in, line)) {
            // 讀取頂點 v x y z
            if (!line.compare(0, 2, "v ")) {
                std::istringstream s(line.substr(2));
                Vec3f v; s >> v.x >> v.y >> v.z;
                verts_.push_back(v);
            }
            // 讀取面 f v1 v2 v3 ...
            else if (!line.compare(0, 2, "f ")) {
                std::vector<int> f;
                std::istringstream s(line.substr(2));
                std::string temp;
                while (s >> temp) {
                    size_t slash = temp.find('/');
                    int v_idx = std::stoi(temp.substr(0, slash));
                    f.push_back(v_idx - 1); // .obj 索引從 1 開始，轉為從 0 開始
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

// --- 3. 畫線函式 (DDA 演算法) ---
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
            // 畫白色線條 (RGB: 255, 255, 255)
            framebuffer[idx] = 255;
            framebuffer[idx + 1] = 255;
            framebuffer[idx + 2] = 255;
        }
        x += xInc;
        y += yInc;
    }
}

// --- 4. 座標轉換函式 (核心數學) ---
// 這個函式負責將 3D 空間的點進行旋轉，並確保深度 z 軸正確更新
Vec3f transform(Vec3f v, float angleX, float angleY) {
    float x = v.x, y = v.y, z = v.z;

    // A. 繞 Y 軸旋轉 (控制水平轉動)
    float cosY = cos(angleY), sinY = sin(angleY);
    float x_new = x * cosY + z * sinY;
    float z_new = -x * sinY + z * cosY;
    x = x_new;
    z = z_new; // 將更新後的 z 傳給下一步

    // B. 繞 X 軸旋轉 (控制垂直翻轉)
    float cosX = cos(angleX), sinX = sin(angleX);
    float y_new = y * cosX - z * sinX;
    float z_final = y * sinX + z * cosX;
    y = y_new;
    z = z_final;

    return { x, y, z };
}

// --- 5. 主程式 ---
int main() {
    const int width = 500;
    const int height = 500;
    Model model("cube.obj");

    if (model.verts_.empty()) {
        std::cerr << "錯誤：無法讀取模型頂點！" << std::endl;
        return 1;
    }

    // 建立黑色背景的 Framebuffer (RGB 格式)
    std::vector<unsigned char> framebuffer(width * height * 3, 0);

    // 渲染參數
    float angleX = 0.5f;   // X 軸旋轉弧度
    float angleY = 0.8f;   // Y 軸旋轉弧度
    float zoom = 150.0f;   // 縮放倍率

    // ---------------------------------------------------------
    // 💡 階段一：幾何處理 (Geometry Pass)
    // 遍歷所有頂點，進行 3D 變換並投影到 2D 螢幕座標
    // ---------------------------------------------------------
    std::vector<std::pair<int, int>> projected_verts;
    for (int i = 0; i < (int)model.verts_.size(); i++) {
        // 1. 執行 3D 旋轉
        Vec3f v = transform(model.verts_[i], angleX, angleY);

        // 2. 視口變換：將 (-0.5 ~ 0.5) 的空間映射到 (0 ~ 500) 的螢幕座標
        float dist = 2.0f; // 距離
        float factor = dist / (dist - v.z); // 根據深度計算縮放倍率
        int sx = (int)(v.x * factor * zoom + width / 2.0f);
        int sy = (int)(height / 2.0f - v.y * factor * zoom);

        projected_verts.push_back({ sx, sy });
    }

    // ---------------------------------------------------------
    // 💡 階段二：光柵化 (Rasterization Pass)
    // 根據模型的面資訊 (索引)，連接剛剛算好的 2D 頂點
    // ---------------------------------------------------------
    for (int i = 0; i < (int)model.faces_.size(); i++) {
        const std::vector<int>& face = model.faces_[i];
        for (int j = 0; j < (int)face.size(); j++) {
            // 透過索引 face[j] 從快取中直接拿座標，效率最高
            std::pair<int, int> p0 = projected_verts[face[j]];
            std::pair<int, int> p1 = projected_verts[face[(j + 1) % face.size()]];

            line(p0.first, p0.second, p1.first, p1.second, framebuffer, width, height);
        }
    }

    // ---------------------------------------------------------
    // 💡 階段三：檔案輸出 (PPM 格式)
    // ---------------------------------------------------------
    std::ofstream ofs("result.ppm");
    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < (int)framebuffer.size(); i += 3) {
        ofs << (int)framebuffer[i] << " "
            << (int)framebuffer[i + 1] << " "
            << (int)framebuffer[i + 2] << "\n";
    }
    ofs.close();

    std::cout << "渲染完成！請開啟 result.ppm 查看結果。" << std::endl;
    return 0;
}