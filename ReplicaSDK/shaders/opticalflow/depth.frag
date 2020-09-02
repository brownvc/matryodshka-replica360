// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core

layout(location = 0) out vec4 FragColor;

in float vdepth;

void main()
{
    FragColor = vec4(abs(vdepth), 0.0, 0.0, 1.0f);
}
