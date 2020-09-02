#version 430 core

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 64) out;

smooth in vec4 pos_next[];

smooth out vec4 vpos;
smooth out vec4 vposNext;

struct vertex_struct
{
  vec4 current;
  vec4 next;
};

#include "opticalflow/common.glsl"

// the method to deal with warp around
// 0. w/o warp around: ignore the ERP camera model, output pin-hole image optical flow;
// 1. warp around: make the pixel warp around, output panoramic image optical flow.
const int wrap_around_method = 0;

// if it's true, it will generate optical flow.
// if it's false, it just process the points' current location
const bool compute_optical_flow = true;

// the point warping around need clip, and interpolate to generate the new point
// interpolate current & next points when split line, and process the warping round of next vertex.
void split_line(in vertex_struct point_1st, in vertex_struct point_2nd,
                  out vertex_struct point_1st_inserted, out vertex_struct point_2nd_inserted, 
                  in bool vertex_current)
{
    if(!vertex_current)
    {
        // split the line base on the next point position
        vec4 temp_0 = point_1st.next;
        point_1st.next = point_1st.current;
        point_1st.current = temp_0;

        vec4 temp_1 = point_2nd.next;
        point_2nd.next = point_2nd.current;
        point_2nd.current = temp_1; 
    }

    // 1) split the line of current points to generate the interpolated point for first & second 
    float ratio = abs(point_1st.current.x / (point_2nd.current.x - point_1st.current.x));  
    vec2 interpolated = ratio * (point_2nd.current.yz - point_1st.current.yz) + point_1st.current.yz;
    if (point_1st.current.x <= 0.0)
    {
        point_1st_inserted.current = vec4(0.0, interpolated.xy, 0.5); // +PI
        point_2nd_inserted.current = vec4(0.0, interpolated.xy, -0.5); // -PI
    }else{
        point_1st_inserted.current = vec4(0.0, interpolated.xy, -0.5); // -PI
        point_2nd_inserted.current = vec4(0.0, interpolated.xy, 0.5); // +PI
    }

    // 2) split corresponding line of next points to generate the interpolated point for first & second 
    vec3 interpolated_next = ratio * (point_2nd.next.xyz - point_1st.next.xyz) + point_1st.next.xyz;
    if (interpolated_next.x != 0){
        point_1st_inserted.next = vec4(interpolated_next.xyz, 1.0);
        point_2nd_inserted.next = vec4(interpolated_next.xyz, 1.0);
    }else{
        // process the case interpolated next.x is 0
        if (point_1st.next.x < 0.0){
            point_1st_inserted.next = vec4(interpolated_next.xyz, 0.5); // +PI
            point_2nd_inserted.next = vec4(interpolated_next.xyz, -0.5); // -PI
        }else if(point_1st.next.x > 0.0){
            point_1st_inserted.next = vec4(interpolated_next.xyz, -0.5); // -PI
            point_2nd_inserted.next = vec4(interpolated_next.xyz, 0.5); // +PI
        }else if(point_1st.next.x == 0.0){
            // the point1_st.next.x & point_2nd.next.x are both 0
            point_1st_inserted.next = vec4(interpolated_next.xyz, point_1st.next.w);
            point_2nd_inserted.next =  vec4(interpolated_next.xyz, point_1st.next.w);
        }
    }
    
    if(!vertex_current)
    {// restore the next and current point site
        vec4 temp_0 = point_1st_inserted.next;
        point_1st_inserted.next = point_1st_inserted.current;
        point_1st_inserted.current = temp_0;

        vec4 temp_1 = point_2nd_inserted.next;
        point_2nd_inserted.next = point_2nd_inserted.current;
        point_2nd_inserted.current = temp_1; 
    }
}

