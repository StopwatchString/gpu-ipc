#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glh/glh.h"

#include "glh/classes/OpenGLApplication.h"
#include "cpputils/SharedMemory.h"
#include "cpputils/SharedLibraryLoader.h"
#include "cpputils/windows/handle_utils.h"

#include <D3D11_1.h>
#include <wrl/client.h> // for Microsoft::WRL::ComPtr template

#include <iostream>
#include <chrono>
#include <array>
#include <numeric>

OpenGLApplication::ApplicationConfig appConfig{};
constexpr int MOVE_FACTOR = 100;
constexpr int FRAMETIME_ROLLING_AVG_SAMPLE_SIZE = 100;

float size = 20.0f;

class KeyState {
public:
    KeyState(int inKey) : key(inKey) {}
    void update(int inKey, int inAction) {
        if (key == inKey) {
            if (inAction == GLFW_PRESS) {
                active = true;
            }
            else if (inAction == GLFW_RELEASE) {
                active = false;
            }
        }
    }
    bool isActive() { return active; }

private:
    int key;
    bool active;
};

bool doRotation = true;
void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {

    static KeyState left(GLFW_KEY_LEFT);
    static KeyState right(GLFW_KEY_RIGHT);
    static KeyState up(GLFW_KEY_UP);
    static KeyState down(GLFW_KEY_DOWN);

    left.update(key, action);
    right.update(key, action);
    up.update(key, action);
    down.update(key, action);

    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool shift = mods & GLFW_MOD_SHIFT;
    bool alt = mods & GLFW_MOD_ALT;
    bool noMod = !ctrl && !shift && !alt;


    if (key == GLFW_KEY_ESCAPE && (action == GLFW_PRESS)) {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS)) {
        doRotation = !doRotation;
    }

    if (left.isActive() && shift) {
        size -= 2.0f;
        if (size < 1.0f) size = 2.0f;
    }

    if (right.isActive() && shift) {
        size += 2.0f;
    }

    if (left.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos - MOVE_FACTOR, yPos);
    }

    if (right.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos + MOVE_FACTOR, yPos);
    }

    if (up.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos - MOVE_FACTOR);
    }

    if (down.isActive() && ctrl) {
        int xPos, yPos;
        glfwGetWindowPos(glfwWindow, &xPos, &yPos);
        glfwSetWindowPos(glfwWindow, xPos, yPos + MOVE_FACTOR);
    }
}

std::string doubleToString(double val, size_t precision) {
    std::string str = std::to_string(val);
    int dec = str.find('.');
    return str.substr(0, dec + precision + 1);
}

struct d3dshare {
    DWORD sourceProcessId{ NULL };
    HANDLE sourceProcessShareHandle{ NULL };
    bool ready{ false };
};

Microsoft::WRL::ComPtr<ID3D11Device1> d3dDevice1 = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext1> d3dDeviceContext1 = nullptr;
HANDLE hWglD3DDevice = NULL;

