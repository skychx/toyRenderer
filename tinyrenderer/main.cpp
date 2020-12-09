//
//  main.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model = NULL;
const int WIDTH  = 800;
const int HEIGHT = 800;

vec3 light_dir(1, 1, 1);
vec3 camera(0, 0, 3);
vec3 eye(1, 1, 3);
vec3 center(0, 0, 0);
vec3 up(0, 1, 0);


extern mat<4,4> ModelView;
extern mat<4,4> Projection;
extern mat<4,4> Viewport;

// 思路很简单，点连成线
void line(vec3 p0, vec3 p1, TGAImage &image, TGAColor color) {
    // 处理比较陡的线：交换 x y 位置
    bool steep = false;
    if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
        steep = true;
    }
    // 处理 x0 > x1 的情况
    if (p0.x > p1.x) {
        std::swap(p0.x, p1.x);
        std::swap(p0.y, p1.y);
    }
    // 缓存偏移量
    int dx = p1.x - p0.x;
    int dy = p1.y - p0.y;
    // 存储误差
    // float derror = std::abs(dy / float(dx)); // 每次循环增加的小误差
    // float error = 0; // 累积误差
    // 浮点运算肯定没有 int 快，我们想办法去掉浮点运算
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = p0.y;
    for (int x = p0.x; x <= p1.x; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        // 误差的处理：每次误差大于一个像素时，y 就要进位 1，相应的累积误差也要减小 1
        // error += derror;
        // if (error > .5) {
            // y += (p1.y > p0.y ? 1 : -1);
            // error -= 1.;
        // }
        error2 += derror2;
        if (error2 > dx) {
            y += (p1.y > p0.y ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}


struct GouraudShader : public IShader {
    // written by vertex shader, read by fragment shader
    vec3 varying_intensity;
    mat<2,3> varying_uv;

    virtual vec4 vertex(int iface, int nthvert) {
        // 从 .obj 文件读取三角形顶点数据
        vec4 gl_Vertex = embed<4>(model->vert(iface, nthvert));
        // MVP & Viewport 变换
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
        // 获取顶点的光照数据
        varying_intensity[nthvert] = std::max<float>(0.f, model->normal(iface, nthvert) * light_dir);
        // 获取顶点的贴图位置信息
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        return gl_Vertex;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        // 因为要做插值，所以光照和贴图都要乘以重心坐标（bar 是重心坐标）
        float intensity = varying_intensity * bar;
        vec2 uv = varying_uv * bar;
        // 像素着色（考虑贴图和光照因素）
        color = model->diffuse(uv) * intensity;
        // no, we do not discard this pixel
        return false;
    }
};


void drawModelTriangle() {
    model = new Model("obj/african_head.obj");

    // build the ModelView matrix
    lookat(eye, center, up);

    // build the Projection matrix
    projection(-1.f / (eye - center).norm());

    // 其实这里用 viewport(0, 0, WIDTH, HEIGHT) 就可以，这样渲染的图像会撑满整个屏幕
    // 乘以 3/4 后再平移 1/8 的距离，就可以把图像摆到图片中央
    viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3/4, HEIGHT * 3/4); // build the Viewport matrix
    light_dir.normalize();
    
    TGAImage frame(WIDTH, HEIGHT, TGAImage::RGB);
    TGAImage zbuffer(WIDTH, HEIGHT, TGAImage::GRAYSCALE);
    
    // 遍历所有三角形
    GouraudShader shader;
    for (int i = 0; i < model->nfaces(); i++) {
        vec4 screen_coords[3];
        for (int j = 0; j < 3; j++) {
            screen_coords[j] = shader.vertex(i, j);
        }

        triangle(screen_coords, shader, frame, zbuffer);
    }
    
    frame.flip_vertically();
    zbuffer.flip_vertically();
    frame.write_tga_file("output/lesson06_textures.tga");
//    zbuffer.write_tga_file("output/lesson06_zbuffer.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawModelTriangle();

    return 0;
}
