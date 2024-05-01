
//
// API:
//
// void draw_clear_screen(float r, float g, float b);
// void draw_rect(float x, float y, float w, float h, Vec4f color);
// void draw_rect_frame(float x, float y, float w, float h, Vec4f color, float border_thickness);
// void draw_rect_vgrad(float x, float y, float w, float h, Vec4f color1, Vec4f color2);
// void draw_rect_hgrad(float x, float y, float w, float h, Vec4f color1, Vec4f color2);
// void draw_set_rect_corner_radius(float r);
// void draw_set_rect_edge_softness(float v);
// void draw_string(float x, float y, float scale, Vec4f color, char* format, ...);
// void draw_commit(); // needs to be called at the end of frame
//

#define MAX_RECTS 1024

#define WHITE   color(1.0,1.0,1.0)
#define BLACK   color(0.0,0.0,0.0)
#define GRAY    color(0.5,0.5,0.5)
#define RED     color(1.0,0.0,0.0)
#define GREEN   color(0.0,1.0,0.0)
#define BLUE    color(0.0,0.0,1.0)
#define MAGENTA color(1.0,0.0,1.0)
#define YELLOW  color(1.0,1.0,0.0)
#define CYAN    color(0.0,1.0,1.0)

#define FONT_PATH_IMAGE  "src/fonts/atlas.png"
#define FONT_PATH_LAYOUT "src/fonts/atlas_layout.csv"

static GLuint vao;
static GLuint vbo;

typedef struct
{
    float x,y;
} Vec2f;

typedef struct
{
    float x,y,z,w;
} Vec4f;

typedef struct
{
    Vec2f p0;     // top left on screen
    Vec2f p1;     // bottom right on screen
    Vec2f tex_p0; // top left on texture
    Vec2f tex_p1; // bottom right on texture
    Vec4f colors[4];
    float corner_radius;
    float edge_softness;
    float border_thickness;
} DrawRect;

typedef struct
{
    int w,h,n;
    unsigned char* data;
    int texture;
} Image;

typedef struct
{
    float l,b,r,t;
} CharBox;

typedef struct
{
    float advance;
    CharBox plane_box;
    CharBox pixel_box;
    CharBox tex_coords;
    float w,h;
} FontChar;

static FontChar font_chars[255];
Image font_image = {0};

DrawRect queued_rects[MAX_RECTS] = {0};
int  rect_count = 0;

bool scale_view = false;
int default_corner_radius = 2.0;
int default_edge_softness = 1.0;

GLuint loc_res;
GLuint loc_font_image;
GLuint loc_verts[4];

Vec4f color(float r, float g, float b)
{
    Vec4f c = {r,g,b,1.0};
    return c;
}

Vec4f colora(float r, float g, float b, float a)
{
    Vec4f c = {r,g,b,a};
    return c;
}

