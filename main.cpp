#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

// 1. 基礎結構
struct Vec3f { float x, y, z; };
struct Color { unsigned char r, g, b; };

// 2. 模型讀取器 (讀取 .obj 檔案)
class Model {
private:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int>> faces_;
public:
    Model(const char* filename) {
        std::ifstream in(filename, std::ifstream::in);
        if (in.fail()) {
            std::cerr << "錯誤：找不到模型檔案 " << filename << std::endl;
            return;
        }
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            char trash;
            if (!line.compare(0, 2, "v ")) {
                iss >> trash; Vec3f v;
                iss >> v.x >> v.y >> v.z;
                verts_.push_back(v);
            }
            else if (!line.compare(0, 2, "f ")) {
                std::vector<int> f; int idx, tmp;
                iss >> trash;
                while (iss >> idx >> trash >> tmp >> trash >> tmp) {
                    f.push_back(--idx); // .obj 是 1-based，轉為 0-based
                }
                faces_.push_back(f);
            }
        }
        std::cout << "模型讀取成功，面數：" << faces_.size() << std::endl;
    }
    int nfaces() { return (int)faces_.size(); }
    Vec3f vert(int i) { return verts_[i]; }
    std::vector<int> face(int i) { return faces_[i]; }
};

// 3. 重心座標 (計算點是否在三角形內)
void barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p, float& u, float& v, float& w) {
    float det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(det) < 1e-5) { u = v = w = -1; return; } // 防止除以 0
    u = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / det;
    v = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / det;
    w = 1.0f - u - v;
}

int main() {
    const int width = 800;
    const int height = 800;
    std::vector<unsigned char> frameBuffer(width * height * 3, 20); // 暗灰色背景
    std::vector<float> depthBuffer(width * height, -1e10); // 深度緩衝

    // !!! 注意：這行會去讀取資料夾下的 cube.obj !!!
    Model* model = new Model("cube.obj");

    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f pts[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            // 投影：把模型 (-1~1) 縮放到螢幕中心 (0~800)
            pts[j] = { (v.x + 1.f) * width / 2.f, (v.y + 1.f) * height / 2.f, v.z };
        }

        // 畫三角形 (Bounding Box 掃描)
        int minX = std::max(0, (int)std::min({ pts[0].x, pts[1].x, pts[2].x }));
        int maxX = std::min(width - 1, (int)std::max({ pts[0].x, pts[1].x, pts[2].x }));
        int minY = std::max(0, (int)std::min({ pts[0].y, pts[1].y, pts[2].y }));
        int maxY = std::min(height - 1, (int)std::max({ pts[0].y, pts[1].y, pts[2].y }));

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float u, v, w;
                barycentric(pts[0], pts[1], pts[2], { (float)x, (float)y, 0 }, u, v, w);
                if (u >= 0 && v >= 0 && w >= 0) {
                    float z = pts[0].z * u + pts[1].z * v + pts[2].z * w;
                    if (depthBuffer[y * width + x] < z) {
                        depthBuffer[y * width + x] = z;
                        int idx = (y * width + x) * 3;
                        // 畫成白色
                        frameBuffer[idx] = 255; frameBuffer[idx + 1] = 255; frameBuffer[idx + 2] = 255;
                    }
                }
            }
        }
    }

    std::ofstream ofs("output.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write((char*)frameBuffer.data(), frameBuffer.size());
    std::cout << "渲染完成！請查看 output.ppm" << std::endl;
    return 0;
}