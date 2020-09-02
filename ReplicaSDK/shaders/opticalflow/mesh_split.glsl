//-----------------------------------------------
// This file contains the common functions for rendering depth and RGB images
//-----------------------------------------------

// the point warping around need clip, and interpolate to generate the new point
// interpolate current & next points when split line, and process the warping round of next vertex.
void split_line(in vertex_struct point_1st, in vertex_struct point_2nd,
                  out vertex_struct point_1st_inserted, out vertex_struct point_2nd_inserted)
{
    // 1) split the line of current points to generate the interpolated point for first & second 
    // float ratio = (1.0 -  point_1st.position.x) / (2.0 - point_1st.position.x + point_2nd.position.x);
    float ratio = abs(point_1st.position.x / (point_2nd.position.x - point_1st.position.x));
    // if (ratio == 0)
    //     return;
    vec2 interpolated = ratio * (point_2nd.position.yz - point_1st.position.yz) + point_1st.position.yz;
    if (point_1st.position.x < 0.0)
    {
        point_1st_inserted.position = vec4(0.0, interpolated.xy, +0.5); // +PI
        point_2nd_inserted.position = vec4(0.0, interpolated.xy, -0.5); // -PI
    }else{
        point_1st_inserted.position = vec4(0.0, interpolated.xy, -0.5); // -PI
        point_2nd_inserted.position = vec4(0.0, interpolated.xy, +0.5); // +PI
    }

    // 2) split corresponding line of next points to generate the interpolated point for first & second 
    vec2 uv = ratio * (point_2nd.uv - point_1st.uv) + point_1st.uv;
    point_1st_inserted.uv = uv;
    point_2nd_inserted.uv = uv;
}


// generate the center's next with current point position
void triangle_center(in vertex_struct[3] center_triangle, out vertex_struct center)
{
    center.position = vec4(0, 0, 0, 1.0);
    center.uv = vec2(0, 0);

    // 0) compute Y
    vec3 normal = cross(center_triangle[1].position.xyz - center_triangle[0].position.xyz, 
                        center_triangle[2].position.xyz - center_triangle[0].position.xyz);
    center.position.y = dot(normal.xyz, center_triangle[0].position.xyz) / normal.y;

    // 1) interpolate 
    float[3] weight;
    float weight_sum = 0;
    for(int idx = 0; idx < 3; idx++){
        weight[idx] = length(center_triangle[idx].position.xyz - center.position.xyz);
        weight_sum += weight[idx];
    }

    center.uv = center_triangle[0].uv * (weight[0] / weight_sum)
                  + center_triangle[1].uv * (weight[1] / weight_sum)
                  + center_triangle[2].uv * (weight[2] / weight_sum);
}

// counting the split edges number of current points
void test_split_triangle(inout vertex_struct vertex_list[tile_point_number], 
                    out int split_counter, out int split_line_idx)
{
    split_counter = 0;
    split_line_idx = 0;

    for(int idx = 0; idx < tile_point_number; idx++)
    {
        int idx_next = int(mod(idx + 1, tile_point_number));
        if(test_split_line(vertex_list[idx].position, vertex_list[idx_next].position))
        {
            split_counter++;
            split_line_idx += int(pow(2, idx));
        }
    }

    // double-check whether poles (x=0,z=0) in the triangle
    if(split_counter == 1)
    {
        vec2 a = vec2(vertex_list[0].position.xz);
        vec2 ab = vec2(vertex_list[1].position.xz) - a;
        vec2 ac = vec2(vertex_list[2].position.xz) - a;
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


void split_case_0(inout vertex_struct triangles[3])
{
    // correct the case 1 in the points
    bool contain_zero = (triangles[0].position.x == 0.0 && triangles[0].position.z < 0)
                    || (triangles[1].position.x == 0.0 && triangles[1].position.z < 0)
                    || (triangles[2].position.x == 0.0 && triangles[2].position.z < 0);

    if(contain_zero)
    {
        bool larger_than_zero = (triangles[0].position.x > 0.0)
                        || (triangles[1].position.x > 0.0)
                        || (triangles[2].position.x > 0.0);

        for(int i = 0 ; i< triangles.length(); i++){
            if(triangles[i].position.x == 0.0)
            {
                if(larger_than_zero){
                    triangles[i].position.w = -0.5;
                }else if(!larger_than_zero){
                    triangles[i].position.w = +0.5;
                }
            }
        }
    }

    commit_triangle(triangles); 
}


void split_case_1(in vertex_struct triangle[3], in int split_line_idx)
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
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011);

    // 1) generate the triangles in pole point
    // generate the current center point for split the triangle with next points
    // Pole_L(Left) ---> pole ---> Pole_R(Right)
    vertex_struct center;
    triangle_center(triangle, center); // interpolate the center's next value
    vertex_struct point_pole_L = vertex_struct(center.position, center.uv);
    vertex_struct point_pole_center = vertex_struct(center.position, center.uv);
    vertex_struct point_pole_R = vertex_struct(center.position, center.uv);
     
    point_pole_L.position = vec4(0.0, center.position.y, 0.0, -0.5);// Left corner
    point_pole_center.position = vec4(0.0, center.position.y, 0.0, 0.0);// north pole
    point_pole_R.position = vec4(0.0, center.position.y, 0.0, 0.5);// Right corner

    // 2) commit or return tiles
    // commit the split triangles
    if(triangle[point_0_idx].position.y >= 0){
        // North pole
        vertex_struct vertex_list[8];
        vertex_list[0] = point_insert_010;
        vertex_list[1] = point_pole_R;
        vertex_list[2] = triangle[point_0_idx];
        vertex_list[3] = point_pole_center;
        vertex_list[4] = triangle[point_2_idx];
        vertex_list[5] = triangle[point_1_idx];
        commit_vertex_list(vertex_list, 6);  

        vertex_list[0] = point_pole_center;
        vertex_list[1] = point_pole_L;
        vertex_list[2] = triangle[point_1_idx];
        vertex_list[3] = point_insert_011;
        commit_vertex_list(vertex_list, 4);  
    }
    else{
        // South pole
        vertex_struct vertex_list[8];
        vertex_list[0] = point_insert_010;
        vertex_list[1] = point_pole_L;
        vertex_list[2] = triangle[point_0_idx];
        vertex_list[3] = point_pole_center;
        vertex_list[4] = triangle[point_2_idx];
        vertex_list[5] = triangle[point_1_idx];
        commit_vertex_list(vertex_list, 6);

        vertex_list[0] = point_pole_center;
        vertex_list[1] = point_pole_R;
        vertex_list[2] = triangle[point_1_idx];
        vertex_list[3] = point_insert_011;
        commit_vertex_list(vertex_list, 4);       
    }
}


