#include "OpenGLApplication.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include "glh/glh.h"

#include "directx_utils.h"
#include "cpputils/windows/handle_utils.h"

#include <iostream>
#include <array>

OpenGLApplication::ApplicationConfig appConfig{};

constexpr int moveFactor = 100;
void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos - moveFactor, yPos);
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos + moveFactor, yPos);
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos - moveFactor);
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos + moveFactor);
    }
}

void draw(GLFWwindow* window) {
    glfwMakeContextCurrent(window);

    D3DInteropTexture2D tex(100, 100, false);

    Framebuffer framebuf;
    framebuf.bind();

    tex.interopLock();
    framebuf.setColorAttachment2D(GL_TEXTURE_2D, tex.handle());
    tex.interopUnlock();

    if (!framebuf.isComplete()) {
        std::cout << "panic" << std::endl;
    }

    glEnable(GL_TEXTURE_2D);

    while (!glfwWindowShouldClose(window)) {
        // Render to the framebuffer/texture
        tex.interopLock();
        framebuf.bind();

        float sinVal = sinf((float)glfwGetTime()) * 0.5f + 0.5f;
        glhClearColor(sinVal, 0.0f, sinVal, 1.0f);
        glhClear(GL_COLOR_BUFFER_BIT);

        // Render to the screen framebuffer, drawing a triangle that uses the texture from the framebuffer
        framebuf.unbind();
        
        //glhBindFramebuffer(GL_FRAMEBUFFER, 0);
        glhClearColor(1.0, 0.0, 0.0, 1.0);
        glhClear(GL_COLOR_BUFFER_BIT);

        tex.bind();
        glhBegin(GL_TRIANGLES);

        glhTexCoord2f(0.0f, 0.0f);
        glhVertex2f(-1.0f, -1.0f);

        glhTexCoord2f(0.0f, 1.0f);
        glhVertex2f(0.0f, 1.0f);

        glhTexCoord2f(1.0f, 0.0f);
        glhVertex2f(1.0f, 0.0f);

        glhEnd();

        tex.interopUnlock();

        glhErrorCheck("End of Render");
        glfwSwapBuffers(window);
    }
}

int main(int argc, char argv[])
{
    appConfig.windowName = "Producer";
    appConfig.windowInitWidth = 1000;
    appConfig.windowInitHeight = 1000;
    appConfig.windowPosX = 4000;
    appConfig.windowPosY = 100;
    appConfig.windowBorderless = false;
    appConfig.windowResizeEnable = false;
    appConfig.windowDarkmode = true;
    appConfig.windowRounded = true;
    appConfig.glVersionMajor = 4;
    appConfig.glVersionMinor = 6;
    appConfig.customDrawFunc = &draw;
    appConfig.customKeyCallback = &keyCallback;

    try {
        OpenGLApplication application(appConfig);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}