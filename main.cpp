#define GL_GLEXT_PROTOTYPES
#include "includes/EulerAngles.h"
#include "includes/glm/glm.hpp"
#include "includes/glm/gtc/matrix_transform.hpp"
#include "includes/glm/gtc/type_ptr.hpp"
#include "includes/shader.h"
#include <GLFW/glfw3.h>
#include <iomanip>
#include <iostream>

const unsigned int SCREEN_WIDTH  = 1024;
const unsigned int SCREEN_HEIGHT = 1024;
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

float xaxis = 0.0, yaxis = 0.0, zaxis = -1.0;
float lastX     = SCREEN_WIDTH / 2.0;
float lastY     = SCREEN_HEIGHT / 2.0;
bool firstMouse = true;
MouseInput mouse;

int main(void)
{
    GLFWwindow *window;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ray Marching", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // clang-format off
    float quadVertices[] = {
        //positions   texture Coords
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    // clang-format on

    // screen quad
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // create texture for compute shader
    int tex_w = SCREEN_WIDTH, tex_h = SCREEN_HEIGHT;
    unsigned int tex_output;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int work_grp_cnt[ 3 ];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[ 0 ]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[ 1 ]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[ 2 ]);
    // printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[ 0 ], work_grp_cnt[ 1 ],
    //        work_grp_cnt[ 2 ]);

    int work_grp_size[ 3 ];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[ 0 ]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[ 1 ]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[ 2 ]);
    // printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[ 0 ], work_grp_size[ 1 ],
    //        work_grp_size[ 2 ]);

    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    // printf("max local work group invocations %i\n", work_grp_inv);

    // Quad's Fragment and Vertex shader
    ShaderProgramSource basic = ParseShader("includes/Basic.glsl");
    unsigned int quad         = CreateShader(basic.VertexShader, basic.FragmentShader);

    // Compute Shader
    const std::string &compShader = ParseCompute("includes/marchinghader.glsl");
    unsigned int marching         = CreateCompute(compShader);

    double lastTime      = 0.0;
    unsigned int counter = 0;

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glm::mat4 projection = glm::perspective(60.0, (double)SCREEN_WIDTH / (double)SCREEN_HEIGHT, 1.0, 2.0);
        glm::vec3 camPos     = glm::vec3(3.0, 2.0, 7.0);
        glm::mat4 view       = glm::lookAt(camPos, glm::vec3(0.0, 0.5, 0.0), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 inv        = glm::transpose(glm::inverse(projection * view));

        glm::vec4 ray00 = glm::vec4(-1, -1, 0, 1) * inv;
        ray00 /= ray00.w;
        ray00 -= glm::vec4(camPos, 1.0);

        glm::vec4 ray10 = glm::vec4(1, -1, 0, 1) * inv;
        ray10 /= ray10.w;
        ray10 -= glm::vec4(camPos, 1.0);

        glm::vec4 ray01 = glm::vec4(-1, 1, 0, 1) * inv;
        ray01 /= ray01.w;
        ray01 -= glm::vec4(camPos, 1.0);

        glm::vec4 ray11 = glm::vec4(1, 1, 0, 1) * inv;
        ray11 /= ray11.w;
        ray11 -= glm::vec4(camPos, 1.0);

        useShader(marching);
        setFloat(marching, "iTime", (float)glfwGetTime());
        setFloat(marching, "zaxis", zaxis);
        setFloat(marching, "xaxis", xaxis);
        setFloat(marching, "yaxis", yaxis);
        setVec3(marching, "mouse", mouse.MouseLookAt());
        setVec2(marching, "iMouse", glm::vec2(mouse.getYaw(), mouse.getPitch()));

        unsigned int ray00Id = glGetUniformLocation(marching, "ray00");
        glUniform3f(ray00Id, ray00.x, ray00.y, ray00.z);

        unsigned int ray01Id = glGetUniformLocation(marching, "ray01");
        glUniform3f(ray01Id, ray01.x, ray01.y, ray01.z);

        unsigned int ray10Id = glGetUniformLocation(marching, "ray10");
        glUniform3f(ray10Id, ray10.x, ray10.y, ray10.z);

        unsigned int ray11Id = glGetUniformLocation(marching, "ray11");
        glUniform3f(ray11Id, ray11.x, ray11.y, ray11.z);

        unsigned int camId = glGetUniformLocation(marching, "eye");
        glUniform3f(camId, camPos.x, camPos.y, camPos.z);

        // Number of work groups in dispach: X, Y, Z
        glDispatchCompute(SCREEN_WIDTH / 32, SCREEN_HEIGHT / 32, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        useShader(quad);

        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_output);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ++counter;
        double currentTime = glfwGetTime();

        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS:\t" << counter << std::endl;
            ++lastTime;
            counter = 0;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Window close
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // Lines of Triangles
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        zaxis -= 0.1;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        zaxis += 0.1;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        xaxis -= 0.1;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        xaxis += 0.1;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        yaxis += 0.1;

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        yaxis -= 0.1;
        if (yaxis <= 0.0)
            yaxis = 0.0;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX      = xpos;
        lastY      = ypos;
        firstMouse = false;
    }

    float xoffset = lastX - xpos;
    float yoffset = lastY - ypos;
    lastX         = xpos;
    lastY         = ypos;

    mouse.ProcessMouseOffset(xoffset, yoffset);
}
// g++ main.cpp -lGL -lglfw && ./a.out