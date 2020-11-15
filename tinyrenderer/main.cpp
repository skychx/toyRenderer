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
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 200;
const int height = 200;

// 思路很简单，点连成线
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    // 处理比较陡的线：交换 x y 位置
    bool steep = false;
    if (std::abs(x0 - x1)<std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    // 处理 x0 > x1 的情况
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    // 缓存偏移量
    int dx = x1-x0;
    int dy = y1-y0;
    // 存储误差
    // float derror = std::abs(dy / float(dx)); // 每次循环增加的小误差
    // float error = 0; // 累积误差
    // 浮点运算肯定没有 int 快，我们想办法去掉浮点运算
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x=x0; x<=x1; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        // 误差的处理：每次误差大于一个像素时，y 就要进位 1，相应的累积误差也要减小 1
        // error += derror;
        // if (error > .5) {
            // y += (y1 > y0 ? 1 : -1);
            // error -= 1.;
        // }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
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
    
    if((AB^AP) > 0 && (BC^BP) > 0 && (CA^CP) > 0) {
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


int main(int argc, char** argv) {
    TGAImage frame(width, height, TGAImage::RGB);

    Vec2i pts[3] = {Vec2i(10, 10), Vec2i(150, 30), Vec2i(70, 160)};

    triangle(pts, frame, red);

    frame.flip_vertically();
    frame.write_tga_file("output/lesson02_self.tga");
    delete model;
    return 0;
}
