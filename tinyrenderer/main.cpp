//
//  main.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include <iostream>
#include <vector>
#include <cmath>
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
bool isInside(Vec2i *pts, Vec2i P) {
    Vec2i AB(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
    Vec2i BC(pts[2].x - pts[1].x, pts[2].y - pts[1].y);
    Vec2i CA(pts[0].x - pts[2].x, pts[0].y - pts[2].y);
    
    Vec2i AP(P.x - pts[0].x, P.y - pts[0].y);
    Vec2i BP(P.x - pts[1].x, P.y - pts[1].y);
    Vec2i CP(P.x - pts[2].x, P.y - pts[2].y);
    
    if((AB^AP) >= 0 && (BC^BP) >= 0 && (CA^CP) >= 0) {
        return true;
    }
    return false;
}

// 自己实现的三角形光栅化函数
// 主要思路是遍历 Box 中的每一个像素，和三角形的三个点做叉乘，如果叉乘均为正，着色；有一个负数，不着色
void triangle(Vec2i *pts, TGAImage &image, TGAColor color) {
    // 步骤 1: 找出包围盒
    Vec2i boxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i boxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界
    // 查找包围盒边界
    for (int i = 0; i < 3; i++) {
        // 第一层循环，遍历 pts
        for (int j = 0; j < 2; j++) {
            // 第二层循环，遍历 Vec2i
            boxmin.x = std::max(0,        std::min(boxmin.x, pts[i].x));
            boxmin.y = std::max(0,        std::min(boxmin.y, pts[i].y));
            
            boxmax.x = std::min(clamp.x, std::max(boxmax.x, pts[i].x));
            boxmax.y = std::min(clamp.y, std::max(boxmax.y, pts[i].y));
        }
    }
    
    std::cout << "boxmin: " << boxmin << "boxmax: "  << boxmax << std::endl;
    
    
    // 步骤 2: 包围盒内的每一个像素和三角形顶点连线做叉乘
    Vec2i P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++) {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++) {
            if (isInside(pts, P)) {
                image.set(P.x, P.y, color);
            }
        }
    }
}

void drawSingleTriangle() {
    TGAImage frame(200, 200, TGAImage::RGB);
    Vec2i pts[3] = {Vec2i(10, 10), Vec2i(150, 30), Vec2i(70, 160)};
    
    triangle(pts, frame, red);
    
    frame.flip_vertically();
    frame.write_tga_file("output/lesson02_self.tga");
}

void rasterize(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color, int ybuffer[]) {
    if (p0.x > p1.x) {
        std::swap(p0, p1);
    }
    
    for (int x = p0.x; x < p1.x; x++) {
        // 这里的公式很简单，利用直线的「两点式」求未知数：
        // t = (x - p0.x) / (p1.x - p0.x) = (y - p0.y) / (p1.y - p0.y)
        // 官方实现中，y = p0.y * (1. - t) + p1.y * t，(1 - t, t)
        // (1-t, t) 这个比例告诉我们，(x, y) 其实是 p0p1 这条线段的重心坐标，这样我们可以很方便的把概念迁移到三角形中
        float t = (x - p0.x) / (float)(p1.x - p0.x);
        int y = (p1.y - p0.y) * t + p0.y + 0.5;
        
        // ybuffer 只记录最大值
        if (y > ybuffer[x]) {
            ybuffer[x] = y;
            image.set(x, 0, color);
        }
    }
}

void drawYbuffer() {
    // 画三条交叉的线
    {
        TGAImage frame(800, 800, TGAImage::RGB);
        
        line(Vec2i(20, 34),   Vec2i(744, 400), frame, red);
        line(Vec2i(120, 434), Vec2i(444, 400), frame, green);
        line(Vec2i(330, 463), Vec2i(594, 200), frame, blue);

        line(Vec2i(10, 10), Vec2i(790, 10), frame, white);
        
        frame.flip_vertically();
        frame.write_tga_file("output/lesson03_y_buffer_line.tga");
    }
    
    // 根据上面的三条线绘制 Y-buffer
    {
        int width = 800;
        TGAImage ybufferRender(width, 16, TGAImage::RGB);
        
        // ybuffer 中默认存储 int 类型的最小值，即 -2147483648
        int ybuffer[width];
        for (int i=0; i < width; i++) {
            ybuffer[i] = std::numeric_limits<int>::min();
        }
        
        rasterize(Vec2i(20, 34),   Vec2i(744, 400), ybufferRender, red,   ybuffer);
        rasterize(Vec2i(120, 434), Vec2i(444, 400), ybufferRender, green, ybuffer);
        rasterize(Vec2i(330, 463), Vec2i(594, 200), ybufferRender, blue,  ybuffer);
        
        // 把 1px 的 ybuffer 扩展为 16px，易于展示
        for (int i=0; i<width; i++) {
            for (int j=1; j<16; j++) {
                ybufferRender.set(i, j, ybufferRender.get(i, 0));
            }
        }
        
        ybufferRender.flip_vertically();
        ybufferRender.write_tga_file("output/lesson03_y_buffer.tga");
    }

}

void drawModelTriangle() {
    TGAImage frame(width, height, TGAImage::RGB);
    
    // 遍历模型里的每个三角形，然后随机着色
//    for (int i = 0; i < model->nfaces(); i++) {
//        std::vector<int> face = model->face(i);
//        Vec2i screen_coords[3];
//        for (int j = 0; j < 3; j++) {
//            Vec3f world_coords = model->vert(face[j]);
//            screen_coords[j] = Vec2i((world_coords.x + 1.) * width / 2., (world_coords.y + 1.) * height / 2.);
//        }
//        triangle(screen_coords, frame, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
//    }
    
    // 这个是用一个模拟光照对三角形进行着色
    Vec3f light_dir(0, 0, -1); // 假设光是垂直屏幕的
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec2i screen_coords[3];
        Vec3f world_coords[3];
        
        // 计算世界坐标和屏幕坐标
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            // 投影为正交投影，而且只做了个简单的视口变换
            screen_coords[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
            world_coords[j]  = v;
        }
        
        // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize(); // 对 n 做归一化处理
        
        // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
        // 值越大，说明越多的光照射到三角形上，颜色越白
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords, frame, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }
    
    frame.flip_vertically();
    frame.write_tga_file("output/lesson02_light_model.tga");
    
    delete model;
}



int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }
//    drawSingleTriangle();
//    drawModelTriangle();
    
    drawYbuffer();

    return 0;
}
