
// System libs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// For timing
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <profileapi.h>
#else
#include <sys/time.h>
#endif

// Open GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Image loading
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

// Local libs
#include "util.c"
#include "draw.c"
#include "window.c"
#include "shader.c"
#include "timer.c"

// =========================
// Global Vars
// =========================

bool paused = false;
Timer main_timer = {0};

// =========================
// Function Prototypes
// =========================

void start_gui();
void init();
void deinit();
//void simulate(double);
void draw();

// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    logi("--------------");
    logi("Starting GUI");
    logi("--------------");

    time_t t;
    srand((unsigned) time(&t));

    init();

    timer_set_fps(&main_timer,TARGET_FPS);
    timer_begin(&main_timer);

    double curr_time = timer_get_time();
    double new_time  = 0.0;
    double accum = 0.0;

    const double dt = 1.0/TARGET_FPS;

    // main game loop
    for(;;)
    {
        new_time = timer_get_time();
        double frame_time = new_time - curr_time;
        curr_time = new_time;

        accum += frame_time;

        window_poll_events();
        if(window_should_close())
            break;

        while(accum >= dt)
        {
            //simulate(dt);
            accum -= dt;
        }

        draw();

        timer_wait_for_frame(&main_timer);
        window_swap_buffers();
        window_mouse_update_actions();
    }

    deinit();
    return 0;
}

void init()
{
    init_timer();

    logi("resolution: %d %d",VIEW_WIDTH, VIEW_HEIGHT);
    bool success = window_init(VIEW_WIDTH, VIEW_HEIGHT, false);

    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    logi("Initializing...");

    logi(" - Shaders.");
    shader_load_all();

    logi(" Init Complete.");

}

void deinit()
{
    shader_deinit();
    window_deinit();
}

void draw()
{
}
