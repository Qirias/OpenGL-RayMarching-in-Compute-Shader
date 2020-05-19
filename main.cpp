#define GL_GLEXT_PROTOTYPES
#include "includes/EulerAngles.h"
#include "includes/camera.h"
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

bool zaxisPos   = false;
bool zaxisNeg   = false;
bool xaxisPos   = false;
bool xaxisNeg   = false;
float halfSpeed = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX     = SCREEN_WIDTH / 2.0;
float lastY     = SCREEN_HEIGHT / 2.0;
bool firstMouse = true;
MouseInput mouse;
Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, 0.025, 10.0, glm::vec4(0, 0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));

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
    const std::string &compShader = ParseCompute("includes/computeShader.glsl");
    unsigned int marching         = CreateCompute(compShader);

    double lastTime      = 0.0;
    unsigned int counter = 0;

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime          = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        processInput(window);

        useShader(marching);
        setFloat(marching, "iTime", (float)glfwGetTime());

        setVec4(marching, "camera.pos", camera.cameraPos.x, camera.cameraPos.y, camera.cameraPos.z, 0.0);
        setVec4(marching, "camera.dir", camera.forward.x, camera.forward.y, camera.forward.z, 0.0);
        setVec4(marching, "camera.yAxis", camera.up.x, camera.up.y, camera.up.z, 0.0);
        setVec4(marching, "camera.xAxis", camera.right.x, camera.right.y, camera.right.z, 0.0);
        setVec3(marching, "mouse", mouse.MouseLookAt());
        setVec2(marching, "iMouse", glm::vec2(mouse.getYaw(), mouse.getPitch()));

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
        zaxisNeg = true;
    else {
        zaxisNeg = false;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        zaxisPos = true;
    else {
        zaxisPos = false;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        xaxisPos = true;
    else {
        xaxisPos = false;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        xaxisNeg = true;
    else {
        xaxisNeg = false;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        halfSpeed = true;
    }
    else {
        halfSpeed = false;
    }

    camera.lookAt(zaxisNeg, zaxisPos, xaxisNeg, xaxisPos, halfSpeed, deltaTime);
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
    camera.setMouse(-xpos, -ypos);
}
// g++ main.cpp -lGL -lglfw && ./a.out