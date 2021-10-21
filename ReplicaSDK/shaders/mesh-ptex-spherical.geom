// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
// generates UVs for quads

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 12) out;

out vec2 uv;

void main()
{
    gl_PrimitiveID = gl_PrimitiveIDIn;
    bool zeroOut = (gl_in[0].gl_Position == vec4(0, 0, 0, 0) || gl_in[1].gl_Position == vec4(0, 0, 0, 0) || gl_in[2].gl_Position == vec4(0, 0, 0, 0) || gl_in[3].gl_Position == vec4(0, 0, 0, 0));
    float tol = 0.8;
    bool stretchOut = ((abs(gl_in[0].gl_Position.x - gl_in[1].gl_Position.x) > tol) || (abs(gl_in[0].gl_Position.x - gl_in[2].gl_Position.x) > tol) || (abs(gl_in[0].gl_Position.x - gl_in[3].gl_Position.x) > tol) || (abs(gl_in[1].gl_Position.x - gl_in[2].gl_Position.x) > tol) || (abs(gl_in[1].gl_Position.x - gl_in[3].gl_Position.x) > tol) || (abs(gl_in[2].gl_Position.x - gl_in[3].gl_Position.x) > tol));
    float t = 0.98;


    if(gl_in[0].gl_Position.y > t){
      float z = gl_in[0].gl_Position.z;
      float w = gl_in[0].gl_Position.w;

      uv = vec2(0.0, 1.0);
      gl_Position = vec4(-1.0, t, z, w);
      EmitVertex();

      uv = vec2(1.0, 0.0);
      gl_Position = vec4(-1.0, 1.0, z, w);
      EmitVertex();

      uv = vec2(1.0, 1.0);
      gl_Position = vec4(1.0, t, z, w);
      EmitVertex();

      uv = vec2(1.0, 0.0);
      gl_Position = vec4(1.0, 1.0, z, w);
      EmitVertex();

      EndPrimitive();
    }

    if(gl_in[0].gl_Position.y < -t){
      float z = gl_in[0].gl_Position.z;
      float w = gl_in[0].gl_Position.w;

      uv = vec2(0.0, 1.0);
      gl_Position = vec4(-1.0, -1.0, z, w);
      EmitVertex();

      uv = vec2(1.0, 0.0);
      gl_Position = vec4(-1.0, -t, z, w);
      EmitVertex();

      uv = vec2(1.0, 1.0);
      gl_Position = vec4(1.0, -1.0, z, w);
      EmitVertex();

      uv = vec2(1.0, 0.0);
      gl_Position = vec4(1.0, -t, z, w);
      EmitVertex();

      EndPrimitive();
    }

    // First primitive
    uv = vec2(1.0, 0.0);
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

    uv = vec2(0.0, 0.0);
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

    uv = vec2(1.0, 1.0);
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

    uv = vec2(0.0, 1.0);
    gl_ClipDistance[0] = gl_in[3].gl_ClipDistance[0];
    gl_Position = gl_in[3].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x > 0 && stretchOut) {
        gl_Position.x = -2 + gl_Position.x;
    }

    EmitVertex();
    EndPrimitive();

    if(!stretchOut)
    {
        return;
    }

    // Second primitive
    uv = vec2(1.0, 0.0);
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

    uv = vec2(0.0, 0.0);
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

    uv = vec2(1.0, 1.0);
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

    uv = vec2(0.0, 1.0);
    gl_ClipDistance[0] = gl_in[3].gl_ClipDistance[0];
    gl_Position = gl_in[3].gl_Position;
    if(zeroOut)
    {
        gl_Position = vec4(0, 0, 0, 0);
    }
    else if(gl_Position.x <= 0) {
        gl_Position.x = 2 + gl_Position.x;
    }
    EmitVertex();

    EndPrimitive();

}
