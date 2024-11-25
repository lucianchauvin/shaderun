#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

GLFWwindow* init_win(int width, int height) {
    assert(glfwInit());

    GLFWwindow* win;
    if (width == 0 || height == 0) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        win = glfwCreateWindow(mode->width, mode->height, "prog", glfwGetPrimaryMonitor(), NULL);
    } else
        win = glfwCreateWindow(width, height, "prog", NULL, NULL);

    assert(win);

    glfwMakeContextCurrent(win);
    assert(glewInit() == GLEW_OK);
    return win;
}

char* read_shader(const char* filepath) {
    const char* shader_preamble = 
        "#version 460 core\n"
        "out vec4 fragColor;\n"
        "uniform vec3 iResolution;\n"
        "uniform float iTime;\n"
        "uniform float iTimeDelta;\n"
        "uniform float iFrame;\n"
        "uniform float iChannelTime[4];\n"
        "uniform vec4 iMouse;\n"
        "uniform vec4 iDate;\n"
        "uniform float iSampleRate;\n"
        "uniform vec3 iChannelResolution[4];\n"
        "uniform sampler2D iChannel0;\n"
        "void mainImage(out vec4 fragColor, in vec2 fragCoord);\n"
        "void main() {\n"
        "    vec2 fragCoord = gl_FragCoord.xy;\n"
        "    mainImage(fragColor, fragCoord);\n"
        "}\n";

    FILE* file = fopen(filepath, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* shader_src = (char*)malloc(length + 1);
    shader_src[length] = '\0';
    fread(shader_src, 1, length, file);
    fclose(file);

    size_t preamble_len = strlen(shader_preamble);
    char* full_src = (char*)malloc(preamble_len + length + 1);
    strcpy(full_src, shader_preamble);
    strcat(full_src, shader_src);

    free(shader_src);
    return full_src;
}

GLuint compile_shader(const char* source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Shader Compilation Error: %s\n", infoLog);
        exit(EXIT_FAILURE);
    }

    return shader;
}

GLuint create_shader(const char* vertexSource, const char* fragmentSource) {
    GLuint vertex_shader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertex_shader);
    glAttachShader(prog, fragment_shader);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return prog;
}

void setup_quad() {
    float vertices[] = {
        -1.0f,  1.0f, 0.0f,  // top-left
        1.0f,  1.0f, 0.0f,  // top-right
        -1.0f, -1.0f, 0.0f,  // bottom-left
        1.0f, -1.0f, 0.0f,  // bottom-right
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void capture_frame(int width, int height, FILE* file) {
    size_t image_size = width * height * 3;  // RGB format
    unsigned char* pixels = (unsigned char*)malloc(image_size);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    fwrite(pixels, 1, image_size, file);

    free(pixels);
}

FILE* start_ffmpeg(int width, int height, const char* filename) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size %dx%d -r 60 -i - -c:v libx264 -pix_fmt yuv420p %s",
             width, height, filename);

    FILE* pipe = popen(cmd, "w");
    assert(pipe);
    return pipe;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fragment_path> [window_width] [window_height] [-o <output_file>]\n", argv[0]);
        return 1;
    }

    const char* output_file = NULL;
    const char* shader_path = argv[1];
    int w_width = 0;
    int w_height = 0;

    if (strcmp(argv[argc - 2], "-o") == 0)
        output_file = argv[argc - 1];

    GLFWwindow* win = init_win(w_width, w_height);

    FILE* ffmpeg_pipe = NULL;
    if (output_file) {
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
        ffmpeg_pipe = start_ffmpeg(width, height, output_file);
    }

    // load vertex source
    const char* vert_src = 
        "#version 460 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "}\n";

    char* frag_src = read_shader(argv[1]);

    GLuint prog = create_shader(vert_src, frag_src);
    free(frag_src);

    setup_quad();

    GLint i_time = glGetUniformLocation(prog, "iTime");
    GLint i_res = glGetUniformLocation(prog, "iResolution");
    GLint i_keyinput = glGetUniformLocation(prog, "iKeyInput");
    GLint i_dtime = glGetUniformLocation(prog, "iTimeDelta");
    GLint i_frame = glGetUniformLocation(prog, "iFrame");
    GLint i_mouse = glGetUniformLocation(prog, "iMouse");
    GLint i_chan_time[4], iDateLoc, iSampleRateLoc, iChannelResLoc[4];
    GLint i_chan0 = glGetUniformLocation(prog, "iChannel0");

    float prev_time = glfwGetTime();
    int f = 0;
    while (!glfwWindowShouldClose(win)) {

        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
        glViewport(0, 0, width, height); 

        glUseProgram(prog);

        // set uniform values
        float cur_time = glfwGetTime();
        float dtime = cur_time - prev_time;
        prev_time = cur_time;

        if (i_time != -1) glUniform1f(i_time, cur_time);
        if (i_res != -1) glUniform3f(i_res, (float)width, (float)height, 1.0f); 
        if (i_dtime != -1) glUniform1f(i_dtime, dtime);
        if (i_frame != -1) glUniform1f(i_frame, (float)f);
        if (i_chan_time[0] != -1) glUniform1f(i_chan_time[0], (float)f * 0.1f);

        // mouse
        float mouse_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        double mouse_x, mouse_y;
        glfwGetCursorPos(win, &mouse_x, &mouse_y);

        mouse_pos[0] = (float) mouse_x;
        mouse_pos[1] = height - (float) mouse_y;

        if (i_mouse != -1)
            glUniform4fv(i_mouse, 1, mouse_pos);  

        // texture stuff not really sure if this works
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);  // bind the appropriate texture
        if (i_chan0 != -1) glUniform1i(i_chan0, 0);  // texture unit 0

        glBindVertexArray(1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (ffmpeg_pipe)
            capture_frame(width, height, ffmpeg_pipe);

        glfwSwapBuffers(win);
        glfwPollEvents();
        f++;
    }

    if (ffmpeg_pipe)
        pclose(ffmpeg_pipe);

    glfwTerminate();
    return 0;
}
