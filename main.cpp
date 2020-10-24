#define GL_GLEXT_PROTOTYPES
#include "includes/glm/glm.hpp"
#include "includes/glm/gtc/matrix_transform.hpp"
#include "includes/glm/gtc/type_ptr.hpp"

#include "source/shader.hpp"
#include "source/camera.cpp"
#include "source/MousePosition.cpp"
#include "source/quad.cpp"
#include "source/texture.cpp"
#include "source/WorkGroups.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

const unsigned int SCREEN_WIDTH  = 1080;
const unsigned int SCREEN_HEIGHT = 1080;

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool zaxisPos   = false;
bool zaxisNeg   = false;
bool xaxisPos   = false;
bool xaxisNeg   = false;
bool AA         = true;
bool showQuad   = false;
float halfSpeed = false;
int bounce = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX     = SCREEN_WIDTH / 2.0;
float lastY     = SCREEN_HEIGHT / 2.0;
bool firstMouse = true;

MouseInput mouse;
Camera camera(SCREEN_WIDTH, SCREEN_HEIGHT, 0.025, 10.0, glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));

int main(void)
{
    GLFWwindow *window;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ray Marching", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);

    // Create quad
    FullScreenQuad quad;
    quad.GenBuffer();
    

    // create texture for compute shader
    Texture tex(SCREEN_WIDTH, SCREEN_HEIGHT);
    tex.GenerateTexture();

    
    // printWorkGroupCount();
    // printWorkGroupSize();
    unsigned int invocations = printInvocations();
    unsigned int workgroups = sqrt(invocations);
    std::cout << "Maximum Workgroups:\t" << workgroups << std::endl;

    // Quad's Fragment and Vertex shader
    ShaderProgramSource basic = ParseShader("shaders/Quad.glsl");
    unsigned int quadShader   = CreateShader(basic.VertexShader, basic.FragmentShader);

    // Compute Shader
    const std::string &compShader = ParseCompute("shaders/computeShader.glsl");
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
        setuInt(marching, "workgroups", &workgroups);

        setVec4(marching, "camera.pos", camera.cameraPos.x, camera.cameraPos.y, camera.cameraPos.z, 0.0);
        setVec4(marching, "camera.dir", camera.forward.x, camera.forward.y, camera.forward.z, 0.0);
        setVec4(marching, "camera.yAxis", camera.up.x, camera.up.y, camera.up.z, 0.0);
        setVec4(marching, "camera.xAxis", camera.right.x, camera.right.y, camera.right.z, 0.0);

        setVec3(marching, "light.position", glm::vec3(-5, 5, -10));
        setVec3(marching, "light.ambient", glm::vec3(0.03, 0.04, 0.1));
        setVec3(marching, "light.diffuse", glm::vec3(0.8, 0.8, 0.8));
        setVec3(marching, "light.specular", glm::vec3(0.5, 0.5, 0.5));
        setFloat(marching, "light.constant", 1.0);
        setFloat(marching, "light.linear", 0.009);
        setFloat(marching, "light.quadratic", 0.00032);

        setBool(marching, "AA", AA);
        setInt(marching, "bounceVar", bounce);
        setFloat(marching, "drand48", drand48());
        setVec3(marching, "mouse", mouse.EulerAngles());
        setVec2(marching, "iMouse", glm::vec2(mouse.yaw, mouse.pitch));

        // Number of work groups in dispach: X, Y, Z
        glDispatchCompute(SCREEN_WIDTH / workgroups, SCREEN_HEIGHT / workgroups, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        useShader(quadShader);

        glBindVertexArray(quad.quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex.texOutput);
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
    quad.DeleteVertex();
    quad.DeleteBuffer();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        if (bounce < 5)
            bounce += 1;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        if (bounce > 0)
            bounce -= 1;

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
        AA = !AA;

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        if (showQuad)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        showQuad = !showQuad;
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
    camera.setMouse(-xpos, -ypos);
}
// g++ main.cpp -lGL -lglfw && ./a.out