void split_case_2_0(inout vertex_struct[3] triangle, in int split_line_idx)
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
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011);

    vertex_struct point_insert_20 = vertex_struct(
        vec4(triangle[point_2_idx].position.xyz, - sign(triangle[point_0_idx].position.x) * 0.5), 
        vec2(triangle[point_2_idx].uv));
    vertex_struct point_insert_21 = vertex_struct(
        vec4(triangle[point_2_idx].position.xyz, - sign(triangle[point_1_idx].position.x) * 0.5), 
        vec2(triangle[point_2_idx].uv));

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


// the O_yx split two lines of triangle
void split_case_2(inout vertex_struct[3] triangle, in int split_line_idx)
{
    // process the case, one of the split point is 0
    if(split_line_idx == 1 || split_line_idx == 2 || split_line_idx == 4)
    {
        split_case_2_0(triangle, split_line_idx);
        return;
    } 

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
    split_line(triangle[point_0_idx], triangle[point_1_idx], point_insert_010, point_insert_011);
    
    vertex_struct point_insert_121; 
    vertex_struct point_insert_122; 
    split_line(triangle[point_1_idx],triangle[point_2_idx], point_insert_121, point_insert_122);

    // 1) commit or return tiles
    vertex_struct[4] quadrilateral_list;
    quadrilateral_list[0] = triangle[point_0_idx];
    quadrilateral_list[1] = point_insert_010;
    quadrilateral_list[2] = triangle[point_2_idx];
    quadrilateral_list[3] = point_insert_122;
    commit_quadrilateral(quadrilateral_list);

    vertex_struct[3] triangle_list;
    triangle_list[0] = point_insert_011;
    triangle_list[1] = triangle[point_1_idx];
    triangle_list[2] = point_insert_121;
    commit_triangle(triangle_list);
}


void process_quadrilateral()
{
    vertex_struct[4] vertex_list;
    if(gl_in.length() == 3){
        return;
    }else if(gl_in.length() == 4){
        mat4 cs_trans = get_coordinate_system_transform();
        vertex_list[0] = vertex_struct(cs_trans * gl_in[0].gl_Position, vec2(0.0, 0.0));
        vertex_list[1] = vertex_struct(cs_trans * gl_in[1].gl_Position, vec2(1.0, 0.0));
        vertex_list[2] = vertex_struct(cs_trans * gl_in[2].gl_Position, vec2(1.0, 1.0));;
        vertex_list[3] = vertex_struct(cs_trans * gl_in[3].gl_Position, vec2(0.0, 1.0));
    }

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
        test_split_triangle(triangles[i], split_counter, split_line_idx);

        // when 0_yx cross the tile, split the quad tile to triangles
        if(split_counter == 0){
            // do not need split
            split_case_0(triangles[i]);
        }else if (split_counter == 1){
            // the pole in this tile
            split_case_1(triangles[i], split_line_idx);
        }else if(split_counter == 2){
            // O_yz through this tile, split the tile with current vertex 
            split_case_2(triangles[i], split_line_idx);
        }
    }
}