#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glh/glh.h"

#include "OpenGLApplication.h"

#include <iostream>
#include <array>

OpenGLApplication::ApplicationConfig appConfig{};

constexpr int moveFactor = 100;
float size = 20.0f;
void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {

    bool pressed = action == GLFW_PRESS;
    bool pressedOrHeld = action == GLFW_PRESS || action == GLFW_REPEAT;
    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool shift = mods & GLFW_MOD_SHIFT;
    bool alt = mods & GLFW_MOD_ALT;
    bool noMod = !ctrl && !shift && !alt;


    if (key == GLFW_KEY_ESCAPE && pressed) {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    if (key == GLFW_KEY_LEFT && pressedOrHeld && shift) {
        size -= 2.0f;
        if (size < 1.0f) size = 2.0f;
    }

    if (key == GLFW_KEY_RIGHT && pressedOrHeld && shift) {
        size += 2.0f;
    }

    if (key == GLFW_KEY_LEFT && pressedOrHeld && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos - moveFactor, yPos);
    }

    if (key == GLFW_KEY_RIGHT && pressedOrHeld && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos + moveFactor, yPos);
    }

    if (key == GLFW_KEY_UP && pressedOrHeld && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos - moveFactor);
    }

    if (key == GLFW_KEY_DOWN && pressedOrHeld && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos + moveFactor);
    }
}

void draw(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    D3DInteropTexture2D::initDirect3D();

    D3DInteropTexture2D tex(100, 100, false);

    Framebuffer framebuf;
    framebuf.bind();

    tex.interopLock();
    framebuf.setColorAttachment2D(GL_TEXTURE_2D, tex.handle());
    tex.interopUnlock();

    glhInitFont(appConfig.windowInitWidth, appConfig.windowInitHeight);

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
        glhClearColor(.2, 0.3, 0.4, 1.0);
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

        glhDrawText(std::to_string(glfwGetTime()), 0, 0, size);
        
        std::string moveText = "Ctrl+Arrow Keys to move window";
        FontRect moveTextRect = glhGetTextSize(moveText, size);
        glhDrawText(moveText, appConfig.windowInitWidth - moveTextRect.width, 0, size);

        std::string scaleText = "Shift+Arrow Keys to scale text";
        FontRect scaleTextRect = glhGetTextSize(scaleText, size);
        glhDrawText(scaleText, appConfig.windowInitWidth - scaleTextRect.width, moveTextRect.height, size);

        glhErrorCheck("End of Render");
        glfwSwapBuffers(window);
    }
    glhFreeFont();
    D3DInteropTexture2D::shutdownDirect3D();
}

int main(int argc, char argv[])
{
    appConfig.windowName = "Producer";
    appConfig.windowInitWidth = 1000;
    appConfig.windowInitHeight = 1000;
    appConfig.windowPosX = 200;
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