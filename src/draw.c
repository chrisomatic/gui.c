
#define MAX_RECTS 1024

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
    Vec2f p0; // top left
    Vec2f p1; // bottom right
    Vec4f color;
} Rect;

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
int font_image;

Rect queued_rects[MAX_RECTS] = {0};
int  rect_count = 0;

GLuint loc_res;
GLuint loc_verts[4];

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
    Image font_img = {0};

    bool loaded = load_image(FONT_PATH_IMAGE, &font_img, true);
    if(!loaded) return;

    logi("Font loaded at index: %d", font_img.texture);

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

    memset(queued_rects, 0, MAX_RECTS*sizeof(Rect));
    rect_count = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RECTS*sizeof(Rect), NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rect),(const GLvoid*)(0)); // p0
    glVertexAttribDivisor(0, 1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Rect),(const GLvoid*)(8)); // p1
    glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Rect),(const GLvoid*)(16)); // color
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loc_res = glGetUniformLocation(program, "res");

    loc_verts[0] = glGetUniformLocation(program, "verts[0]");
    loc_verts[1] = glGetUniformLocation(program, "verts[1]");
    loc_verts[2] = glGetUniformLocation(program, "verts[2]");
    loc_verts[3] = glGetUniformLocation(program, "verts[3]");

    load_font();

}

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

void draw_clear_screen(float r, float g, float b)
{
    glClearColor(r, g, b, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw_rect(float x0, float y0, float x1, float y1, Vec4f color)
{
    if(rect_count >= MAX_RECTS)
    {
        logw("Hit rect count max, failed to queue drawing routine");
        return;
    }

    Rect* rect = &queued_rects[rect_count++];

    rect->p0.x = x0;
    rect->p0.y = y0;
    rect->p1.x = x1;
    rect->p1.y = y1;

    rect->color.x = color.x;
    rect->color.y = color.y;
    rect->color.z = color.z;
    rect->color.w = color.w;
}

void draw_commit()
{
    glUseProgram(program);
    glBindVertexArray(vao);

    glUniform2f(loc_res,(float)view_width, (float)view_height);

    glUniform2f(loc_verts[0], -1.0, -1.0);
    glUniform2f(loc_verts[1], -1.0, +1.0);
    glUniform2f(loc_verts[2], +1.0, -1.0);
    glUniform2f(loc_verts[3], +1.0, +1.0);

    // buffer new rect data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, rect_count*sizeof(Rect), queued_rects, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rect_count); 
    rect_count = 0;

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    glBindVertexArray(0);
    glUseProgram(0);
}

