// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
// generates UVs for quads

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 uv;
} gs_in[];  

out vec2 uv_frag;

void main()
{
    gl_PrimitiveID = gl_PrimitiveIDIn;

    uv_frag = gs_in[1].uv;
    //uv_frag = vec2(1.0, 0.0);
    gl_ClipDistance[0] = gl_in[1].gl_ClipDistance[0];
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    uv_frag = gs_in[0].uv;
    //uv_frag = vec2(0.0, 0.0);
    gl_ClipDistance[0] = gl_in[0].gl_ClipDistance[0];
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    uv_frag = gs_in[2].uv;
    //uv_frag = vec2(1.0, 1.0);
    gl_ClipDistance[0] = gl_in[2].gl_ClipDistance[0];
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
