// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#include "atlas.glsl"

layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D colorTex;

in vec2 uv_frag;

void main()
{
    vec4 c = texture2D(colorTex, uv_frag);

    FragColor = vec4(c.rgb, 1);
}
