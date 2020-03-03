// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 in_uv;

out vec2 uv_frag;

void main()
{
    // Clip space
    gl_Position = vec4(position.xyz, 1);

    // Frag uv
    uv_frag = in_uv;
}
