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
        {w/2.,    0,  0, x + w/2.},
        {   0, h/2.,  0, y + h/2.},
        {   0,    0,  1,        0},
        {   0,    0,  0,        1}
    }};
}

//// 利用叉乘判断是否在三角形内部
//bool isInside(vec3 *pts, vec2 P) {
//    vec2 AB(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
//    vec2 BC(pts[2].x - pts[1].x, pts[2].y - pts[1].y);
//    vec2 CA(pts[0].x - pts[2].x, pts[0].y - pts[2].y);
//
//    vec2 AP(P.x - pts[0].x, P.y - pts[0].y);
//    vec2 BP(P.x - pts[1].x, P.y - pts[1].y);
//    vec2 CP(P.x - pts[2].x, P.y - pts[2].y);
//
//    // 叉乘计算
//    if(
//       (AB.x * AP.y - AP.x * AB.y) >= 0 &&
//       (BC.x * BP.y - BP.x * BC.y) >= 0 &&
//       (CA.x * CP.y - CP.x * CA.y) >= 0
//    ) {
//        return true;
//    }
//    return false;
//}
//
//// 利用重心坐标求解，返回三角形的重心坐标
//vec3 barycentric(vec3 A, vec3 B, vec3 C, vec3 P) {
//    
//    vec3 x(C[0] - A[0], B[0] - A[0], A[0] - P[0]); // AB AC PA 在 x 上的分量
//    vec3 y(C[1] - A[1], B[1] - A[1], A[1] - P[1]); // AB AC PA 在 y 上的分量
//
//    vec3 u = cross(x, y); // u 向量和 x y 点乘都为 0，所以 u 垂直于 xy 平面
//    
//    // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
//    // 根据 u[2] 判断法线是向内的还是向外的，向内的抛弃，向外的保留并归一化
//    if (std::abs(u[2]) > 1e-2) {
//        return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
//    }
//    
//    // in this case generate negative coordinates, it will be thrown away by the rasterizator
//    return vec3(-1, 1, 1);
//}
