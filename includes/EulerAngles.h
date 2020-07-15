#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

float MouseSensitivity = 0.001;

class MouseInput {
  public:
    float pitch, yaw;

    MouseInput() noexcept : pitch(0.0), yaw(0.0) {}

    MouseInput(const float newPitch, const float newYaw) noexcept : pitch(newPitch), yaw(newYaw) {}

    void ProcessMouseOffset(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // if (pitch > 89.0)
        //     pitch = 89.0;

        // if (pitch < -89.0)
        //     pitch = -89.0;
    }

    glm::vec3 MouseLookAt()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw) * cos(glm::radians(pitch)));

        return glm::vec3(glm::normalize(front));
    }
};
