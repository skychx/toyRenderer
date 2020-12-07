//
//  model.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), norms_(), uv_()  {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        
        // istringstream 可以用于分割被空格、制表符等符号分割的字符串
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            // v -0.000581696 -0.734665 -0.623267
            // 几何顶点
            iss >> trash;
            vec3 v;
            for (int i = 0; i < 3; i++) {
                iss >> v[i];
            }
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vn ")) {
            // vn  0.001 0.482 -0.876
            // 顶点法线
            iss >> trash >> trash;
            vec3 n;
            for (int i = 0; i < 3; i++) {
                iss >> n[i];
            }
            norms_.push_back(n);
        } else if (!line.compare(0, 3, "vt ")) {
            // vt  0.395 0.584 0.000
            // 贴图坐标，贴图坐标的范围为第一象限 [0, 1] 内的浮点数
            iss >> trash >> trash;
            vec2 uv;
            for (int i = 0; i < 2; i++) {
                iss >> uv[i];
            }
            uv_.push_back(uv);
        } else if (!line.compare(0, 2, "f ")) {
            // f 24/1/24 25/2/25 26/3/26
            // 面，先记录了三个三角形顶点，然后使用顶点(v)，纹理(vt)和法线索引(vn)的列表来定义面
            std::vector<vec3> f;
            vec3 tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) {
                    // in wavefront obj all indices start at 1, not zero
                    // 索引从 1 开始
                    tmp[i]--;
                }
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i = 0; i < (int)faces_[idx].size(); i++) {
        face.push_back(faces_[idx][i][0]);
    }

    return face;
}

vec3 Model::vert(int i) {
    return verts_[i];
}

// 加载纹理贴图
void Model::load_texture(std::string filename, const char *suffix, TGAImage &img) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    // 判断 filename 是否非空
    if (dot != std::string::npos) {
        texfile = texfile.substr(0,dot) + std::string(suffix); // 拼接出纹理路径
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}

// 获取某个纹理坐标对应的纹理颜色
TGAColor Model::diffuse(vec2 uv) {
    return diffusemap_.get(uv.x, uv.y);
}

// uv_ 映射到纹理贴图中的真实位置
vec2 Model::uv(int iface, int nvert) {
    int idx = faces_[iface][nvert][1];
    return vec2(uv_[idx].x * diffusemap_.get_width(), uv_[idx].y * diffusemap_.get_height());
}

vec3 Model::norm(int iface, int nvert) {
    int idx = faces_[iface][nvert][2];
    return norms_[idx].normalize();
}

