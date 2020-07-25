#pragma once
#include "../includes/glm/glm.hpp"
#include "../includes/glm/gtc/matrix_transform.hpp"
#include "../includes/glm/gtc/type_ptr.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

float MouseSensitivity = 0.001;

class MouseInput {
  public:
    float pitch, yaw;

    MouseInput() noexcept;

    MouseInput(const float newPitch, const float newYaw) noexcept;

    void ProcessMouseOffset(float xoffset, float yoffset);

    glm::vec3 EulerAngles(); //You can use this for color input, it's fun I guess
};