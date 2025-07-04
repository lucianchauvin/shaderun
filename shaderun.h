#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/inotify.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024*(EVENT_SIZE + 16))

const char *path;
GLuint prog;
GLint i_res, i_time, i_dtime, i_frame, i_mouse, i_keyinput;
int f;
double prev_time, cur_time, dtime, mouse_pos[2];
float mouse_vec[4];

const char *vrtx_src = 
    "#version 460 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

const char *frag_preamble = 
    "#version 460 core\n"
    "out vec4 fragColor;\n"
    "uniform vec3 iResolution;\n"
    "uniform float iTime;\n"
    "uniform float iTimeDelta;\n"
    "uniform float iFrame;\n"
    // "uniform float iChannelTime[4];\n"
    "uniform vec4 iMouse;\n"
    // "uniform vec4 iDate;\n"
    // "uniform float iSampleRate;\n"
    // "uniform vec3 iChannelResolution[4];\n"
    // "uniform sampler2D iChannel0;\n"
    "void mainImage(out vec4 fragColor, in vec2 fragCoord);\n"
    "void main() {\n"
    "    vec2 fragCoord = gl_FragCoord.xy;\n"
    "    mainImage(fragColor, fragCoord);\n"
    "}\n";

GLFWwindow *init_win(int width, int height);
void setup_quad();
char *read_shader(const char *path);
GLuint compile_shader(const char *source, GLenum shader_type);
GLuint create_prog(const char *vrtx_src, const char *frag_src);
void load_prog(int signo);
