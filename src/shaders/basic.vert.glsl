#version 330 core

uniform vec2 res; // resolution
uniform vec2 verts[4];

uniform sampler2D font_image;

layout (location = 0)  in vec2 dst_p0; // top-left on screen
layout (location = 1)  in vec2 dst_p1; // bottom-right on screen
layout (location = 2)  in vec2 src_p0; // top-left of texture
layout (location = 3)  in vec2 src_p1; // bottom-left of texture
layout (location = 4)  in vec4 colors[4]; // takes up for slots
layout (location = 8)  in float corner_radius;
layout (location = 9)  in float edge_softness;
layout (location = 10) in float border_thickness;

// outputs

out vec4 color0;
out vec2 uv0;
out vec2 dst_half_size0;
out vec2 dst_center0;
out vec2 dst_pos0;
out float corner_radius0;
out float edge_softness0;
out float border_thickness0;

void main()
{

    vec2 dst_half_size = (dst_p1 - dst_p0) / 2.0;
    vec2 dst_center    = (dst_p1 + dst_p0) / 2.0;
    vec2 dst_pos       = (verts[gl_VertexID] * dst_half_size + dst_center);

    vec2 src_half_size = (src_p1 - src_p0) / 2.0;
    vec2 src_center    = (src_p1 + src_p0) / 2.0;
    vec2 src_pos       = (verts[gl_VertexID] * src_half_size + src_center);

    gl_Position = vec4(2.0 * dst_pos.x / res.x - 1.0,
                       2.0 * dst_pos.y / res.y - 1.0,
                       0.0,
                       1.0);

    gl_Position.y *= -1;

    color0 = colors[gl_VertexID];
    uv0 = vec2(src_pos.x, src_pos.y);

    dst_half_size0 = dst_half_size;
    dst_center0 = dst_center;
    dst_pos0 = dst_pos;
    
    corner_radius0 = corner_radius;
    edge_softness0 = edge_softness;
    border_thickness0 = border_thickness;
}
