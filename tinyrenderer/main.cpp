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

vec3 light_dir = vec3(1, -1, 1).normalize();
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


// 利用重心坐标求解，返回三角形的重心坐标
vec3 barycentric(vec3 A, vec3 B, vec3 C, vec3 P) {
    
    vec3 x(C[0] - A[0], B[0] - A[0], A[0] - P[0]); // AB AC PA 在 x 上的分量
    vec3 y(C[1] - A[1], B[1] - A[1], A[1] - P[1]); // AB AC PA 在 y 上的分量

    vec3 u = cross(x, y); // u 向量和 x y 点乘都为 0，所以 u 垂直于 xy 平面
    
    // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
    // 根据 u[2] 判断法线是向内的还是向外的，向内的抛弃，向外的保留并归一化
    if (std::abs(u[2]) > 1e-2) {
        return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }
    
    // in this case generate negative coordinates, it will be thrown away by the rasterizator
    return vec3(-1, 1, 1);
}

// 自己实现的三角形光栅化函数
// 主要思路是利用重心坐标判断点是否在三角形内
void triangle(vec3 *pts, float *intensity, float *zbuffer,  TGAImage &image) {
    // 步骤 1: 找出包围盒
    vec2 boxmin(image.get_width() - 1, image.get_height() - 1);
    vec2 boxmax(0, 0);
    vec2 clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界
    // 查找包围盒边界
    for (int i = 0; i < 3; i++) {
        // 第一层循环，遍历 pts
        for (int j = 0; j < 2; j++) {
            // 第二层循环，遍历 Vec2i
            boxmin.x = std::max<int>(0, std::min(boxmin.x, pts[i].x));
            boxmin.y = std::max<int>(0, std::min(boxmin.y, pts[i].y));
            
            boxmax.x = std::min(clamp.x, std::max(boxmax.x, pts[i].x));
            boxmax.y = std::min(clamp.y, std::max(boxmax.y, pts[i].y));
        }
    }
    
//    std::cout << "boxmin: " << boxmin << "boxmax: "  << boxmax << std::endl;
    
    // 步骤二：对包围盒里的每一个像素进行遍历
    vec3 P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
            // if (isInside(pts, P)) {
            //     image.set(P.x, P.y, color);
            // }
            vec3 bc = barycentric(pts[0], pts[1], pts[2], P); // bc 是 Barycentric Coordinates 的缩写

            // 重心坐标某一项小于 0，说明在三角形外，跳过不绘制
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) {
                continue;
            }
            
            // 计算当前像素的 zbuffer
            P.z = 0;
            for (int i = 0; i < 3; i++) {
                P.z += pts[i][2] * bc[i];
            }
            

            float ityP = 0.;
            for (int i = 0; i < 3; i++) {
                ityP += intensity[i] * bc[i];
            }
            
            
            // 更新总的 zbuffer 并绘制
            if (zbuffer[int(P.x + P.y * WIDTH)] < P.z) {
                zbuffer[int(P.x + P.y * WIDTH)] = P.z;
                image.set(P.x, P.y, TGAColor(255, 255, 255) * ityP);
            }
        }
    }
}

// 矩阵 -> 向量
vec3 m2v(mat<4,4> m) {
    return vec3(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

// 向量 -> 矩阵
mat<4,4> v2m(vec3 v) {
    mat<4,4> m;
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

void drawModelTriangle() {
    model = new Model("obj/african_head.obj");
    TGAImage frame(WIDTH, HEIGHT, TGAImage::RGB);
    
    // 初始化 zbuffer
    // 按道理来说 zbuffer 应该是个二维向量，这里只是用一维表示二维
    // [[1, 2, 3],       [1, 2, 3,
    //  [4, 5, 6],   =>   4, 5, 6,
    //  [7, 8, 9]]        7, 8, 9],
    float *zbuffer = new float[WIDTH * HEIGHT];
    for (int i = WIDTH * HEIGHT;
         i--;
         zbuffer[i] = -std::numeric_limits<float>::max()
    );
    
    lookat(eye, center, vec3(0, 1, 0));       // build the ModelView matrix
    projection(-1.f / (eye - center).norm()); // build the Projection matrix

    // 其实这里用 viewport(0, 0, WIDTH, HEIGHT) 就可以，这样渲染的图像会撑满整个屏幕
    // 乘以 3/4 后再平移 1/8 的距离，就可以把图像摆到图片中央
    viewport(WIDTH / 8, HEIGHT / 8, WIDTH * 3/4, HEIGHT * 3/4); // build the Viewport matrix
    
    // 遍历所有三角形
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        vec3 screen_coords[3];
        vec3 world_coords[3];
        float intensity[3];
        
        // 每个三角形的顶点都是一个三维向量
        for (int j = 0; j < 3; j++) {
            vec3 v = model->vert(face[j]);
            world_coords[j]  = v;
            screen_coords[j] = m2v(Viewport * Projection * ModelView * v2m(v)); // 透视投影
            intensity[j] = model->norm(i, j) * light_dir;
        }
        
        // 着色时同时考虑光照和 zbuffer，渲染效果会好一些
        triangle(screen_coords, intensity, zbuffer, frame);
    }
    
    frame.flip_vertically();
    frame.write_tga_file("output/lesson05_gouraud_shading.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawModelTriangle();

    return 0;
}