// generate the center's next with current point position
void triangle_center(in vertex_struct[3] center_triangle, out vertex_struct center, in bool vertex_current)
{
    if(!vertex_current)
    {
        for(int idx = 0; idx < 3; idx ++){
            vec4 temp = center_triangle[idx].next;
            center_triangle[idx].next = center_triangle[idx].current;
            center_triangle[idx].current = temp;
        }
    }

    center.current = vec4(0.0, 0.0, 0.0, 1.0);
    center.next = vec4(0.0, 0.0, 0.0, 1.0);

    // 0) compute Y
    vec3 normal = cross(center_triangle[1].current.xyz - center_triangle[0].current.xyz, 
                        center_triangle[2].current.xyz - center_triangle[0].current.xyz);
    center.current.y = dot(normal.xyz, center_triangle[0].current.xyz) / normal.y;

    // 1) interpolate 
    float[3] weight;
    float weight_sum = 0;
    for(int idx = 0; idx < 3; idx++){
        weight[idx] = length(center_triangle[idx].current.xyz - center.current.xyz);
        weight_sum += weight[idx];
    }

    center.next.xyz = center_triangle[0].next.xyz * (weight[0] / weight_sum)
                  + center_triangle[1].next.xyz * (weight[1] / weight_sum)
                  + center_triangle[2].next.xyz * (weight[2] / weight_sum);

    // restore the next and current point site
    if(!vertex_current)
    {
        vec4 temp = center.next;
        center.next = center.current;
        center.current = temp;
    }
}


// get the next point position in spherical coordinates, with regard to warp around
vec4 get_next_position(inout vec4 current, inout vec4 next)
{
    // to spherical coordinate
    vec4 next_sph = cartesian_2_sphere(next);
    
    if(wrap_around_method == 0){
        // do not process warp around, as pine-hole optical flow
        return next_sph;       
    }else if(wrap_around_method == 1){
        // test whether the optical flow wraps around
        bool warp_around = test_split_line(current, next);
        warp_around = warp_around || current.x == 0 || next.x == 0;
        if(warp_around){
            if((current.x < 0 && next.x > 0) ||
                (current.x < 0 && next.w == -0.5) ||
                (current.w == 0.5 && next.x > 0))
            {
                next_sph.x = +1 + (1 + next_sph.x);
            }
            else if((current.x > 0 && next.x < 0) || 
                (current.x > 0 && next.w == 0.5) ||
                (current.w == -0.5 && next.x < 0))
            {
                next_sph.x = -1 - (1 - next_sph.x);
            }
        }
        return next_sph; 
    }
}

