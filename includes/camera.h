#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

class Camera {
  public:
    int width;
    int height;
    float angleY;
    float angleX;
    float mouseSensitivity;
    float keyboardSpeed;
    float xpos;
    float ypos;
    bool firstEntry = true;

    vec3 cameraPos;
    vec3 forward;
    vec3 up;
    vec3 right;

    Camera()
        : width(1024), height(1024), angleY(0.0), angleX(0.0), mouseSensitivity(1.0), keyboardSpeed(10.0),
          cameraPos(vec3(0)), forward(vec3(0, 0, -1)), up(vec3(0, 1, 0)), right(vec3(1, 0, 0))
    {
    }

    Camera(int width, int height, float mouseSensitivity, float keyboardSpeed, vec3 pos, vec3 lookAt, vec3 up)
        : width(width), height(height), mouseSensitivity(mouseSensitivity), cameraPos(pos), keyboardSpeed(keyboardSpeed)
    {
        forward = glm::normalize(lookAt - pos);
        right   = glm::normalize(glm::cross(up, forward));
        up      = glm::cross(right, forward);
    }

    void setMouse(float x, float y)
    {
        xpos = x;
        ypos = y;
    }

    void lookAt(bool zN, bool zP, bool xN, bool xP, bool halfSpeed, float deltaTime)
    {

        angleX = xpos * mouseSensitivity;
        angleY = ypos * mouseSensitivity;

        mat4 rotateX = glm::rotate(mat4(1.0), glm::radians(angleY), vec3(1.0, 0.0, 0.0));
        mat4 rotateY = glm::rotate(mat4(1.0), glm::radians(angleX), vec3(0.0, 1.0, 0.0));

        mat4 rotationMatrix = rotateY * rotateX;

        // convert the mat4 to vec4 and get the the xyz
        forward = glm::normalize(vec3(rotationMatrix * vec4(0.0, 0.0, -1.0, 0.0)));
        up      = glm::normalize(vec3(rotationMatrix * vec4(0.0, 1.0, 0.0, 0.0)));
        right   = glm::normalize(glm::cross(forward, up));

        if (halfSpeed && firstEntry) {
            keyboardSpeed /= 2.0;
            firstEntry = false;
        }

        if (zN) // W
            cameraPos += (keyboardSpeed * (forward + right)) * deltaTime;
        if (zP) // S
            cameraPos += (keyboardSpeed * ((-forward) + right)) * deltaTime;
        if (xN) // A
            cameraPos += (keyboardSpeed * (vec3(0) * forward + (-right))) * deltaTime;
        if (xP) // D
            cameraPos += (keyboardSpeed * (vec3(0) * forward + right)) * deltaTime;

        if (zP && xP) // S D
            cameraPos += (keyboardSpeed * ((-forward) + right)) * deltaTime;
        if (zN && xN) // W A
            cameraPos += ((keyboardSpeed) * (forward + (-right))) * deltaTime;
        if (xN && zP) // A S
            cameraPos += ((keyboardSpeed) * ((-forward) + (-right))) * deltaTime;
        if (xN && xP) // A D
            cameraPos = cameraPos * vec3(0.0);
    }
};