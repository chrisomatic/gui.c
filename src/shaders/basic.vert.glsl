#version 330 core

uniform vec2 res; // resolution
uniform vec2 verts[4];

layout (location = 0) in vec2 p0;
layout (location = 1) in vec2 p1;
layout (location = 2) in vec4 color;

out vec4 color0;

void main()
{
    vec2 dst_half_size = (p1 - p0) / 2;
    vec2 dst_center    = (p1 + p0) / 2;
    vec2 dst_pos       = (verts[gl_VertexID] * dst_half_size + dst_center);

    gl_Position = vec4(2.0 * dst_pos.x / res.x - 1.0,
                       2.0 * dst_pos.y / res.y - 1.0,
                       0.0,
                       1.0);

    gl_Position.y *= -1;

    color0 = color;
}
