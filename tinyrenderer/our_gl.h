//
//  our_gl.hpp
//  tinyrenderer
//
//  Created by skychx on 2020/12/6.
//

#ifndef __OUR_GL_H__
#define __OUR_GL_H__
//
#include "tgaimage.h"
#include "geometry.h"
//
void viewport(const int x, const int y, const int w, const int h);
void projection(const double coeff=0); // coeff = -1/c
void lookat(const vec3 eye, const vec3 center, const vec3 up);

struct IShader {
    virtual ~IShader();
    virtual vec4 vertex(int iface, int nthvert) = 0;      // 顶点着色器
    virtual bool fragment(vec3 bar, TGAColor &color) = 0; // 片元着色器
};

void triangle(vec4 *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);

#endif /* our_gl_hpp */
