//
//  main.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include "tgaimage.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);

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

int main(int argc, char** argv) {
        TGAImage image(100, 100, TGAImage::RGB);
        line(13, 20, 80, 40, image, white);
        line(20, 13, 40, 80, image, red);
        line(80, 40, 13, 20, image, red);
        image.flip_vertically();
        image.write_tga_file("output.tga");
        return 0;
}
