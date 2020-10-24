#include <iostream>
#include "camera.hpp"

Camera::Camera() noexcept
: width(1024), height(1024), angleY(0.0), angleX(0.0), mouseSensitivity(1.0), keyboardSpeed(10.0),
          cameraPos(vec3(0)), forward(vec3(0, 0, -1)), up(vec3(0, 1, 0)), right(vec3(1, 0, 0)) {}

Camera::Camera(int width, int height, float mouseSensitivity, float keyboardSpeed, vec3 pos, vec3 lookAt, vec3 up) noexcept
: width(width), height(height), mouseSensitivity(mouseSensitivity), cameraPos(pos), keyboardSpeed(keyboardSpeed)
{
    forward = glm::normalize(lookAt - pos);
    right   = glm::normalize(glm::cross(up, forward));
    up      = glm::cross(right, forward);
}

void Camera::setMouse(float x, float y)
{
    xpos = x;
    ypos = y;
}

void Camera::lookAt(bool zN, bool zP, bool xN, bool xP, bool halfSpeed, float deltaTime)
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

    if (halfSpeed) {
        keyboardSpeed = 5.0;
    }
    else
        keyboardSpeed = 10.0;

    if (zN) // W
        cameraPos += (keyboardSpeed * forward) * deltaTime;
    if (zP) // S
        cameraPos += (keyboardSpeed * (-forward)) * deltaTime;
    if (xN) // A
        cameraPos += (keyboardSpeed * (-right)) * deltaTime;
    if (xP) // D
        cameraPos += (keyboardSpeed * right) * deltaTime;
}