void commit_triangle(inout vertex_struct vertex_list_curt[3])
{
    for(int idx = 0 ; idx < 3; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].current);
        vpos = cartesian_2_sphere(vertex_list_curt[idx].current);
        vec4 vposNext_temp = get_next_position(vertex_list_curt[idx].current, vertex_list_curt[idx].next);
        vposNext = vec4(vposNext_temp.xyz, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

void commit_quadrilateral(inout vertex_struct vertex_list_curt[4])
{
    for(int idx = 0 ; idx < 4; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].current);
        vpos = cartesian_2_sphere(vertex_list_curt[idx].current);
        vec4 vposNext_temp = get_next_position(vertex_list_curt[idx].current, vertex_list_curt[idx].next);
        vposNext = vec4(vposNext_temp.xyz, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

void commit_vertex_list(inout vertex_struct vertex_list_curt[8], in int vertex_numb)
{
    for(int idx = 0; idx < vertex_numb; idx++ ){
        gl_Position = cartesian_2_sphere(vertex_list_curt[idx].current);
        vpos = cartesian_2_sphere(vertex_list_curt[idx].current);
        vec4 vposNext_temp = get_next_position(vertex_list_curt[idx].current, vertex_list_curt[idx].next);
        vposNext = vec4(vposNext_temp.xyz, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

// counting the split edges number of current points
void test_split_triangle(inout vertex_struct vertex_list[tile_point_number], 
                    out int split_counter, out int split_line_idx , in bool vertex_current)
{
    split_counter = 0;
    split_line_idx = 0;
    for(int idx = 0; idx < tile_point_number; idx++)
    {
        int idx_next = int(mod(idx + 1, tile_point_number));
        if(vertex_current){
            if(test_split_line(vertex_list[idx].current, vertex_list[idx_next].current))
            {
                split_counter++;
                split_line_idx += int(pow(2, idx));
            }
        }else{
            if(test_split_line(vertex_list[idx].next, vertex_list[idx_next].next))
            {
                split_counter++;
                split_line_idx += int(pow(2, idx));
            }
        }
    }

    // double-check whether poles (x=0,z=0) in the triangle
    if(split_counter == 1)
    {
        vec2 a,ab,ac;
        if(vertex_current){
            a = vec2(vertex_list[0].current.xz);
            ab = vec2(vertex_list[1].current.xz) - a;
            ac = vec2(vertex_list[2].current.xz) - a;
        }else{
            a = vec2(vertex_list[0].next.xz);
            ab = vec2(vertex_list[1].next.xz) - a;
            ac = vec2(vertex_list[2].next.xz) - a;  
        }
        vec2 ap = vec2(0.0, 0.0) - a;

        float det = ab.x * ac.y - ab.y * ac.x;
        float w_b = (ac.y * ap.x - ac.x * ap.y) / det;
        float w_c =  (ab.x * ap.y - ab.y * ap.x) / det;
        float w_a = 1 - w_b - w_c;

        bool w_a_range = w_a >= 0 && w_a <= 1;
        bool w_b_range = w_b >= 0 && w_b <= 1;
        bool w_c_range = w_c >= 0 && w_c <= 1;

        if (!(w_a_range && w_b_range && w_c_range))
            split_counter++; 
    }
}

void split_case_0_correct(inout vertex_struct triangles[3], in bool vertex_current)
{
    if(vertex_current){
        // correct the case when 1 in the points
        bool contain_zero = (triangles[0].current.x == 0.0 && triangles[0].current.z < 0)
                        || (triangles[1].current.x == 0.0 && triangles[1].current.z < 0)
                        || (triangles[2].current.x == 0.0 && triangles[2].current.z < 0);

        if(contain_zero)
        {
            bool larger_than_zero = (triangles[0].current.x > 0.0)
                            || (triangles[1].current.x > 0.0)
                            || (triangles[2].current.x > 0.0);

            for(int i = 0 ; i< triangles.length(); i++){
                if(triangles[i].current.x == 0.0)
                {
                    if(larger_than_zero){
                        triangles[i].current.w = -0.5;
                    }else if(!larger_than_zero){
                        triangles[i].current.w = +0.5;
                    }
                }
            }
        }
    }else{
        bool contain_zero = (triangles[0].next.x == 0.0 && triangles[0].next.z < 0)
                || (triangles[1].next.x == 0.0 && triangles[1].next.z < 0)
                || (triangles[2].next.x == 0.0 && triangles[2].next.z < 0);

        if(contain_zero)
        {
            bool larger_than_zero = (triangles[0].next.x > 0.0)
                            || (triangles[1].next.x > 0.0)
                            || (triangles[2].next.x > 0.0);

            for(int i = 0 ; i< triangles.length(); i++){
                if(triangles[i].next.x == 0.0)
                {
                    if(larger_than_zero){
                        triangles[i].next.w = -0.5;
                    }else if(!larger_than_zero){
                        triangles[i].next.w = +0.5;
                    }
                }
            }
        }
    }
}

void split_case_0(inout vertex_struct triangles[3], in bool vertex_current, inout vertex_struct triangle_list[6][3])
{
    if(compute_optical_flow && vertex_current){
        // split_case_0_correct(triangles, true);
        // split_case_0_correct(triangles, false);
        triangle_list[0][0] = triangles[0];
        triangle_list[0][1] = triangles[1];
        triangle_list[0][2] = triangles[2];
    }else{
        split_case_0_correct(triangles, true);
        split_case_0_correct(triangles, false);
        commit_triangle(triangles); 
    }
}

// process the pole points
void split_case_1(in vertex_struct triangle[3], in int split_line_idx ,
                in bool vertex_current,
                inout vertex_struct triangle_list[6][3])
{
    // 0) insert new points to split the tile
    int split_start_point_idx; 
    if(split_line_idx == 1){
        split_start_point_idx = 0;
    }else if(split_line_idx == 2){
        split_start_point_idx = 1;
    }else if(split_line_idx == 4){
        split_start_point_idx = 2;
    }

    int point_0_idx = int(mod(split_start_point_idx + 0, tile_point_number));
    int point_1_idx = int(mod(split_start_point_idx + 1, tile_point_number));
    int point_2_idx = int(mod(split_start_point_idx + 2, tile_point_number));

    vertex_struct point_insert_010;
    vertex_struct point_insert_011;
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011, vertex_current);

    // 1) generate the triangles in pole point
    // generate the current center point for split the triangle with next points
    // Pole_L(Left) ---> pole ---> Pole_R(Right)
    vertex_struct center;
    triangle_center(triangle, center, vertex_current); // interpolate the center's next value
    vertex_struct point_pole_L = vertex_struct(center.current, center.next);
    vertex_struct point_pole_center = vertex_struct(center.current, center.next);
    vertex_struct point_pole_R = vertex_struct(center.current, center.next);
     
    if(vertex_current){
        point_pole_L.current = vec4(0.0, center.current.y, 0.0, -0.5);// L corner
        point_pole_center.current = vec4(0.0, center.current.y, 0.0, 0.0);//  pole
        point_pole_R.current = vec4(0.0, center.current.y, 0.0, 0.5);// R corner
    }else{
        point_pole_L.next = vec4(0.0, center.next.y, 0.0, -0.5);// L corner
        point_pole_center.next = vec4(0.0, center.next.y, 0.0, 0.0);// pole
        point_pole_R.next = vec4(0.0, center.next.y, 0.0, 0.5);// R corner
    }

    // 2) commit or return tiles
    if(vertex_current && compute_optical_flow)
    {
        //return the vertex list for process the next 
        if(triangle[point_0_idx].current.y >= 0)
        {
            // North pole
            triangle_list[0][0] = point_insert_011;
            triangle_list[0][1] = point_pole_L;
            triangle_list[0][2] = triangle[point_1_idx];

            triangle_list[1][0] = point_pole_L;
            triangle_list[1][1] = point_pole_center;
            triangle_list[1][2] = triangle[point_1_idx];

            triangle_list[2][0] = triangle[point_1_idx];
            triangle_list[2][1] = point_pole_center;
            triangle_list[2][2] = triangle[point_2_idx];

            triangle_list[3][0] = point_pole_center;
            triangle_list[3][1] = triangle[point_0_idx];
            triangle_list[3][2] = triangle[point_2_idx];

            triangle_list[4][0] = point_pole_center;
            triangle_list[4][1] = point_pole_R; 
            triangle_list[4][2] = triangle[point_0_idx];

            triangle_list[5][0] = point_pole_R;
            triangle_list[5][1] = point_insert_010; 
            triangle_list[5][2] = triangle[point_0_idx];
        }else{
            // South pole
            triangle_list[0][0] = point_insert_010;
            triangle_list[0][1] = triangle[point_0_idx];
            triangle_list[0][2] = point_pole_L;

            triangle_list[1][0] = triangle[point_0_idx];
            triangle_list[1][1] = point_pole_center;
            triangle_list[1][2] = point_pole_L;

            triangle_list[2][0] = triangle[point_0_idx];
            triangle_list[2][1] = triangle[point_2_idx];
            triangle_list[2][2] = point_pole_center;

            triangle_list[3][0] = triangle[point_2_idx];
            triangle_list[3][1] = triangle[point_1_idx];
            triangle_list[3][2] = point_pole_center;

            triangle_list[4][0] = point_pole_center;
            triangle_list[4][1] = triangle[point_1_idx];
            triangle_list[4][2] = point_pole_R;

            triangle_list[5][0] = triangle[point_1_idx];
            triangle_list[5][1] = point_insert_011;
            triangle_list[5][2] = point_pole_R;
        }
    }else{
        // commit triangle, when don't compute OF and process next points
        if(triangle[point_0_idx].next.y >= 0){
            // North pole
            vertex_struct vertex_list[8];
            vertex_list[0] = point_insert_011;
            vertex_list[1] = point_pole_L;
            vertex_list[2] = triangle[point_1_idx];
            vertex_list[3] = point_pole_center;
            vertex_list[4] = triangle[point_2_idx];
            vertex_list[5] = triangle[point_0_idx];
            commit_vertex_list(vertex_list, 6);

            vertex_list[0] = point_pole_center;
            vertex_list[1] = point_pole_R;
            vertex_list[2] = triangle[point_0_idx];
            vertex_list[3] = point_insert_010;
            commit_vertex_list(vertex_list, 4);  
        }else{
            // South pole
            vertex_struct vertex_list[8];
            vertex_list[0] = point_insert_011;
            vertex_list[1] = point_pole_R;
            vertex_list[2] = triangle[point_1_idx];
            vertex_list[3] = point_pole_center;
            vertex_list[4] = triangle[point_2_idx];
            vertex_list[5] = triangle[point_0_idx];
            commit_vertex_list(vertex_list, 6);

            vertex_list[0] = point_pole_center;
            vertex_list[1] = point_pole_L;
            vertex_list[2] = triangle[point_0_idx];
            vertex_list[3] = point_insert_010;
            commit_vertex_list(vertex_list, 4);
        }
    }
}

void split_case_2_0(inout vertex_struct[3] triangle, in int split_line_idx,
                    in bool vertex_current, out vertex_struct[6][3] triangle_list)
{
    int split_start_point_idx; 
    if(split_line_idx == 1){
        split_start_point_idx =0;
    }else if(split_line_idx == 2){
        split_start_point_idx = 1;
    }else if(split_line_idx == 4){
        split_start_point_idx = 2;
    }

    int point_0_idx = int(mod(split_start_point_idx + 0, tile_point_number));
    int point_1_idx = int(mod(split_start_point_idx + 1, tile_point_number));
    int point_2_idx = int(mod(split_start_point_idx + 2, tile_point_number));

    vertex_struct point_insert_010;
    vertex_struct point_insert_011;
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011, vertex_current);

    vertex_struct point_insert_20 = vertex_struct(
        vec4(triangle[point_2_idx].current.xyz, - sign(triangle[point_0_idx].current.x) * 0.5), triangle[point_2_idx].next);
    vertex_struct point_insert_21 = vertex_struct(
        vec4(triangle[point_2_idx].current.xyz, - sign(triangle[point_1_idx].current.x) * 0.5), triangle[point_2_idx].next);

    if(compute_optical_flow && vertex_current){
        triangle_list[0][0] = triangle[point_0_idx];
        triangle_list[0][1] = point_insert_010;
        triangle_list[0][2] = point_insert_20;

        triangle_list[1][0] = point_insert_011;
        triangle_list[1][1] = triangle[point_1_idx];
        triangle_list[1][2] = point_insert_21;
    }else{
        vertex_struct[3] triangle_list;
        triangle_list[0] = triangle[point_0_idx];
        triangle_list[1] = point_insert_010;
        triangle_list[2] = point_insert_20;
        commit_triangle(triangle_list);

        triangle_list[0] = point_insert_011;
        triangle_list[1] = triangle[point_1_idx];
        triangle_list[2] = point_insert_21;
        commit_triangle(triangle_list);
    }
}

// the O_yx split two lines of triangle
void split_case_2(inout vertex_struct[3] triangle, in int split_line_idx, 
                    in bool vertex_current, out vertex_struct[6][3] triangle_list, out int triangles_number)
{

    // process the case, one of the split point is 0
    if(split_line_idx == 1 || split_line_idx == 2 || split_line_idx == 4)
    {
        split_case_2_0(triangle, split_line_idx, vertex_current, triangle_list);
        triangles_number = 2;
        return;
    } 

    triangles_number = 3;
    // 0) generate the additional points for two split-up tile
    int split_start_point_idx; 
    if(split_line_idx == 3){
        split_start_point_idx = 0;
    }else if(split_line_idx == 6){
        split_start_point_idx = 1;
    }else if(split_line_idx == 5){
        split_start_point_idx = 2;
    }

    int point_0_idx = int(mod(split_start_point_idx + 0, tile_point_number));
    int point_1_idx = int(mod(split_start_point_idx + 1, tile_point_number));
    int point_2_idx = int(mod(split_start_point_idx + 2, tile_point_number));

    vertex_struct point_insert_010;
    vertex_struct point_insert_011;
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011, vertex_current);
    
    vertex_struct point_insert_121; 
    vertex_struct point_insert_122; 
    split_line(triangle[point_1_idx],triangle[point_2_idx], point_insert_121, point_insert_122, vertex_current);

    // 1) commit or return tiles
    if(compute_optical_flow && vertex_current){
        triangle_list[0][0] = triangle[point_0_idx];
        triangle_list[0][1] = point_insert_010;
        triangle_list[0][2] = triangle[point_2_idx];

        triangle_list[1][0] = point_insert_010;
        triangle_list[1][1] = point_insert_122;
        triangle_list[1][2] = triangle[point_2_idx];

        triangle_list[2][0] = point_insert_011;
        triangle_list[2][1] = triangle[point_1_idx];
        triangle_list[2][2] = point_insert_121;
    }else{
        vertex_struct[4] quadrilateral_list;
        quadrilateral_list[0] = triangle[point_0_idx];
        quadrilateral_list[1] = point_insert_010;
        quadrilateral_list[2] = triangle[point_2_idx];
        quadrilateral_list[3] = point_insert_122;
        commit_quadrilateral(quadrilateral_list);

        vertex_struct[3] triangle_list_commit;
        triangle_list_commit[0] = point_insert_011;
        triangle_list_commit[1] = triangle[point_1_idx];
        triangle_list_commit[2] = point_insert_121;
        commit_triangle(triangle_list_commit);
    }
}

void process_triangle(in vertex_struct[6][3] vertex_list, in int vertex_numb)
{
    for (int i = 0 ; i < vertex_numb;i++)
    {
        // get the index of split line
        int split_counter = 0;
        int split_line_idx = 0;
        test_split_triangle(vertex_list[i], split_counter, split_line_idx, false);

        // when 0_yx cross the tile, split the quad tile to triangles
        if(split_counter == 0){
            // do not need split
            vertex_struct[6][3] triangle_list;
            split_case_0(vertex_list[i], false, triangle_list);
        }else if (split_counter == 1){
            // the pole in this tile
            vertex_struct[6][3] triangle_list;
            split_case_1(vertex_list[i], split_line_idx, false, triangle_list);
        }else if(split_counter == 2){
            // O_yz through this tile, split the tile with next vertex 
            vertex_struct[6][3] triangle_list;
            int triangle_number;
            split_case_2(vertex_list[i], split_line_idx, false, triangle_list, triangle_number);
        }
    }
}


void process_quadrilateral(inout vertex_struct[4] vertex_list)
{
    // split an Quadrilateral to 2 triangles
    //   0------> 1              0------ >1 
    //   ^        |      --->    |  \     |
    //   |        |      --->    |    \   | 
    //   |        V              |      \ V
    //   3<------ 2              3<------ 2  
    vertex_struct triangles[2][3];
    triangles[0][0] = vertex_list[0];
    triangles[0][1] = vertex_list[1];
    triangles[0][2] = vertex_list[2];
    // check_triangle_direction(triangles[0]);

    triangles[1][0] = vertex_list[0];
    triangles[1][1] = vertex_list[2];
    triangles[1][2] = vertex_list[3];
    // check_triangle_direction(triangles[1]);

    for (int i = 0 ; i < 2 ;i++)
    {    
        // figure out the line will be split
        int split_counter = 0;
        int split_line_idx = 0;
        test_split_triangle(triangles[i], split_counter, split_line_idx, true);

        // when 0_yz cross the tile, split the quad tile to triangles
        vertex_struct[6][3] triangle_list;
        if(split_counter == 0){
            // do not need split
            split_case_0(triangles[i], true, triangle_list);
            if(compute_optical_flow){
                process_triangle(triangle_list, 1);
            }
        }else if (split_counter == 1){
            // the pole in this tile
            split_case_1(triangles[i], split_line_idx, true, triangle_list);
            if(compute_optical_flow){
                process_triangle(triangle_list, 6);
            }
        }else if(split_counter == 2){
            // O_y-z through this tile, split the tile with current vertex 
            int triangles_number;
            split_case_2(triangles[i], split_line_idx, true, triangle_list, triangles_number);
            if(compute_optical_flow){
                process_triangle(triangle_list, triangles_number);
            }
        }
    }
}

void main()
{
    vertex_struct[4] vertex_list;
    mat4 cs_trans = get_coordinate_system_transform();
    vertex_list[0] = vertex_struct(cs_trans * gl_in[0].gl_Position, cs_trans * pos_next[0]);
    vertex_list[1] = vertex_struct(cs_trans * gl_in[1].gl_Position, cs_trans * pos_next[1]);
    vertex_list[2] = vertex_struct(cs_trans * gl_in[2].gl_Position, cs_trans * pos_next[2]);;
    vertex_list[3] = vertex_struct(cs_trans * gl_in[3].gl_Position, cs_trans * pos_next[3]);
    process_quadrilateral(vertex_list);
}