// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#version 430 core
#include "atlas.glsl"

layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D colorTex;
layout(binding = 1) uniform sampler2D depthTex;
layout(binding = 2) uniform sampler2D alphaTex;
layout(binding = 3) uniform sampler2D blendTex;

uniform float exposure;
uniform float gamma;
uniform float saturation;

uniform int render_layered;
uniform int first_pass;
uniform int show_depth = 0;

in vec2 uv_frag;
in vec2 uv_screen;

void main()
{
    // Texture
    vec2 uv = vec2(1 - uv_frag.x, uv_frag.y);

    vec4 c = texture2D(colorTex, uv);
    vec4 a = texture2D(alphaTex, uv);
    vec4 d = texture2D(depthTex, uv);
    vec4 b = texture2D(blendTex, uv_screen);

    // Apply image transforms
    c *= exposure;
    applySaturation(c, saturation);
    c.rgb = pow(c.rgb, vec3(gamma));

    // Depth
    if(show_depth == 1) {
        c = d;
    }

    // Output color
    if(render_layered == 0 || first_pass == 1) {
        FragColor = vec4(c.rgb, 1);
    }
    else {
        FragColor = vec4(mix(b.rgb, c.rgb, a.x), 1);
    }
}
