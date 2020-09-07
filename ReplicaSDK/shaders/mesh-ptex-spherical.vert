// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec4 position;

uniform mat4 MVP;
uniform mat4 MV;
uniform float baseline;
uniform vec4 clipPlane;
uniform int leftRight;


void main()
{
    gl_ClipDistance[0] = 1;
    vec4 perspPos = MVP * position;

    // Map point to clip space based on equirectangular projection
    vec3 p = (MV * position).xyz;

    float r = min(baseline, max(0,sqrt(p.x*p.x + p.z*p.z)-0.002));
    float f = r * r - p.x * p.x - p.z * p.z;

    vec3 d;
    float px, pz;

    if(abs(p.z) > abs(p.x)) {
        px = p.x;
        pz = p.z;
    }
    else {
        px = p.z;
        pz = p.x;
    }

    float a = 1 + px * px / (pz * pz);
    float b = -2 * f * px / (pz * pz);
    float c = f + f * f / (pz * pz);

    float disc = b * b - 4 * a * c;

    if(leftRight!=2){
      if(disc < 0){
          d=p;
          float theta = - atan(d.x, d.z);
          float phi = - atan(d.y, sqrt(d.x * d.x + d.z * d.z));
          vec4 pos;
          pos.x = (-theta / (M_PI) + 0.0) * 1;
          pos.y = (-phi / (M_PI / 2) + 0.0) * 1;
          pos.z = abs(length(p.xyz)) / 20;
          pos.w = 1;

          gl_Position = pos;

          //gl_Position = vec4(0, 0, 0, 0);
          return;
      }
    }

    // Direction vector from point
    float s = sign(pz) * sqrt(disc);
    if(abs(p.z) <= abs(p.x)) {
        s = -s;
    }
    // Do 'left-eye' rendering
    if(leftRight == 1){
        s = -s;
    }

    float dx = (-b + s) / (2 * a);
    float dz = (f - px * dx) / pz;

    if(abs(p.z) > abs(p.x)) {
        d = vec3(-dx, p.y, -dz);
    }
    else {
        d = vec3(-dz, p.y, -dx);
    }

    //Do monoscopic rendering with baseline 0
    if(leftRight == 2){
        d=p;
    }

    // Angles from direction vector
    float theta = - atan(d.x, d.z);
    float phi = - atan(d.y, sqrt(d.x * d.x + d.z * d.z));

    // Normalize to clip space
    vec4 pos;

    pos.x = (-theta / (M_PI) + 0.0) * 1;
    pos.y = (-phi / (M_PI / 2) - 0.0002) * 1;

    pos.z = abs(length(p.xyz)) / 20;
    pos.w = 1;

    gl_Position = pos;
    //gl_Position = perspPos;
}
