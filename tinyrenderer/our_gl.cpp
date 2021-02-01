//
//  our_gl.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/12/6.
//

#include "our_gl.h"

mat<4,4> ModelView;
mat<4,4> Projection;
mat<4,4> Viewport;


// 计算 ModelView 矩阵，实现坐标系的转换
void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    // 新的 x'y'z' 坐标系
    vec3 z = (eye - center).normalize();
    vec3 x = cross(up, z).normalize();
    vec3 y = cross(z, x).normalize();
    
    // 计算 xyz -> x'y'z' 的转换矩阵
    mat<4,4> Minv = {{
        {x.x, x.y, x.z, 0},
        {y.x, y.y, y.z, 0},
        {z.x, z.y, z.z, 0},
        {  0,   0,   0, 1}
    }};
    mat<4,4> Tr = {{
        {1, 0, 0, -center.x},
        {0, 1, 0, -center.y},
        {0, 0, 1, -center.z},
        {0, 0, 0,         1}
    }};

    ModelView = Minv*Tr;
}


// 投影矩阵
// 注意：乘以投影矩阵并没有进行实际的透视投影变换，它只是计算出合适的分母，投影实际发生在从 4D 到 3D 变换时
// 这个投影矩阵，认为 z 轴垂直于屏幕切方向向外； z=0 处为投影平面，z=c 处为摄像机，[0, c] 间为模型
// 具体结构可见课程图片：https://raw.githubusercontent.com/ssloy/tinyrenderer/gh-pages/img/04-perspective-projection/525d3930435c3be900e4c7956edb5a1c.png
void projection(const double coeff) {
    Projection = {{
        {1, 0,     0, 0},
        {0, 1,     0, 0},
        {0, 0,     1, 0},
        {0, 0, coeff, 1}
    }};
}

// 视口变换
// [-1, 1]*[-1, 1]*[-1, 1] 正方体转换为长方体 [x, x+w]*[y, y+h]*[-1, 1]
void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{
        {w/2.,    0,         0,  x + w/2.},
        {   0, h/2.,         0,  y + h/2.},
        {   0,    0, 255.f/2.f, 255.f/2.f},
        {   0,    0,         0,         1}
    }};
}


// 利用重心坐标求解，返回三角形的重心坐标
vec3 barycentric(vec2 A, vec2 B, vec2 C, vec2 P) {

    vec3 x(C[0] - A[0], B[0] - A[0], A[0] - P[0]);
    vec3 y(C[1] - A[1], B[1] - A[1], A[1] - P[1]);

    vec3 u = cross(x, y); // u 向量和 x y 向量的点积为 0，所以 x y 向量叉乘可以得到 u 向量

    // 由于 A, B, C, P 的坐标都是 int 类型，所以 u[2] 必定是 int 类型
    // 如果 u[2] 为 0，则表示三角形 ABC 退化了（退还为直线 or 一个点），需要对其舍弃
    if (std::abs(u[2]) > 1e-2) {
        return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }

    // in this case generate negative coordinates, it will be thrown away by the rasterizator
    return vec3(-1, 1, 1);
}

// 自己实现的三角形光栅化函数
// 主要思路是利用重心坐标判断点是否在三角形内
void triangle(vec4 *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
    // 步骤 1: 找出包围盒
    vec2 boxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    vec2 boxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    // 查找包围盒边界
    for (int i = 0; i < 3; i++) {
        // 第一层循环，遍历 pts
        for (int j = 0; j < 2; j++) {
            // 第二层循环，遍历 Vec2i
            boxmin[j] = std::min(boxmin[j], pts[i][j] / pts[i][3]);
            boxmax[j] = std::max(boxmax[j], pts[i][j] / pts[i][3]);
        }
    }

    // 步骤二：对包围盒里的每一个像素进行遍历
    vec2 P;
    TGAColor color;
    // 这里注意要强制指定 int 类型，不然 P 坐标转为浮点数时绘制会出现边界着色失败的现象
    for (P.x = (int)boxmin.x; P.x <= (int)boxmax.x; P.x++) {
        for (P.y = (int)boxmin.y; P.y <= (int)boxmax.y; P.y++) {
            // c 是根据三个顶点坐标计算出的重心坐标
            vec3 c = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            
            // TODO: 这里感觉是在做 zbuffer 记录，没太看懂明天看吧
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            int frag_depth = std::max(0, std::min(255, int(z/w + .5)));
            
            // 重心坐标某一项小于 0，说明在三角形外，跳过不绘制
            if (c.x < 0 || c.y < 0 || c.z < 0 ||
                zbuffer.get(P.x, P.y)[0] > frag_depth) {
                continue;
            }
            
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                image.set(P.x, P.y, color);
            }
        }
    }
}
