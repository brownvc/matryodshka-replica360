// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 in_uv;
layout(binding = 1) uniform sampler2D depthTex;
uniform mat4 MVP;
uniform vec4 clipPlane;

uniform int render_jump;
uniform int render_ods;

out vec2 uv_frag;
out vec2 uv_screen;

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

    // Clip distance
    gl_ClipDistance[0] = 1;

    // Clip space
    gl_Position = MVP * vec4(p, 1);
    gl_Position.z = gl_Position.z / 20;

    // Frag uv
    uv_frag = in_uv;
    uv_screen = (gl_Position.xy / gl_Position.w + 1) / 2;
}
