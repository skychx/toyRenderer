//
//  model.hpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<vec3> verts_;
    std::vector<std::vector<vec3> > faces_; // attention, this Vec3i means vertex/uv/normal
    std::vector<vec3> norms_; // 法线
    std::vector<vec2> uv_;    // uv 贴图向量
    TGAImage diffusemap_;      // 纹理 map
    TGAImage normalmap_;
    TGAImage specularmap_;
    void load_texture(std::string filename, const char *suffix, TGAImage &img);
public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    vec3 normal(int iface, int nvert);
    vec3 normal(vec2 uv);
    vec3 norm(int iface, int nvert);
    vec3 vert(int i);
    vec3 vert(int iface, int nvert);
    vec2 uv(int iface, int nvert);
    TGAColor diffuse(vec2 uv);
    std::vector<int> face(int idx);
};

#endif //__MODEL_H__
