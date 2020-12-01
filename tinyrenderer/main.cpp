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
const int depth  = 255;

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

// 利用叉乘判断是否在三角形内部
bool isInside(vec3 *pts, vec2 P) {
    vec2 AB(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
    vec2 BC(pts[2].x - pts[1].x, pts[2].y - pts[1].y);
    vec2 CA(pts[0].x - pts[2].x, pts[0].y - pts[2].y);

    vec2 AP(P.x - pts[0].x, P.y - pts[0].y);
    vec2 BP(P.x - pts[1].x, P.y - pts[1].y);
    vec2 CP(P.x - pts[2].x, P.y - pts[2].y);

    // 叉乘计算
    if(
       (AB.x * AP.y - AP.x * AB.y) >= 0 &&
       (BC.x * BP.y - BP.x * BC.y) >= 0 &&
       (CA.x * CP.y - CP.x * CA.y) >= 0
    ) {
        return true;
    }
    return false;
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
void triangle(vec3 *pts, vec2 *uv, float *zbuffer, float intensity, TGAImage &image) {
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
            
            // 计算当前像素的 uv
            vec2 uvP;
            for (int i = 0; i < 3; i++) {
                uvP.x += uv[i][0] * bc[i];
                uvP.y += uv[i][1] * bc[i];
            }
            
            
            // 更新总的 zbuffer 并绘制
            if (zbuffer[int(P.x + P.y * width)] < P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, TGAColor(intensity * color.r, intensity * color.g, intensity * color.b, 255));
            }
        }
    }
}

vec3 world2screen(vec3 v) {
    return vec3(int((v.x + 1.0) * width / 2.0 + 0.5), int((v.y + 1.0) * height / 2.0 + 0.5), v.z);
}

float lightIntensity(vec3 *world_coords) {
    // 假设光是垂直屏幕的
    // 这个是用一个模拟光照对三角形进行着色
    vec3 light_dir(0, 0, -1);
    
    // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
    vec3 n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
    
    // 对 n 做归一化处理
    n.normalize();

    // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
    // 值越大，说明越多的光照射到三角形上，颜色越白
    return n * light_dir;
}

void drawModelTriangle() {
    model = new Model("obj/african_head.obj");
    TGAImage frame(width, height, TGAImage::RGB);
    
    // 初始化 zbuffer
    // 按道理来说 zbuffer 应该是个二维向量，这里只是用一维表示二维
    // [[1, 2, 3],       [1, 2, 3,
    //  [4, 5, 6],   =>   4, 5, 6,
    //  [7, 8, 9]]        7, 8, 9],
    float *zbuffer = new float[width * height];
    for (int i = width * height;
         i--;
         zbuffer[i] = -std::numeric_limits<float>::max()
    );
    
    // 遍历所有三角形
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        vec3 screen_coords[3];
        vec3 world_coords[3];
        
        // 每个三角形的顶点都是一个三维向量
        for (int j = 0; j < 3; j++) {
            vec3 v = model->vert(face[j]);
            world_coords[j]  = v;
            screen_coords[j] = world2screen(v); // 正交投影，而且只做了个简单的视口变换
        }
        
        // 计算光照强度
        float intensity = lightIntensity(world_coords);
        
        // 着色时同时考虑光照和 zbuffer，渲染效果会好一些
        if (intensity > 0) {
            
            // 拿出三个顶点的 uv 坐标
            vec2 uv[3];
            for (int k = 0; k < 3; k++) {
                uv[k] = model->uv(i, k);
            }
            triangle(screen_coords, uv, zbuffer, intensity, frame);
        }
    }
    
    frame.flip_vertically();
    frame.write_tga_file("output/lesson03_diffuse_texture.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawModelTriangle();
//    drawGeometry();

    return 0;
}
