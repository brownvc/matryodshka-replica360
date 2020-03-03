// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
// generates UVs for quads

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

in VS_OUT {
    vec2 uv;
    vec2 uv_screen;
} gs_in[];  

out vec2 uv_frag;
out vec2 uv_screen;

void main()
{
    gl_PrimitiveID = gl_PrimitiveIDIn;
    bool zeroOut = (gl_in[0].gl_Position == vec4(0, 0, 0, 0) || gl_in[1].gl_Position == vec4(0, 0, 0, 0) || gl_in[2].gl_Position == vec4(0, 0, 0, 0));
    float tol = 0.8;
    bool stretchOut = ((abs(gl_in[0].gl_Position.x - gl_in[1].gl_Position.x) > tol) || (abs(gl_in[0].gl_Position.x - gl_in[2].gl_Position.x) > tol) || (abs(gl_in[1].gl_Position.x - gl_in[2].gl_Position.x) > tol));

    // First primitive
    uv_frag = gs_in[1].uv;
    uv_screen = gs_in[1].uv_screen;
    gl_ClipDistance[0] = gl_in[1].gl_ClipDistance[0];
    gl_Position = gl_in[1].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x > 0 && stretchOut) {
        gl_Position.x = -2 + gl_Position.x;
    }
    EmitVertex();

    uv_frag = gs_in[0].uv;
    uv_screen = gs_in[0].uv_screen;
    gl_ClipDistance[0] = gl_in[0].gl_ClipDistance[0];
    gl_Position = gl_in[0].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x > 0 && stretchOut) {
        gl_Position.x = -2 + gl_Position.x;
    }

    EmitVertex();

    uv_frag = gs_in[2].uv;
    uv_screen = gs_in[2].uv_screen;
    gl_ClipDistance[0] = gl_in[2].gl_ClipDistance[0];
    gl_Position = gl_in[2].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(1, 0, 0, 0);
    }
    else if(gl_Position.x > 0 && stretchOut) {
        gl_Position.x = -2 + gl_Position.x;
    }

    EmitVertex();

    //uv_frag = vec2(0.0, 1.0);
    //gl_ClipDistance[0] = gl_in[3].gl_ClipDistance[0];
    //gl_Position = gl_in[3].gl_Position;
    //if(zeroOut)
    //{
    //    gl_Position = vec4(0, 0, 0, 0);
    //}
    //else if(gl_Position.x > 0 && stretchOut) {
    //    gl_Position.x = -2 + gl_Position.x;
    //}

    //EmitVertex();

    EndPrimitive();

    if(!stretchOut)
    {
        return;
    }

    // Second primitive
    uv_frag = gs_in[1].uv;
    uv_screen = gs_in[1].uv_screen;
    gl_ClipDistance[0] = gl_in[1].gl_ClipDistance[0];
    gl_Position = gl_in[1].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x <= 0) {
        gl_Position.x = 2 + gl_Position.x;
    }
    EmitVertex();

    uv_frag = gs_in[0].uv;
    uv_screen = gs_in[0].uv_screen;
    gl_ClipDistance[0] = gl_in[0].gl_ClipDistance[0];
    gl_Position = gl_in[0].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x <= 0) {
        gl_Position.x = 2 + gl_Position.x;
    }
    EmitVertex();

    uv_frag = gs_in[2].uv;
    uv_screen = gs_in[2].uv_screen;
    gl_ClipDistance[0] = gl_in[2].gl_ClipDistance[0];
    gl_Position = gl_in[2].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x <= 0) {
        gl_Position.x = 2 + gl_Position.x;
    }
    EmitVertex();

    //uv_frag = vec2(0.0, 1.0);
    //gl_ClipDistance[0] = gl_in[3].gl_ClipDistance[0];
    //gl_Position = gl_in[3].gl_Position;
    //if(zeroOut)
    //{
    //    gl_Position = vec4(0, 0, 0, 0);
    //}
    //else if(gl_Position.x <= 0) {
    //    gl_Position.x = 2 + gl_Position.x;
    //}
    //EmitVertex();

    EndPrimitive();

}
