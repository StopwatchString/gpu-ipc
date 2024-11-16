#include "OpenGLApplication.h"

#include "cpputils/windows/dwm.h"

#include <iostream>
#include <exception>

//-----------------------------------------------
// static defaultKeyCallback()
//-----------------------------------------------
static void defaultKeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS) {
            glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
        }
        break;
    }
}

//-----------------------------------------------
// static defaultErrorCallback()
//-----------------------------------------------
static void defaultErrorCallback(int errorcode, const char* description)
{
    std::cerr << "GLFW Error! Code: " << errorcode << " Description: " << description << std::endl;
}

//-----------------------------------------------
// static defaultDrawFunc()
//-----------------------------------------------
static void defaultDrawFunc(GLFWwindow* window)
{
    auto scale = [](float oldLower, float oldUpper, float newLower, float newUpper, float input) {
        input -= oldLower;               // Move old lower bound to 0
        input /= (oldUpper - oldLower);  // Normalize value between 0 and 1
        input *= (newUpper - newLower);  // Scale normalized value to new bound size
        input += newLower;               // Move scale to new lower bound
        return input;
        };

    glfwMakeContextCurrent(window);
    while (!glfwWindowShouldClose(window)) {
        float r = 0.0f;
        float g = 0.0f;
        float b = scale(-1.0f, 1.0f, 0.2f, 0.8f, sinf((float)glfwGetTime()));
        float a = 0.0f;
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }
}

//-----------------------------------------------
// Constructor
//-----------------------------------------------
OpenGLApplication::OpenGLApplication(const ApplicationConfig& appConfig)
    : appConfig(appConfig)
{
    initGLFW();
    initExtensions();
    startRenderThread();
    eventLoop();
}

//-----------------------------------------------
// Destructor
//-----------------------------------------------
OpenGLApplication::~OpenGLApplication()
{
    renderThread.join();
}

//-----------------------------------------------
// initGLFW()
//-----------------------------------------------
void OpenGLApplication::initGLFW()
{
    if (!glfwInit()) {
        throw std::runtime_error("ERROR OpenGLApplication::initGLFW() glfwInit() failed!");
    }

    glfwWindowHint(GLFW_VERSION_MAJOR, appConfig.glVersionMajor);
    glfwWindowHint(GLFW_VERSION_MINOR, appConfig.glVersionMinor);
    glfwWindowHint(GLFW_RESIZABLE, appConfig.windowResizeEnable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, appConfig.windowBorderless ? GLFW_FALSE : GLFW_TRUE);

    glfwWindow = glfwCreateWindow(appConfig.windowInitWidth, appConfig.windowInitHeight, appConfig.windowName.c_str(), nullptr, nullptr);
    if (glfwWindow == nullptr) {
        throw std::runtime_error("ERROR OpenGLApplication::initGLFW() Could not create window!");
    }

    HWND hWnd = glfwGetWin32Window(glfwWindow);
    HRESULT hDarkModeResult = setWindowDarkMode(hWnd, appConfig.windowDarkmode);
    HRESULT hRoundedResult = setWindowRoundedCorners(hWnd, appConfig.windowRounded);

    GLFWerrorfun errorCallback = appConfig.customErrorCallback == nullptr ? defaultErrorCallback : appConfig.customErrorCallback;
    glfwSetErrorCallback(errorCallback);
    
    GLFWkeyfun keyCallback = appConfig.customKeyCallback == nullptr ? defaultKeyCallback : appConfig.customKeyCallback;
    glfwSetKeyCallback(glfwWindow, keyCallback);

    glfwSetWindowPos(glfwWindow, appConfig.windowPosX, appConfig.windowPosY);

    glfwMakeContextCurrent(glfwWindow);
}

//-----------------------------------------------
// initExtensions()
//-----------------------------------------------
void OpenGLApplication::initExtensions()
{
    if (!glhLoadExtensions(glfwGetProcAddress)) {
        throw std::runtime_error("ERROR OpenGLApplication::initExtensions() Failed to load platform-agnostic OpenGL extensions!");
    }

    HWND hWnd = glfwGetWin32Window(glfwWindow);
    if (hWnd == NULL) {
        throw std::runtime_error("ERROR OpenGLApplication::initExtensions() Failed to retreive native Win32 window handle from GLFW!");
    }

    HDC hDc = GetDC(hWnd);
    if (hDc == NULL) {
        throw std::runtime_error("ERROR OpenGLApplication::initExtensions() Failed to get Win32 Device Context via handle to window!");
    }

    if (!glhLoadPlatformExtensions(hDc, glfwGetProcAddress)) {
        throw std::runtime_error("ERROR OpenGLApplication::initExtensions() Failed to load platform-specific OpenGL extensions!");
    }
}

//-----------------------------------------------
// renderFunc()
//-----------------------------------------------
void OpenGLApplication::renderFunc() const
{
    try {
        std::function<void(GLFWwindow*)> drawFunction = appConfig.customDrawFunc == nullptr ? defaultDrawFunc : appConfig.customDrawFunc;
        drawFunction(glfwWindow);
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

//-----------------------------------------------
// startRenderThread()
//-----------------------------------------------
void OpenGLApplication::startRenderThread()
{
    // Release context from this thread so that the render thread can take control
    glfwMakeContextCurrent(nullptr);

    renderThread = std::thread(&OpenGLApplication::renderFunc, this);
}

//-----------------------------------------------
// eventLoop()
//-----------------------------------------------
void OpenGLApplication::eventLoop()
{
    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwWaitEvents();
    }
}