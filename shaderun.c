#include "shaderun.h"

GLFWwindow *init_win(int width, int height) {
    assert(glfwInit());

    GLFWwindow *win;
    if (width == 0 || height == 0) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        win = glfwCreateWindow(mode->width, mode->height, "shaderun", glfwGetPrimaryMonitor(), NULL);
    } else
        win = glfwCreateWindow(width, height, "shaderun", NULL, NULL);

    assert(win);

    glfwMakeContextCurrent(win);
    assert(glewInit() == GLEW_OK);
    return win;
}

void setup_quad() {
    float vertices[] = {
        -1.0f,  1.0f, 0.0f,  // top-left
        1.0f ,  1.0f, 0.0f,  // top-right
        -1.0f, -1.0f, 0.0f,  // bottom-left
        1.0f , -1.0f, 0.0f,  // bottom-right
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

char *gen_frag_src(const char *path) {
    FILE *f = fopen(path, "r");

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *frag_src = (char*)malloc(len + 1);
    frag_src[len] = '\0';
    fread(frag_src, 1, len, f);
    fclose(f);

    char *full = (char*)malloc(strlen(frag_preamble) + len + 1);
    strcpy(full, frag_preamble);
    strcat(full, frag_src);

    free(frag_src);
    return full;
}

GLuint compile_shader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
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

GLuint create_prog(const char *vrtx_src, const char *frag_src) {
    GLuint vrtx = compile_shader(vrtx_src, GL_VERTEX_SHADER);
    GLuint frag = compile_shader(frag_src, GL_FRAGMENT_SHADER);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vrtx);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(vrtx);
    glDeleteShader(frag);

    return prog;
}

void load_prog(int signo) {
    if (signo == SIGUSR1) printf("recompiling %s\n", path);
    char *frag_src = gen_frag_src(path);
    glDeleteProgram(prog);
    prog = create_prog(vrtx_src, frag_src);
    free(frag_src);
    i_res      = glGetUniformLocation(prog, "iResolution");
    i_time     = glGetUniformLocation(prog, "iTime");
    i_dtime    = glGetUniformLocation(prog, "iTimeDelta");
    i_frame    = glGetUniformLocation(prog, "iFrame");
    i_mouse    = glGetUniformLocation(prog, "iMouse");
    i_keyinput = glGetUniformLocation(prog, "iKeyInput");

    f = 0;
    prev_time = 0.0;
    glfwSetTime(0.0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fragment_path> [window_width] [window_height]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // check frag path exists
    path = argv[1];
    if (access(path, F_OK)) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    int w_width, w_height;
    GLFWwindow *win = init_win(w_width, w_height);
    setup_quad();

    pid_t pid;
    if ((pid = fork()) == 0){
        int fd, wd;
        if ((fd = inotify_init()) < 0)
            perror("inotify_init");

        wd = inotify_add_watch(fd, path, IN_MODIFY);
        if (wd < 0)
            perror("inotify_add_watch");

        while (1) {
            if(read(fd, NULL, sizeof(struct inotify_event) != -1))
                kill(pid, SIGUSR1);
        }
    }

    // setup reocmpile handler
    struct sigaction sa;
    sa.sa_handler = load_prog;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        perror("sigaction");

    load_prog(0);
    while (!glfwWindowShouldClose(win)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetFramebufferSize(win, &w_width, &w_height);
        glViewport(0, 0, w_width, w_height); 

        glUseProgram(prog);

        // set uniform values
        cur_time = glfwGetTime();
        dtime = cur_time - prev_time;
        prev_time = cur_time;

        glUniform1f(i_time, (float)cur_time);
        glUniform3f(i_res, (float)w_width, (float)w_height, 1.0f); 
        glUniform1f(i_dtime, (float)dtime);
        glUniform1f(i_frame, (float)f);

        // TODO: implement clicking/down https://www.shadertoy.com/view/Mss3zH
        glfwGetCursorPos(win, &mouse_pos[0], &mouse_pos[1]);
        mouse_vec[0] = mouse_pos[0];
        mouse_vec[1] = w_height - mouse_pos[1];
        glUniform4fv(i_mouse, 1, mouse_vec);  

        // TODO: implement i_keyinput

        // not sure if this is working
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray(1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(win);
        glfwPollEvents();
        f++;
    }
    glfwTerminate();

    kill(pid, SIGKILL);
    wait(NULL);
    exit(EXIT_SUCCESS);
}
