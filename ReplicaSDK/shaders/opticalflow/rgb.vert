// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core

layout(location = 0) in vec4 position;

uniform mat4 MV;

out gl_PerVertex {
    vec4 gl_Position;
};

// not use
void main()
{
    gl_Position = MV * position;
}