bool load_image(const char* image_path, Image* image, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);
    image->data = stbi_load(image_path, &image->w, &image->h, &image->n, 4);

    if(!image->data)
    {
        loge("Failed to load image: %s", image_path);
        return false;
    }

    logi("Loaded image: %s (w: %d, h: %d, n: %d)", image_path, image->w, image->h, image->n);

    glGenTextures(1, &image->texture);

	glBindTexture(GL_TEXTURE_2D, image->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void load_font()
{
    bool loaded = load_image(FONT_PATH_IMAGE, &font_image, false);
    if(!loaded) return;

    logi("Font loaded at index: %d", font_image.texture);

    FILE* fp = fopen(FONT_PATH_LAYOUT,"r");

    if(!fp)
    {
        logw("Failed to load font layout file");
        return;
    }

    char line[256] = {0};

    while(fgets(line,255,fp) != NULL)
    {
        int char_index = 0;
        float advance, pl_l, pl_b, pl_r, pl_t,  px_l, px_b, px_r, px_t;

        int n = sscanf(line,"%d,%f,%f,%f,%f,%f,%f,%f,%f,%f ",&char_index,&advance,&pl_l, &pl_b, &pl_r, &pl_t, &px_l, &px_b, &px_r, &px_t);

        if(n != 10)
            continue;

        font_chars[char_index].advance = advance;

        font_chars[char_index].plane_box.l = pl_l;
        font_chars[char_index].plane_box.b = pl_b;
        font_chars[char_index].plane_box.r = pl_r;
        font_chars[char_index].plane_box.t = pl_t;

        font_chars[char_index].pixel_box.l = px_l;
        font_chars[char_index].pixel_box.b = px_b;
        font_chars[char_index].pixel_box.r = px_r;
        font_chars[char_index].pixel_box.t = px_t;

        font_chars[char_index].tex_coords.l = px_l/404.0;
        font_chars[char_index].tex_coords.b = 1.0 - (px_b/404.0);
        font_chars[char_index].tex_coords.r = px_r/404.0;
        font_chars[char_index].tex_coords.t = 1.0 - (px_t/404.0);

        font_chars[char_index].w = px_r - px_l;
        font_chars[char_index].h = px_b - px_t;

        // printf("font_char %d: %f [%f %f %f %f], [%f %f %f %f], [%f %f %f %f]\n",char_index,advance,pl_l,pl_b,pl_r,pl_t,px_l,px_b,px_r,px_t,px_l/512.0,px_b/512.0,px_r/512.0,px_t/512.0);
    }

    fclose(fp);
}

void draw_init()
{
    logi("GL version: %s",glGetString(GL_VERSION));

    memset(queued_rects, 0, MAX_RECTS*sizeof(DrawRect));
    rect_count = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RECTS*sizeof(DrawRect), NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(0)); // p0
    glVertexAttribDivisor(0, 1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(8)); // p1
    glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(16)); // tex_p0
    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(24)); // tex_p1
    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(32)); // colors[0]
    glVertexAttribDivisor(4, 1);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(48)); // colors[1]
    glVertexAttribDivisor(5, 1);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(64)); // colors[2]
    glVertexAttribDivisor(6, 1);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(80)); // colors[3]
    glVertexAttribDivisor(7, 1);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(96)); // corner_radius
    glVertexAttribDivisor(8, 1);
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(100)); // edge_softness
    glVertexAttribDivisor(9, 1);
    glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, sizeof(DrawRect),(const GLvoid*)(104)); // border_radius
    glVertexAttribDivisor(10, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loc_res = glGetUniformLocation(program, "res");
    loc_font_image = glGetUniformLocation(program, "font_image");

    loc_verts[0] = glGetUniformLocation(program, "verts[0]");
    loc_verts[1] = glGetUniformLocation(program, "verts[1]");
    loc_verts[2] = glGetUniformLocation(program, "verts[2]");
    loc_verts[3] = glGetUniformLocation(program, "verts[3]");

    load_font();

}

