//
//  geometry.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/29.
//
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include "geometry.h"

vec3 cross(const vec3 &v1, const vec3 &v2) {
    return vec<3>{v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x};
}
