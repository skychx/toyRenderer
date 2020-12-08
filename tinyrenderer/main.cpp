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
    vec3 varying_intensity; // written by vertex shader, read by fragment shader

    virtual vec4 vertex(int iface, int nthvert) {
        // read the vertex from .obj file
        vec4 gl_Vertex = embed<4>(model->vert(iface, nthvert));
        // transform it to screen coordinates
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
        // get diffuse lighting intensity
        varying_intensity[nthvert] = std::max<float>(0.f, model->normal(iface, nthvert) * light_dir);
        return gl_Vertex;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
        color = TGAColor(255, 255, 255) * intensity; // well duh
        return false;                              // no, we do not discard this pixel
    }
};


void drawModelTriangle() {
    model = new Model("obj/african_head.obj");

    
    lookat(eye, center, vec3(0, 1, 0));       // build the ModelView matrix
    projection(-1.f / (eye - center).norm()); // build the Projection matrix

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
    frame.write_tga_file("output/lesson06_gouraud_shading.tga");
    zbuffer.write_tga_file("output/lesson06_zbuffer.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawModelTriangle();

    return 0;
}