void draw_clear_screen(float r, float g, float b)
{
    glClearColor(r, g, b, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw_rect_full(float x, float y, float w, float h, Vec4f color1, Vec4f color2, bool gradient_horizontal, float border_thickness, float corner_radius, float edge_softness)
{
    if(rect_count >= MAX_RECTS)
    {
        logw("Hit rect count max, failed to queue drawing routine");
        return;
    }

    DrawRect* rect = &queued_rects[rect_count++];

    rect->p0.x = x;
    rect->p0.y = y;
    rect->p1.x = x+w;
    rect->p1.y = y+h;

    rect->tex_p0.x = 0.0;
    rect->tex_p0.y = 0.0;
    rect->tex_p1.x = 0.0;
    rect->tex_p1.y = 0.0;

    for(int i = 0; i < 4; ++i)
    {
        if(gradient_horizontal)
        {
            rect->colors[i].x = i < 2 ? color1.x : color2.x;
            rect->colors[i].y = i < 2 ? color1.y : color2.y;
            rect->colors[i].z = i < 2 ? color1.z : color2.z;
            rect->colors[i].w = i < 2 ? color1.w : color2.w;
        }
        else
        {
            rect->colors[i].x = (i == 0 || i == 2) ? color1.x : color2.x;
            rect->colors[i].y = (i == 0 || i == 2) ? color1.y : color2.y;
            rect->colors[i].z = (i == 0 || i == 2) ? color1.z : color2.z;
            rect->colors[i].w = (i == 0 || i == 2) ? color1.w : color2.w;
        }
    }

    rect->corner_radius = corner_radius;
    rect->edge_softness = edge_softness;
    rect->border_thickness = border_thickness;
}

void draw_rect(float x, float y, float w, float h, Vec4f color)
{
    draw_rect_full(x, y, w, h, color, color, true, 0.0, default_corner_radius, default_edge_softness);
}

void draw_rect_frame(float x, float y, float w, float h, Vec4f color, float border_thickness)
{
    draw_rect_full(x, y, w, h, color, color, true, border_thickness, default_corner_radius, default_edge_softness);
}

void draw_rect_hgrad(float x, float y, float w, float h, Vec4f color1, Vec4f color2)
{
    draw_rect_full(x, y, w, h, color1, color2, true, 0.0, default_corner_radius, default_edge_softness);
}

void draw_rect_vgrad(float x, float y, float w, float h, Vec4f color1, Vec4f color2)
{
    draw_rect_full(x, y, w, h, color1, color2, false, 0.0, default_corner_radius, default_edge_softness);
}

void draw_set_rect_corner_radius(float r)
{
    default_corner_radius = r;
}
void draw_set_rect_edge_softness(float v)
{
    default_edge_softness = v;
}

// w,h
Vec2f string_get_size(float scale, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    float x_pos = 0.0;
    float fontsize = 64.0 * scale;
    int num_lines = 1;
    float longest_width = 0.0;

    char* c = str;

    for(;;)
    {
        if(*c == '\0')
            break;

        if(*c == '\n')
        {
            num_lines++;
            if(x_pos > longest_width)
                longest_width = x_pos;
            x_pos = 0.0;
            c++;
            continue;
        }

        FontChar* fc = &font_chars[*c];

        x_pos += (fontsize*fc->advance);
        c++;
    }

    if(x_pos > longest_width)
        longest_width = x_pos;

    Vec2f ret = {longest_width, fontsize*num_lines};
    return ret;
}

void draw_string(float x, float y, float scale, Vec4f color, char* format, ...)
{
    va_list args;
    va_start(args, format);
    char str[256] = {0};
    vsprintf(str, format, args);
    va_end(args);

    float fontsize = 64.0 * scale;

    float x_pos = x;
    float y_pos = y+fontsize;

    int num_lines = 1;

    char* c = str;

    for(;;)
    {
        if(*c == '\0')
            break;

        if(*c == '\n')
        {
            y_pos += fontsize;
            x_pos = x;
            num_lines++;
            c++;
            continue;
        }

        if(rect_count >= MAX_RECTS)
        {
            logw("Hit rect count max, failed to queue drawing routine");
            break;
        }

        FontChar* fc = &font_chars[*c];

        float x0 = x_pos + fontsize*fc->plane_box.l;
        float y0 = y_pos - fontsize*fc->plane_box.t;
        float x1 = x_pos + fontsize*fc->plane_box.r;
        float y1 = y_pos - fontsize*fc->plane_box.b;

        DrawRect* rect = &queued_rects[rect_count++];

        rect->p0.x = x0;
        rect->p0.y = y0;
        rect->p1.x = x1;
        rect->p1.y = y1;

        rect->tex_p0.x = fc->tex_coords.l;
        rect->tex_p0.y = fc->tex_coords.t;
        rect->tex_p1.x = fc->tex_coords.r;
        rect->tex_p1.y = fc->tex_coords.b;

        for(int i = 0; i < 4; ++i)
        {
            rect->colors[i].x = color.x;
            rect->colors[i].y = color.y;
            rect->colors[i].z = color.z;
            rect->colors[i].w = color.w;
        }

        c++;
        x_pos += (fontsize*fc->advance);
    }
}

void draw_commit()
{
    glUseProgram(program);
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_image.texture);
    glUniform1i(loc_font_image, 0);

    if(scale_view)
    {
        glUniform2f(loc_res,(float)view_width, (float)view_height);
    }
    else
    {
        glUniform2f(loc_res,(float)window_width, (float)window_height);
    }

    glUniform2f(loc_verts[0], -1.0, -1.0);
    glUniform2f(loc_verts[1], -1.0, +1.0);
    glUniform2f(loc_verts[2], +1.0, -1.0);
    glUniform2f(loc_verts[3], +1.0, +1.0);

    // buffer new rect data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, rect_count*sizeof(DrawRect), queued_rects, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);
    glEnableVertexAttribArray(9);
    glEnableVertexAttribArray(10);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rect_count); 
    rect_count = 0;

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);
    glDisableVertexAttribArray(7);
    glDisableVertexAttribArray(8);
    glDisableVertexAttribArray(9);
    glDisableVertexAttribArray(10);

    glBindVertexArray(0);
    glUseProgram(0);
}
