#define UNICODE

#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <Windows.h>
#include <stdio.h>
#include <thread>

using namespace std::chrono_literals;

#define GLES_CALL(func) \
        func; \
        checkGLError(#func, __FILE__, __LINE__);

void checkGLError(const char* funcName, const char* fileName, int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error %d in %s at line %d: %s\n", error, fileName, line, funcName);
        exit(1);
    }
}

const char* vertexShaderCode = R"( #version 310 es
    layout (location = 0) in vec3 aPosition;

    void main() {
        gl_Position = vec4(aPosition, 1.0);
    }
)";

const char* fragmentShaderCode = R"( #version 310 es
    precision mediump float;
    out vec4 fragColor;

    void main() {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
    }
)";

GLuint createShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    GLES_CALL(glShaderSource(shader, 1, &source, nullptr));
    GLES_CALL(glCompileShader(shader));

    GLint compileStatus;
    GLES_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
    if (compileStatus != GL_TRUE) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        printf("Shader compilation failed: %s\n", infoLog);
    }

    return shader;
}

void renderFrame() {
    // Clear the color buffer
    GLES_CALL(glClear(GL_COLOR_BUFFER_BIT));
    glClearColor(0.0,0.0,0.0,0.0);

    // Draw a triangle
    GLES_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));
}

int main() {
    // Create a native Windows window
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = [](HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam) -> LRESULT {
      switch (uMsg) {
        case WM_CLOSE:
          DestroyWindow(hwnd);
          return 0;
        case WM_DESTROY:
          PostQuitMessage(0);
          return 0;
        default:
          return DefWindowProc(hwnd, uMsg, wParam, lParam);
      }
    };

    wc.hInstance = hInstance;
    wc.lpszClassName = L"TriangleWindowClass";

    if (!RegisterClass(&wc)) {
        printf("Window class registration failed\n");
        return 1;
    }

    HWND hWnd = CreateWindow(
        wc.lpszClassName,
        L"OpenGL ES 3.1 Triangle",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd) {
        printf("Window creation failed\n");
        return 1;
    }

    // Get the device context for the window
    HDC hdc = GetDC(hWnd);

    // Set up the pixel format descriptor
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    // Choose a pixel format
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        printf("Pixel format selection failed\n");
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Set the chosen pixel format
    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        printf("Pixel format setting failed\n");
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Create EGL display
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        printf("EGL display creation failed\n");
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Initialize EGL
    EGLint majorVersion, minorVersion;
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        printf("EGL initialization failed\n");
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Set EGL attributes
    EGLint eglAttributes[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    // Choose EGL config
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(display, eglAttributes, &config, 1, &numConfigs)) {
        printf("EGL config selection failed\n");
        eglTerminate(display);
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Create EGL context
    EGLint contextAttributes[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 1,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
    if (context == EGL_NO_CONTEXT) {
        printf("EGL context creation failed\n");
        eglTerminate(display);
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Create EGL surface
    EGLSurface surface = eglCreatePlatformWindowSurface(display, config, hWnd, NULL);
    if (surface == EGL_NO_SURFACE) {
        printf("EGL surface creation failed\n");
        eglDestroyContext(display, context);
        eglTerminate(display);
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    // Make EGL context current
    if (!eglMakeCurrent(display, surface, surface, context)) {
        printf("EGL context binding failed\n");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        ReleaseDC(hWnd, hdc);
        return 1;
    }

    printf("%s\n",glGetString(GL_VERSION));

    printf("Vertex--\n");
    // Load and compile the vertex shader
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderCode);

    printf("Fragment--\n");
    // Load and compile the fragment shader
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderCode);

    printf("Program+Link--\n");
    // Create and link the shader program
    GLuint shaderProgram = glCreateProgram();
    GLES_CALL(glAttachShader(shaderProgram, vertexShader));
    GLES_CALL(glAttachShader(shaderProgram, fragmentShader));
    GLES_CALL(glLinkProgram(shaderProgram));

    printf("--Shader Compilation OK\n");

    // Use the shader program
    GLES_CALL(glUseProgram(shaderProgram));

    // Create vertex data for the triangle
    GLfloat triangleVertices[] = {
        0.0f, 0.5f, 0.0f, // top
        -0.5f, -0.5f, 0.0f, // bottom left
        0.5f, -0.5f, 0.0f // bottom right
    };

    // Create and bind vertex buffer
    GLuint vbo;
    GLES_CALL(glGenBuffers(1, &vbo));
    GLES_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLES_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW));

    // Specify vertex data layout
    GLint positionAttributeLocation = GLES_CALL(glGetAttribLocation(shaderProgram, "aPosition"));
    GLES_CALL(glVertexAttribPointer(positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GLES_CALL(glEnableVertexAttribArray(positionAttributeLocation));

    UpdateWindow(hWnd);
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {

        // Ugly but it works---
        renderFrame();
        eglSwapBuffers(display, surface);
        //--- Ugly but it works

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up resources
    GLES_CALL(glDeleteBuffers(1, &vbo));
    GLES_CALL(glDeleteProgram(shaderProgram));
    GLES_CALL(glDeleteShader(vertexShader));
    GLES_CALL(glDeleteShader(fragmentShader));
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    ReleaseDC(hWnd, hdc);
    DestroyWindow(hWnd);

    return 0;
}