void draw(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    glhInitFont(appConfig.windowInitWidth, appConfig.windowInitHeight);

    d3dshare share;
    while (!share.ready) {
        cpputils::SharedMemory d3dshare("d3dshare", sizeof(d3dshare));
        memcpy(&share, d3dshare.data(), sizeof(d3dshare));
        if (!share.ready) {
            std::cout << "Waiting for data to be populated..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    HANDLE localShareHandle = cpputils::windows::duplicateHandle(share.sourceProcessId, share.sourceProcessShareHandle);
    std::cout << "Found sourceProcessId: " << share.sourceProcessId << std::endl;
    std::cout << "Found sourceProcessShareHandle: " << share.sourceProcessShareHandle << std::endl;
    std::cout << "Obtained localShareHandle: " << localShareHandle << std::endl;

    Direct3DContext d3dContext = createDirect3DContext();

    ID3D11Texture2D* importedTexture = nullptr;
    HRESULT hr = d3dContext.d3dDevice1->OpenSharedResource1(
        localShareHandle,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&importedTexture)
    );
    if (hr != S_OK) {
        std::cout << "Failed to open imported texture" << std::endl;
    }

    // Associate the D3DTexture with the generated resource handle
    if (!wglDXSetResourceShareHandleNV(importedTexture, localShareHandle)) {
        std::cerr << "ERROR createSharedTexture() wglDXSetResourceShareHandleNV() failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Make the OpenGL Texture
    GLuint openglTex;
    glGenTextures(1, &openglTex);

    // This lock synchronizes control of the texture between D3D and OpenGL
    // You *must* lock and unlock this resource when using the texture in OpenGL for any operation.
    // TBD for DirectX operations.
    HANDLE textureLock = wglDXRegisterObjectNV(
        hWglD3DDevice,            // hDevice  | 
        importedTexture,             // dxObject | 
        openglTex,      // name     | 
        GL_TEXTURE_2D,            // type     | 
        WGL_ACCESS_READ_WRITE_NV  // access   | 
    );
    if (textureLock == NULL) {
        std::cerr << "wglDXRegisterObjectNV failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    glhTextureParameteri(openglTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glhTextureParameteri(openglTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);
    size_t writeIndex = 0;
    std::array<double, FRAMETIME_ROLLING_AVG_SAMPLE_SIZE> frametimeMsTimings;

    while (!glfwWindowShouldClose(window)) {
        auto begin = std::chrono::high_resolution_clock::now();

        glhClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glhClear(GL_COLOR_BUFFER_BIT);

        wglDXLockObjectsNV(hWglD3DDevice, 1, &textureLock);
        glhBindTexture(GL_TEXTURE_2D, openglTex);

        glPushMatrix();

        if (doRotation) {
            glRotatef(glfwGetTime(), 0.0f, 0.0f, 1.0f);
        }

        glhBegin(GL_POLYGON);

        glhTexCoord2f(0.0f, 0.0f);
        glhVertex2f(-1.0f, -1.0f);

        glhTexCoord2f(0.0f, 1.0f);
        glhVertex2f(-1.0f, 1.0f);

        glhTexCoord2f(1.0f, 1.0f);
        glhVertex2f(1.0f, 1.0f);

        glhTexCoord2f(1.0f, 0.0f);
        glhVertex2f(1.0f, -1.0f);

        glhEnd();

        glPopMatrix();

        wglDXUnlockObjectsNV(hWglD3DDevice, 1, &textureLock);

        std::string controlStr = "Press space to toggle rotation";
        FontRect fontrect = glhGetTextSize(controlStr, 20.0f);
        glhSetTextColor(1.0f, 0.0f, 0.0f, 1.0f);
        glhDrawText(controlStr, 0, appConfig.windowInitHeight - fontrect.height, 20.0f);

        //std::string timeStr = doubleToString(glfwGetTime(), 2) + "s";
        //FontRect timeRect = glhDrawText(timeStr, 0, 0, size);

        //double total = std::accumulate(std::begin(frametimeMsTimings), std::end(frametimeMsTimings), 0);
        //double frametimeMsAvg = total / frametimeMsTimings.size();
        //std::string frametimeStr = doubleToString(frametimeMsAvg, 2) + " frametime (ms)";
        //glhDrawText(frametimeStr, 0, timeRect.height, size);
        //
        //std::string moveText = "Ctrl+Arrow Keys to move window";
        //FontRect moveTextRect = glhGetTextSize(moveText, size);
        //glhDrawText(moveText, appConfig.windowInitWidth - moveTextRect.width, 0, size);

        //std::string scaleText = "Shift+Arrow Keys to scale text";
        //FontRect scaleTextRect = glhGetTextSize(scaleText, size);
        //glhDrawText(scaleText, appConfig.windowInitWidth - scaleTextRect.width, moveTextRect.height, size);

        glhErrorCheck("End of Render");

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        frametimeMsTimings[writeIndex] = ms;
        writeIndex++;
        writeIndex %= FRAMETIME_ROLLING_AVG_SAMPLE_SIZE;

        glfwSwapBuffers(window);
    }
    glhFreeFont();
}

int main(int argc, char argv[])
{
    appConfig.windowName = "Consumer";
    appConfig.windowInitWidth = 1000;
    appConfig.windowInitHeight = 1000;
    appConfig.windowPosX = 200;
    appConfig.windowPosY = 100;
    appConfig.windowBorderless = false;
    appConfig.windowResizeEnable = false;
    appConfig.windowDarkmode = true;
    appConfig.windowRounded = true;
    appConfig.windowAlwaysOnTop = false;
    appConfig.vsyncEnable = true;
    appConfig.transparentFramebuffer = false;
    appConfig.glVersionMajor = 4;
    appConfig.glVersionMinor = 6;
    appConfig.dearImguiGlslVersionString = "#version 460"; // Used for DearImgui, leave default unless you know what to put here
    appConfig.imguiIniFileName = nullptr;
    appConfig.customDrawFunc = draw;           // std::function<void(GLFWwindow*)>
    appConfig.customKeyCallback = keyCallback; // std::function<void(GLFWwindow* window, int key, int scancode, int action, int mods)>
    appConfig.customErrorCallback = nullptr;   // std::function<void(int error_code, const char* description)>
    appConfig.customDropCallback = nullptr;    // std::function<void(GLFWwindow* window, int count, const char** paths)>
    appConfig.customPollingFunc = nullptr;     // std::function<void()>

    try {
        OpenGLApplication application(appConfig);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}