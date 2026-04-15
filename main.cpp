#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// 定義頂點結構
struct Vec3f { float x, y, z; };

class Model {
public:
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int>> faces_;

    Model(const char* filename) {
        std::ifstream in(filename);
        if (in.fail()) {
            std::cout << "找不到檔案: " << filename << "，正在嘗試自動建立..." << std::endl;
            create_test_cube(filename);
            in.open(filename);
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
                int idx;
                std::istringstream s(line.substr(2));
                while (s >> idx) {
                    f.push_back(--idx); // .obj 索引從 1 開始，轉為從 0 開始
                }
                faces_.push_back(f);
            }
        }
    }

    // 如果沒檔案，自動生一個方塊給它讀
    void create_test_cube(const char* filename) {
        std::ofstream out(filename);
        out << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv 0 0 1\nv 1 0 1\nv 1 1 1\nv 0 1 1\n"
            << "f 1 2 3\nf 1 3 4\nf 5 6 7\nf 5 7 8\nf 1 2 6\nf 1 6 5\nf 2 3 7\nf 2 7 6\n"
            << "f 3 4 8\nf 3 8 7\nf 4 1 5\nf 4 5 8\n";
        out.close();
    }
};

int main() {
    // 1. 讀取模型
    Model m("cube.obj");

    // 2. 檢查結果
    if (m.verts_.empty()) {
        std::cout << "讀取失敗！" << std::endl;
    }
    else {
        std::cout << "--- 3D 模型讀取成功 ---" << std::endl;
        std::cout << "頂點數量: " << m.verts_.size() << std::endl;
        std::cout << "面數量: " << m.faces_.size() << std::endl;
        std::cout << "-----------------------" << std::endl;

        // 印出第一個面做測試
        if (!m.faces_.empty()) {
            std::cout << "第一個面的頂點索引為: ";
            for (int i : m.faces_[0]) std::cout << i << " ";
            std::cout << std::endl;
        }
    }

    std::cout << "\n按 Enter 鍵結束..." << std::endl;
    std::cin.get();
    return 0;
}