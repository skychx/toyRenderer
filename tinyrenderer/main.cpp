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

vec3 light_dir(1, 1, 1); // light source
vec3 camera(0, 0, 3);
vec3 eye(1, 1, 3);       // camera position
vec3 center(0, 0, 0);    // camera direction
vec3 up(0, 1, 0);        // camera up vector


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
    vec3 l;               // light direction in normalized device coordinates
    mat<2,3> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<3,3> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3,3> ndc_tri;     // triangle in normalized device coordinates

    virtual vec4 vertex(int iface, int nthvert) {
        // 从 .obj 文件读取三角形顶点数据
        vec4 gl_Vertex = embed<4>(model->vert(iface, nthvert));
        // MVP & Viewport 变换
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;
        // 获取顶点的贴图位置信息
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        // 􏳻􏰓􏰔􏰪􏰫􏰇􏳼􏳽􏳾􏰪􏰫􏳻􏰓􏰔􏰪􏰫􏰇􏳼􏳽􏳾􏰪􏰫􏳻􏰓􏰔􏰪􏰫􏰇􏳼􏳽􏳾􏰪􏰫法线贴图读到的法线 * 变换矩阵的逆转置矩阵
        varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface, nthvert), 0.f)));
        // 对 gl_Vertex 归一化
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        // 因为要做插值，所以光照和贴图都要乘以重心坐标（bar 是重心坐标）
        vec2 uv = varying_uv * bar;
        vec3 bn = (varying_nrm * bar).normalize();
        
        // 从切线空间贴图求解法线
        // 数学推导可见：https://github.com/ssloy/tinyrenderer/wiki/Lesson-6bis-tangent-space-normal-mapping
        // 用三角形的三个特殊点（三个顶点 P_0、P_1、P_2）计算出 A -> AI -> B
        // (P_0->P_1)_x  (P_0->P_1)_y  (P_0->P_1)_z
        // (P_0->P_2)_x  (P_0->P_2)_y  (P_0->P_2)_z
        //         bn_x          bn_y          bn_z
        mat<3,3> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;
        
        // AI 是 A 的逆矩阵
        mat<3,3> AI = A.invert();
        
        vec3 i = AI * vec3(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        vec3 j = AI * vec3(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);
        
        // 切线空间的基
        // 这里的 B 其实就是 TBN_World 矩阵
        mat<3,3> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, bn);
        
        // 光照
        vec3 l = proj<3>(Projection * ModelView * embed<4>(light_dir)).normalize();
        
        // 法线（做了一个基变换，切线空间基向量 * 切线空间的法线向量，得到的是世界坐标系下的法线向量）
        vec3 n = (B * model->normal(uv)).normalize();
        
        // reflected light direction
        vec3 r = (n * (n * l * 2.f) - l).normalize();
        
        // 镜面高亮
        // specular intensity, note that the camera lies on the z-axis (in ndc), therefore simple r.z
        float spec = pow(std::max<float>(r.z, 0.0f), model->specular(uv));
        // 漫反射
        float diff = std::max<float>(0.f, n * l);
        // 固有纹理
        TGAColor c = model->diffuse(uv);
        
        color = c;
        
        // Phong reflection model
        for (int i = 0; i < 3; i++) {
            //   5: ambient component
            //   1: diffuse component
            // 0.6: specular component
            color[i] = std::min<float>(5 + c[i] * (diff + .6 * spec), 255);
        }
        
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
    frame.write_tga_file("output/lesson06_tangent_space_normal_mapping.tga");
//    zbuffer.write_tga_file("output/lesson06_zbuffer.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawModelTriangle();

    return 0;
}
