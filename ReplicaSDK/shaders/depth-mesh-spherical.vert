// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 in_uv;
layout(binding = 1) uniform sampler2D depthTex;

uniform mat4 MVP;
uniform mat4 MV;
uniform float baseline;
uniform vec4 clipPlane;

uniform int render_jump;
uniform int render_ods;

out VS_OUT {
    vec2 uv;
    vec2 uv_screen;
} vs_out;

void main()
{
    // Get position on the sphere from uv coordinates
    vec2 uv = vec2(1 - in_uv.x, in_uv.y);
    vec3 p = vec3(sin(M_PI * uv.y) * cos(2 * M_PI * uv.x),
        cos(M_PI * uv.y),
        sin(M_PI * uv.y) * sin(2 * M_PI * uv.x));

    // Offset by depth
    vec4 texel;
    float depth;
    if(render_jump == 1) {
        depth = textureLod(depthTex, uv, 0.0).x;
        depth = 0.299999999999999999f / (depth + 0.001f);
    }
    else if(render_ods == 1) {
        depth = textureLod(depthTex, uv, 0.0).x * 32;
    }
    else {
        depth = textureLod(depthTex, uv, 0.0).x * 16;
    }

    if(render_ods == 1) {
        vec3 start_p = vec3(cos(2 * M_PI * uv.x), 0, sin(2 * M_PI * uv.x)) * baseline / 2;
        p = start_p + p * depth;
    }
    else {
        p = p * depth;
    }

    // Map point to clip space based on equirectangular projection
    p = (MV * vec4(p, 1)).xyz;

    // Angles from direction vector
    float theta = - atan(p.z, p.x);
    float phi = - atan(p.y, sqrt(p.x * p.x + p.z * p.z));

    // Normalize to clip space
    vec4 pos;
    pos.x = (-theta / (M_PI) + 0.0) * 1;
    pos.y = (-phi / (M_PI / 2) + 0.0) * 1;
    pos.z = abs(length(p.xyz)) / 20;
    pos.w = 1;

    gl_Position = pos;
    gl_Position.z = gl_Position.z / 20;

    // Clip distance
    gl_ClipDistance[0] = 1;

    // Frag uv
    vs_out.uv = in_uv;
    vs_out.uv_screen = (gl_Position.xy / gl_Position.w + 1) / 2;
}
