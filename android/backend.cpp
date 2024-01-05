#include "core/backend.h"
#include "backend.h"

extern EGLDisplay g_EglDisplay;
extern EGLSurface g_EglSurface;

void funcSetMousePos(int, int)
{
    // Not implemented on Android
}

std::pair<int, int> funcBeginFrame()
{
    // Get display dimensions
    EGLint width, height;
    eglQuerySurface(g_EglDisplay, g_EglSurface, EGL_WIDTH, &width);
    eglQuerySurface(g_EglDisplay, g_EglSurface, EGL_HEIGHT, &height);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    return { (int)width, (int)height };
}

void funcEndFrame()
{
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

    if (satdump::light_theme)
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    else
        glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
    // glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

void funcSetIcon(uint8_t*, int, int)
{
    // Not implemented on Android
}

void bindBackendFunctions()
{
    backend::setMousePos = funcSetMousePos;
    backend::beginFrame = funcBeginFrame;
    backend::endFrame = funcEndFrame;
    backend::setIcon = funcSetIcon;
}
