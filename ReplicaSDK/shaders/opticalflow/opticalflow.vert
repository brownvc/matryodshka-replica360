// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec4 position;

uniform mat4 MV_current;
uniform mat4 MV_next;

out vec4 pos_next;

out gl_PerVertex {
    smooth vec4 gl_Position;
};

void main()
{
    gl_Position = MV_current * position;
    pos_next = MV_next * position;
}
