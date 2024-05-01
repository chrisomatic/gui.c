#version 330 core

in vec4 color0;
in vec2 uv0;

in vec2 dst_half_size0;
in vec2 dst_center0;
in vec2 dst_pos0;
in float corner_radius0;
in float edge_softness0;
in float border_thickness0;

out vec4 frag_color;

uniform sampler2D font_image;

float screenPxRange() {
    vec2 unitRange = vec2(4.0)/vec2(textureSize(font_image, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(uv0);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float RoundedRectSDF(vec2 sample_pos,vec2 rect_center,vec2 rect_half_size,float r)
{
    vec2 d2 = (abs(rect_center - sample_pos) - rect_half_size + vec2(r, r));
    return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}

void main()
{
    if(uv0.x > 0.0 || uv0.y > 0.0)
    {
        // draw font
        vec3 msd = texture(font_image, uv0.xy).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = screenPxRange()*(sd - 0.5);
        float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

        frag_color = vec4(color0.rgb, opacity*color0.a);
    }
    else
    {
        // basic rects

        float softness = edge_softness0;
        vec2  softness_padding = vec2(max(0, softness*2-1),max(0, softness*2-1));

        // sample distance
        float dist = RoundedRectSDF(dst_pos0,dst_center0,dst_half_size0-softness_padding, corner_radius0);

        // map distance => a blend factor
        float sdf_factor = 1.f - smoothstep(0, 2*softness, dist);
        
        float border_factor = 1.f;
        if(border_thickness0 != 0.0)
        {
            vec2 interior_half_size = dst_half_size0 - vec2(border_thickness0);

            float interior_radius_reduce_f = 
                min(interior_half_size.x/dst_half_size0.x,
                interior_half_size.y/dst_half_size0.y);
            float interior_corner_radius = corner_radius0 * interior_radius_reduce_f * interior_radius_reduce_f;

            // calculate sample distance from "interior"
            float inside_d = RoundedRectSDF(dst_pos0, dst_center0,
                                          interior_half_size-softness_padding,
                                          interior_corner_radius);

            // map distance => factor
            float inside_f = smoothstep(0, 2*softness, inside_d);
            border_factor = inside_f;
        }

        frag_color = color0 * sdf_factor * border_factor;
    }
}
