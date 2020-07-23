#pragma once
#include "../includes/glm/glm.hpp"
#include "../includes/glm/gtc/matrix_transform.hpp"
#include "../includes/glm/gtc/type_ptr.hpp"
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

    vec3 cameraPos;
    vec3 forward;
    vec3 up;
    vec3 right;

    Camera() noexcept;

    Camera(int width, int height, float mouseSensitivity, float keyboardSpeed, vec3 pos, vec3 lookAt, vec3 up) noexcept;

    void setMouse(float x, float y);

    void lookAt(bool zN, bool zP, bool xN, bool xP, bool halfSpeed, float deltaTime);
};