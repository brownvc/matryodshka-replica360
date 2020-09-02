#define M_PI 3.1415926535897932384626433832795
#define tile_point_number 3

// the near and far sphere of panoramic camera
const float near = -1.0;
const float far = 50.0;

// the optical flow code on coordinate system (CS) the +x left, +y up, +z forward
// transform the OpenGL coordinate system (-x left, -y up, +z forward)
mat4 get_coordinate_system_transform()
{
    mat4 cs_trans = mat4(-1.0,  0.0,  0.0,  0.0,
                            0.0, -1.0,  0.0,  0.0, 
                            0.0,  0.0,  1.0,  0.0,
                            0.0,  0.0,  0.0,  1.0);
    return cs_trans;
}


// transform the cartesian coordinate to spherical CS
vec4 cartesian_2_sphere(inout vec4 cart)
{
    // return cart;
    vec4 sphere;
    if(cart.w == 0.5){
        sphere.x = 1.0;
    }else if(cart.w == 0.0){
        sphere.x = 0;
    }else if(cart.w == -0.5){
        sphere.x = -1.0;
    }else{
        sphere.x = -atan(cart.x, cart.z)  / M_PI;
    }
    sphere.x = sphere.x;
    //flip-y since the texture origin at Left-Bottom
    sphere.y = - atan(cart.y, length(cart.xz))  / (M_PI / 2); 
    sphere.z = -1 + 2 * (length(cart.xyz) - near) / (far - near);
    sphere.w = 1.0;
    return sphere;
}


// test whether the line cross +O_yz plane, if need split return true, else return false
bool test_split_line(in vec4 vertex_1st, in vec4 vertex_2nd)
{
    // the two point on the same side of O_yz or the line parallax to O_yz 
    if(vertex_1st.x * vertex_2nd.x >= 0 || 
        (vertex_1st.z >= 0 && vertex_2nd.z >= 0 )) 
        return false;

    // compute interpolate z
    float ratio = abs(vertex_1st.x / (vertex_2nd.x - vertex_1st.x));
    float inter_z = ratio * (vertex_2nd.z - vertex_1st.z ) + vertex_1st.z;
    if (inter_z <= 0){
        return true;
    }else{
        return false;
    }
}