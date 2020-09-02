// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core

layout(location = 0) out vec4 optical_flow;

smooth in vec4 vpos;
smooth in vec4 vposNext;

uniform vec2 window_size;

void main()
{
    // output the diff x
    float vpos_image_x = (vpos.x + 1.0f ) / 2.0f * window_size.x;
    float vposNext_image_x = (vposNext.x + 1.0f ) / 2.0f * window_size.x;
    float diff_x_forward =  vposNext_image_x - vpos_image_x;

    // output the diff y
    float vpos_image_y = (vpos.y + 1.0f ) / 2.0f * window_size.y;
    float vposNext_image_y = (vposNext.y + 1.0f ) / 2.0f * window_size.y;
    float diff_y_forward = vposNext_image_y - vpos_image_y;

    optical_flow = vec4(diff_x_forward, diff_y_forward,  0.0, 1.0f);
}
