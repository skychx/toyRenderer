//
//  main.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;

// Bresenham’s 直线算法
// 具体实现参考 https://www.wikiwand.com/en/Bresenham%27s_line_algorithm#/All_cases
void line(int x1, int y1, int x2, int y2, TGAImage &image, TGAColor color) {
    // 处理斜率的绝对值大于 1 的直线
    bool steep = false;
    if (std::abs(x1-x2) < std::abs(y1-y2)) {
        std::swap(x1, y1);
        std::swap(x2, y2);
        steep = true;
    }
    // 交换起点终点的坐标
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    int y = y1;
    int eps = 0;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;
    
    // 处理 [-1, 0] 范围内的斜率
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    
    for (int x = x1; x <= x2; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }

        eps += dy;
        // 这里用位运算 <<1 代替 *2
        if((eps << 1) >= dx)  {
            y = y + yi;
            eps -= dx;
        }
    }
}

// DDA 算法
// 缺点：涉及大量的浮点运算
void lineDDA(int x1, int y1, int x2, int y2, TGAImage &image, TGAColor color) {
    float x = x1;
    float y = y1;
    int dx = x2 - x1;
    int dy = y2 - y1;
    float step;
    float dlx, dly;

    // 根据 dx 和 dy 的长度决定基准
    if (std::abs(dx) >= std::abs(dy)) {
      step = std::abs(dx);
    } else {
      step = std::abs(dy);
    }
    
    dlx = dx / step;
    dly = dy / step;
    
    for (int i=1; i<step; i++) {
      image.set(x, y, color);
      x = x + dlx;
      y = y + dly;
    }
}

void drawDDALine() {
    TGAImage image(width, height, TGAImage::RGB);
    
    lineDDA(400, 400, 800, 600, image, white);
    lineDDA(400, 400, 600, 800, image, white);
    lineDDA(400, 400, 200, 800, image, white);
    lineDDA(400, 400,   0, 600, image, white);
    lineDDA(400, 400,   0, 200, image, white);
    lineDDA(400, 400, 200,   0, image, white);
    lineDDA(400, 400, 600,   0, image, white);
    lineDDA(400, 400, 800, 200, image, white);
    lineDDA(  0, 400, 800, 400, image, white);
    lineDDA(400,   0, 400, 800, image, white);
    
    image.flip_vertically();
    image.write_tga_file("output/day_02_line_dda.tga");
}

void drawBresenhamLine() {
    TGAImage image(width, height, TGAImage::RGB);
    
    line(400, 400, 800, 600, image, white);
    line(400, 400, 600, 800, image, white);
    line(400, 400, 200, 800, image, white);
    line(400, 400,   0, 600, image, white);
    line(400, 400,   0, 200, image, white);
    line(400, 400, 200,   0, image, white);
    line(400, 400, 600,   0, image, white);
    line(400, 400, 800, 200, image, white);
    line(  0, 400, 800, 400, image, white);
    line(400,   0, 400, 800, image, white);
    
    image.flip_vertically();
    image.write_tga_file("output/day_02_line_bresenham.tga");
}

void drawObj() {
    model = new Model("obj/african_head.obj");
    
    TGAImage image(width, height, TGAImage::RGB);
    
    // 循环模型里的所有三角形
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        
        // 循环三角形三个顶点，每两个顶点连一条线
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;
            line(x0, y0, x1, y1, image, white);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output/day_02_line_obj.tga");
    
    delete model;
}

int main(int argc, char** argv) {
    drawDDALine();
    drawBresenhamLine();
    drawObj();

    return 0;
}
