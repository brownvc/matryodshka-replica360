#version 430 core

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 32) out;

out vec2 uv;

struct vertex_struct
{
  vec4 position;
  vec2 uv;
};

void commit_triangle(inout vertex_struct vertex_list_curt[3]);
void commit_quadrilateral(inout vertex_struct vertex_list_curt[4]);
void commit_vertex_list(inout vertex_struct vertex_list_curt[8], in int vertex_numb);

#include "opticalflow/common.glsl"
#include "opticalflow/mesh_split.glsl"

void commit_triangle(inout vertex_struct vertex_list_curt[3])
{
    for(int idx = 0 ; idx < 3; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].position);
        uv = vertex_list_curt[idx].uv;
        EmitVertex();
    }
    EndPrimitive();
}

void commit_quadrilateral(inout vertex_struct vertex_list_curt[4])
{
    for(int idx = 0 ; idx < 4; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].position);
        uv = vertex_list_curt[idx].uv;
        EmitVertex();
    }
    EndPrimitive();
}

void commit_vertex_list(inout vertex_struct vertex_list_curt[8], in int vertex_numb)
{
    for(int idx = 0; idx < vertex_numb; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].position);
        uv = vertex_list_curt[idx].uv;
        EmitVertex();
    }
    EndPrimitive();
}

void main()
{
    gl_PrimitiveID = gl_PrimitiveIDIn;
    process_quadrilateral();
}
