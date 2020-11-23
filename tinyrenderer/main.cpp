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

// 思路很简单，点连成线
void line(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color) {
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
bool isInside(Vec3f *pts, Vec2i P) {
    Vec2i AB(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
    Vec2i BC(pts[2].x - pts[1].x, pts[2].y - pts[1].y);
    Vec2i CA(pts[0].x - pts[2].x, pts[0].y - pts[2].y);

    Vec2i AP(P.x - pts[0].x, P.y - pts[0].y);
    Vec2i BP(P.x - pts[1].x, P.y - pts[1].y);
    Vec2i CP(P.x - pts[2].x, P.y - pts[2].y);

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
Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    
    Vec3f x(C[0] - A[0], B[0] - A[0], A[0] - P[0]); // AB AC PA 在 x 上的分量
    Vec3f y(C[1] - A[1], B[1] - A[1], A[1] - P[1]); // AB AC PA 在 y 上的分量

    Vec3f u = cross(x, y); // u 向量和 x y 点乘都为 0，所以 u 垂直于 xy 平面
    
    // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
    // 根据 u[2] 判断法线是向内的还是向外的，向内的抛弃，向外的保留并归一化
    if (std::abs(u[2]) > 1e-2) {
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }
    
    // in this case generate negative coordinates, it will be thrown away by the rasterizator
    return Vec3f(-1, 1, 1);
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
        // 第一层循环，遍历 pts
        for (int j = 0; j < 2; j++) {
            // 第二层循环，遍历 Vec2i
            boxmin.x = std::max(0.f,        std::min(boxmin.x, pts[i].x));
            boxmin.y = std::max(0.f,        std::min(boxmin.y, pts[i].y));
            
            boxmax.x = std::min(clamp.x, std::max(boxmax.x, pts[i].x));
            boxmax.y = std::min(clamp.y, std::max(boxmax.y, pts[i].y));
        }
    }
    
//    std::cout << "boxmin: " << boxmin << "boxmax: "  << boxmax << std::endl;
    
    // 步骤二：对包围盒里的每一个像素进行遍历
    Vec3f P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
            // if (isInside(pts, P)) {
            //     image.set(P.x, P.y, color);
            // }
            Vec3f bc = barycentric(pts[0], pts[1], pts[2], P); // bc 是 Barycentric Coordinates 的缩写

            // 重心坐标某一项小于 0，说明在三角形外，跳过不绘制
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) {
                continue;
            }
            
            // 计算当前像素的 zbuffer
            P.z = 0;
            for (int i = 0; i < 3; i++) {
                P.z += pts[i][2] * bc[i];
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

float lightIntensity(Vec3f n) {
    // 这个是用一个模拟光照对三角形进行着色
    Vec3f light_dir(0, 0, -1); // 假设光是垂直屏幕的
    
    // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
    // Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
    n.normalize(); // 对 n 做归一化处理

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
    for (int i = width * height;
         i--;
         zbuffer[i] = -std::numeric_limits<float>::max()
    );
    
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
        
        // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
        Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
        
        float intensity = lightIntensity(n);
        
        // 因为 zbuffer 已经记录深度值了，根据光照角度的不严谨判断就可以去掉了
        // （加上也无妨，因为可以节省一些分支运算，但是本教程为原理解释，不多考虑性能问题）
        // if (intensity > 0) {
        //     triangle(screen_coords, zbuffer, frame, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        // }
        triangle(screen_coords, zbuffer, frame, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
    }
    
    frame.flip_vertically();
    frame.write_tga_file("output/lesson03_z_buffer.tga");
    
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
