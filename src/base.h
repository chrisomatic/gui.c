#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#if PLATFORM == PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <profileapi.h>
#include <handleapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <pthread.h>
#else
#include <unistd.h> // for usleep
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#endif

#include <assert.h>
#include <float.h>

//:==================================
// Types
//:==================================

typedef uint8_t   U8;
typedef uint16_t  U16;
typedef uint32_t  U32;
typedef uint64_t  U64;
typedef int8_t    I8;
typedef int16_t   I16;
typedef int32_t   I32;
typedef int64_t   I64;
typedef float     F32;
typedef double    F64;
typedef int8_t    B8;
typedef int16_t   B16;
typedef int32_t   B32;
typedef int64_t   B64;

//:==================================
// Debugging
//:==================================

#define DEBUG()   printf("[DEBUG] %s %s(): %d\n", __FILE__, __func__, __LINE__)

//:==================================
// Math
//:==================================

#define PI 3.14159265358979323846
#define ABS(x)   ((x) < 0 ? -(x) : (x))
#define ABSF(x)  ((x) < 0.0 ? -(x) : (x))
#define MIN(x,y) ((x)  < (y) ? (x) : (y))
#define MAX(x,y) ((x) >= (y) ? (x) : (y))
#define CLAMP(x, lo, hi) MAX(MIN((x), (hi)),(lo))
#define SWAP(T, a, b) do { T temp = a; a = b; b = temp; } while (0)

F32 exp_decay(F32 a, F32 b, F32 decay, F32 dt)
{
    return b + (a - b)*exp(-decay*dt);
}

F32 lerp(F32 a, F32 b, F32 t)
{
    t = CLAMP(t,0.0,1.0);
    F32 r = (1.0-t)*a+(t*b);
    return r;
}

F32 exponential_smooth(F32 start, F32 end, F32 alpha, int frame)
{
    alpha = CLAMP(alpha, 0.0f, 1.0f);
    F32 factor = powf(1.0f - alpha, frame + 1);
    return end - (end - start) * factor;
}

typedef struct
{
    int x;
    int y;
} Point2i;

typedef struct
{
    U32 x,y,w,h;
} Rect;

//:==================================
// Memory
//:==================================

#define MemoryZero(p,z) memset((p), 0, (z))
#define MemoryZeroStruct(p) MemoryZero((p), sizeof(*(p)))

#define MemoryCopy(d,s,z) memmove((d), (s), (z))
#define MemoryCopyStruct(d,s) MemoryCopy((d), (s), MIN(sizeof(*(d)), sizeof(*(s))))

//:==================================
// Arenas
//:==================================

#define ARENA_SIZE_TINY        16*1024 //  16K
#define ARENA_SIZE_SMALL      128*1024 // 128K
#define ARENA_SIZE_MEDIUM  1*1024*1024 //   1M
#define ARENA_SIZE_LARGE  16*1024*1024 //  16M

#define ARENA_GROWTH_SIZE ARENA_SIZE_MEDIUM

typedef struct Arena
{
    U8* base;
    size_t capacity;
    size_t offset;
    struct Arena *next; // used for chaining arenas together
} Arena;

Arena *arena_create(size_t capacity)
{
    Arena *a = (Arena*)malloc(sizeof(Arena));
    if(!a) return NULL;

    a->base = (U8*)malloc(capacity * sizeof(U8));
    a->capacity = capacity;
    a->offset = 0;
    a->next = NULL;

    return a;
}

void arena_destroy(Arena* arena)
{
    if(!arena) return;

    Arena* a = arena;

    for(;;)
    {
        if(a->base) free(a->base);

        a->base = NULL;
        a->capacity = 0;
        a->offset = 0;

        if(a->next)
        {
            Arena* tmp = a;
            a = a->next;
            free(tmp);
            continue;
        }

        break;
    }

    arena = NULL;
}

void* arena_alloc(Arena* arena, size_t size)
{
    assert(arena);

    Arena* a = arena;

    for(;;)
    {
        if(a->offset + size <= a->capacity)
            break; // enough space, we're good


        // can't fit data on current arena
        // check for a next arena
        if(a->next)
        {
            a = a->next;
            continue;
        }

        // allocate a new arena that doubles the arena base capacity
        // or more to accommodate a large allocation
        
        size_t new_arena_size = (a->capacity >= size ? a->capacity : size);

        a->next = (Arena*)malloc(sizeof(Arena));
        a->next->base = (U8*)malloc(new_arena_size * sizeof(U8));
        a->next->offset = 0;
        a->next->capacity = new_arena_size;
    }

    void* ptr = a->base+a->offset;
    a->offset += size;

    return ptr;
}

void arena_reset(Arena* arena)
{
    assert(arena);

    Arena* a = arena;
    for(;;)
    {
        a->offset = 0;
        if(a->next)
        {
            a = a->next;   
            continue;
        }
        break;
    }
}

//:==================================
// Strings
//:==================================

#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))
#define STRN_EQUAL(x,y,n) (strncmp((x),(y),(n)) == 0)
#define BOOLSTR(b) ((b) ? "True" : "False")

#define S(literal) (String){sizeof(literal) - 1,(char*)(literal) }

typedef struct
{
    U64 len;
    char* data;
} String;

String StringFromCStr(char* cstr)
{
    return (String){ .len = (U32)strlen(cstr), .data = (char*)cstr };
}

B32 StringEndsWith(String str, String suffix) {
    if (suffix.len > str.len) return 0;
    return (strncmp(str.data + (str.len - suffix.len), suffix.data, suffix.len) == 0);
}

