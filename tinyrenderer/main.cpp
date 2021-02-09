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

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255,   0,   0, 255);
const TGAColor green = TGAColor(  0, 255,   0, 255);
const TGAColor blue  = TGAColor(  0,   0, 255, 255);

Model *model = NULL;
const int width  = 800;
const int height = 800;

// 利用重心坐标判断点是否在三角形内部
Vec3f barycentric(Vec3f *pts, Vec3f P) {
    Vec3i x(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x);
    Vec3i y(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);
    
    // u 向量和 x y 向量的点积为 0，所以 x y 向量叉乘可以得到 u 向量
    Vec3i u = cross(x, y);
    
    // 由于 A, B, C, P 的坐标都是 int 类型，所以 u.z 必定是 int 类型，取值范围为 ..., -2, -1, 0, 1, 2, ...
    // 所以 u.z 绝对值小于 1 意味着三角形退化了，需要舍弃
    if(std::abs(u.z) < 1) {
        return Vec3f(-1, 1, 1);
    }
    return Vec3f(1.f- (u.x+u.y) / (float)u.z, u.x / (float)u.z, u.y / (float)u.z);
}

// 自己实现的三角形光栅化函数
// 主要思路是遍历 Box 中的每一个像素，和三角形的三个点做叉乘，如果叉乘均为正，着色；有一个负数，不着色
void triangle(Vec3f *pts, float *zbuffer, TGAImage &image, TGAColor color) {
    // 步骤 1: 找出包围盒
    Vec2f boxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2f boxmax(0, 0);
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界
    // 查找包围盒边界
    for (int i = 0; i < 3; i++) {
        // 第一层循环，遍历三角形的三个顶点
        for (int j = 0; j < 2; j++) {
            // 第二层循环，根据顶点数值缩小包围盒的范围
            boxmin.x = std::max(0.f,        std::min(boxmin.x, pts[i].x));
            boxmin.y = std::max(0.f,        std::min(boxmin.y, pts[i].y));
            
            boxmax.x = std::min(clamp.x, std::max(boxmax.x, pts[i].x));
            boxmax.y = std::min(clamp.y, std::max(boxmax.y, pts[i].y));
        }
    }
    
//    std::cout << "boxmin: " << boxmin << "boxmax: "  << boxmax << std::endl;
    
    // 步骤二：包围盒内的每一个像素和三角形顶点连线做叉乘
    Vec3f P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P); // bc 是 Barycentric Coordinates 的缩写

            // bc_screen 某个分量小于 0 则表示此点在三角形外（认为边也是三角形的一部分）
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
                continue;
            }
            
            // 计算当前像素的 zbuffer
            P.z = 0;
            for (int i = 0; i < 3; i++) {
                P.z += pts[i][2] * bc_screen[i];
            }
            
            // 更新总的 zbuffer 并绘制
            if(zbuffer[int(P.x + P.y * width)] < P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x + 1.0) * width / 2.0 + 0.5), int((v.y + 1.0) * height / 2.0 + 0.5), v.z);
}

float lightIntensity(Vec3f *world_coords) {
    // 假设光是垂直屏幕的
    // 这个是用一个模拟光照对三角形进行着色
    Vec3f light_dir(0, 0, -1);
    
    // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
    Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
    
    // 对 n 做归一化处理
    n.normalize();

    // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
    // 值越大，说明越多的光照射到三角形上，颜色越白
    return n * light_dir;
}

void drawModelTriangle() {
    TGAImage frame(width, height, TGAImage::RGB);
    
    // 初始化 zbuffer
    // 按道理来说 zbuffer 应该是个二维向量，这里只是用一维表示二维
    // [[1, 2, 3],       [1, 2, 3,
    //  [4, 5, 6],   =>   4, 5, 6,
    //  [7, 8, 9]]        7, 8, 9],
    float *zbuffer = new float[width * height];

    for (int i=0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }
    
    // 遍历所有三角形
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        
        // 每个三角形的顶点都是一个三维向量
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            world_coords[j]  = v;
            screen_coords[j] = world2screen(v); // 正交投影，而且只做了个简单的视口变换
        }
        
        // 计算光照强度
        float intensity = lightIntensity(world_coords);
        
        // 着色时同时考虑光照和 zbuffer，渲染效果会好一些
        if (intensity > 0) {
            triangle(screen_coords, zbuffer, frame, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }
    
    frame.flip_vertically();
    frame.write_tga_file("output/day04_z_buffer.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    drawModelTriangle();

    return 0;
}
