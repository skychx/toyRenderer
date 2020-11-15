//
//  main.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//

#include "tgaimage.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);

// 这个程序很简单，就是以左下角为原点，创建一个 100x100 的正方形， (52, 41) 位置点一个红点
// output.tga 输出路径在 Products 目录下，和 tinyrenderer 可执行文件在同一个目录下
int main(int argc, char** argv) {
        TGAImage image(100, 100, TGAImage::RGB);
        image.set(52, 41, red);
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("output.tga");
        return 0;
}