String StringFormat(Arena* arena, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int required_len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (required_len < 0)
    {
        return (String){ .len = 0, .data = NULL };
    }

    // Allocate from arena (+1 for null terminator)
    char* buffer = (char*)arena_alloc(arena, required_len + 1);
    if (!buffer)
    {
        return (String){ .len = 0, .data = NULL };
    }

    va_start(args, format);
    vsnprintf(buffer, required_len + 1, format, args);
    va_end(args);

    return (String){ .len = (U32)required_len, .data = buffer };
}

int StringGetExtension(const char *source, char *buf, int buf_len)
{
    if (!source || !buf) return 0;

    int len = strlen(source);
    if (len == 0) return 0;

    // Start from the end and move backward
    for (int i = len - 1; i >= 0; i--) {
        if (source[i] == '.')
        {
            // If '.' is the last character, no extension
            if (i == len - 1) return 0;

            // Copy extension
            int copy_len = MIN(len-i-1,buf_len-1);
            strncpy(buf, &source[i+1], copy_len);
            buf[copy_len] = '\0';
            return strlen(buf);
        }
    }

    return 0;
}

//:==================================
// Arrays
//:==================================

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

//:==================================
// Files
//:==================================

inline int FileWriteU8(FILE* file, U8 x)   { return fwrite(&x,sizeof(U8),1,file);}
inline int FileWriteU16(FILE* file, U16 x) { return fwrite(&x,sizeof(U16),1,file);}
inline int FileWriteU32(FILE* file, U32 x) { return fwrite(&x,sizeof(U32),1,file);}
inline int FileWriteF32(FILE* file, F32 x) { return fwrite(&x,sizeof(F32),1,file);}
inline int FileWriteStr(FILE* file, const char* s) { return fwrite(s,sizeof(char),strlen(s),file);}

inline int FileWriteU32AtIndex(FILE* file, U32 x, U32 index) {
    int pos = ftell(file);
    fseek(file, index, SEEK_SET);
    int ret = fwrite(&x,sizeof(U32),1,file);
    fseek(file, pos, SEEK_SET);
    return ret;
}

//:==================================
// Timer
//:==================================

typedef struct
{
    F32  fps;
    F32  spf;
    double time_start;
    double time_last;
    double frame_fps;
    double frame_fps_hist[60];
    double frame_fps_avg;
} Timer;

static struct
{
    bool monotonic;
    uint64_t  frequency;
    uint64_t  offset;
} _timer;

static double _fps_hist[60] = {0};
static int _fps_hist_count = 0;
static int _fps_hist_max_count = 0;

#if _WIN32
void usleep(__int64 usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

static uint64_t get_timer_value(void)
{
#if _WIN32
    uint64_t counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
    return counter;
#else
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    if (_timer.monotonic)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * (uint64_t)1000000000 + (uint64_t)ts.tv_nsec;
    }
    else
#endif

    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;

    }
#endif
}

void init_timer(void)
{
#if _WIN32
    uint64_t freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    _timer.monotonic = false;
    _timer.frequency = freq;
#else

#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        _timer.monotonic = true;
        _timer.frequency = 1000000000;
    }
    else
#endif
    {
        _timer.monotonic = false;
        _timer.frequency = 1000000;
    }
#endif
    _timer.offset = get_timer_value();

}

static double get_time()
{
    return (double) (get_timer_value() - _timer.offset) / (double)_timer.frequency;
}

void timer_begin(Timer* timer)
{
    timer->time_start = get_time();
    timer->time_last = timer->time_start;
    timer->frame_fps = 0.0f;
    timer->frame_fps_avg = 0.0f;
}

double timer_get_time()
{
    return get_time();
}

void timer_set_fps(Timer* timer, F32 fps)
{
    timer->fps = fps;
    timer->spf = 1.0f / fps;
}

void timer_wait_for_frame(Timer* timer)
{
    double now;
    for(;;)
    {
        now = get_time();
        if(now >= timer->time_last + timer->spf)
            break;
    }

    timer->frame_fps = 1.0f / (now - timer->time_last);
    timer->time_last = now;

    // calculate average FPS
    _fps_hist[_fps_hist_count++] = timer->frame_fps;

    if(_fps_hist_count >= 60)
        _fps_hist_count = 0;

    if(_fps_hist_max_count < 60)
        _fps_hist_max_count++;

    double fps_sum = 0.0;
    for(int i = 0; i < _fps_hist_max_count; ++i)
        fps_sum += _fps_hist[i];

    timer->frame_fps_avg = (fps_sum / _fps_hist_max_count);
}

double timer_get_elapsed(Timer* timer)
{
    double time_curr = get_time();
    return time_curr - timer->time_start;
}

double timer_get_prior_frame_fps(Timer* timer)
{
    return timer->frame_fps;
}

void timer_delay_us(int us)
{
    usleep(us);
}

// Logging

typedef enum
{
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
} LogLevel;

const char* log_level_strings[] = {
  "INFO", "WARN", "ERROR"
};

const char* log_level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

va_list ap;

void _log(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    time_t t = time(NULL);
    struct tm* _time = localtime(&t);

    char time_str[10] = {0};
    strftime(time_str, sizeof(time_str), "%H:%M:%S", _time);

    va_start(ap, fmt);
    fprintf(stdout, "%s %s%-5s\x1b[0m \x1b[90m%s:%-4d:\x1b[0m ", time_str, log_level_colors[level], log_level_strings[level], file, line);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}

#define logi(...) _log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__);
#define logw(...) _log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__);
#define loge(...) _log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__);

//
// Program-specific
//

typedef struct
{
    U8 r;
    U8 g;
    U8 b;
    U8 a;
} Color;

typedef struct
{
    bool success;
    int err_code;
} FunctionResult;

#define MAX_ARENAS 64

#if 0 //PLATFORM == PLATFORM_WINDOWS
extern HANDLE *threads;
#else
extern pthread_t *threads;
#endif
