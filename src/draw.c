
#define MAX_RECTS 1024

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

Rect queued_rects[MAX_RECTS] = {0};
int  rect_count = 0;

GLuint loc_res;
GLuint loc_verts[4];

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

