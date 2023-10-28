#include "logger.h"
#include "core/style.h"
#include "gl.h"
#include "logger.h"
#include <signal.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui_flags.h"
#include "init.h"
#include "processing.h"
#include <filesystem>
#include "main_ui.h"
#include "satdump_vars.h"

#include "loading_screen.h"
#include "common/cli_utils.h"
#include "../src-core/resources.h"

static volatile bool signal_caught = false;

void sig_handler_ui(int signo)
{
    if (signo == SIGINT)
        signal_caught = true;
}

static void glfw_error_callback(int error, const char *description)
{
    logger->error("Glfw Error " + std::to_string(error) + ": " + description);
}

void window_content_scale_callback(GLFWwindow*, float xscale, float)
{
    satdump::updateUI(xscale / style::macos_framebuffer_scale());
    style::setFonts();
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
}

void bindImageTextureFunctions();

// OpenGL versions to try to start
// Yes, I did check on SDR++'s way around that
const int OPENGL_VERSIONS_MAJOR[] = {3, 3, 2};
const int OPENGL_VERSIONS_MINOR[] = {0, 1, 1};
const char *OPENGL_VERSIONS_GLSL[] = {"#version 150", "#version 300 es", "#version 120"};
const bool OPENGL_VERSIONS_GLES[] = {false, true, false, false};

int main(int argc, char *argv[])
{
    bindImageTextureFunctions();
    initLogger();

    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/i16/i8/w8] --dc_block --iq_swap");
    }
    else
        satdump::processing::is_processing = true;

    std::string downlink_pipeline = satdump::processing::is_processing ? argv[1] : "";
    std::string input_level = satdump::processing::is_processing ? argv[2] : "";
    std::string input_file = satdump::processing::is_processing ? argv[3] : "";
    std::string output_file = satdump::processing::is_processing ? argv[4] : "";

    // Parse flags
    nlohmann::json parameters = satdump::processing::is_processing ? parse_common_flags(argc - 5, &argv[5]) : "";

    // logger->warn("\n" + parameters.dump(4));
    // exit(0);

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        logger->critical("Could not init GLFW");
        exit(1);
    }

    // GL STUFF

    // Initialize GLFW
    GLFWwindow *window = nullptr;
    int final_gl_version = 0;

#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

    final_gl_version = 0;

    window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);

    if (window == NULL)
    {
        logger->critical("Could not init GLFW Window");
        exit(1);
    }
#elif defined(_WIN32)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

    final_gl_version = 0;

    window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);

    if (window == NULL) // Try 2.1. We assume it's the only available if 3.2 does not work
    {
        logger->warn("Could not init GLFW Window. Trying GL 2.1");
        glfwDefaultWindowHints();
        window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);
        final_gl_version = 2;

        if (window == NULL)
        {
            logger->critical("Could not init GLFW Window");
            exit(1);
        }
    }
#else
    const char *gl_override_rpi = getenv("MESA_GL_VERSION_OVERRIDE");
    if (gl_override_rpi != nullptr)
    {
        if (std::string(gl_override_rpi).find("4.5") != std::string::npos)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

            // Create window with graphics context
            window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);
            final_gl_version = 2;
            if (window == NULL)
            {
                logger->critical("Could not init GLFW Window with OpenGL 3.2 with %s", gl_override_rpi);
                exit(1);
            }
        }
        else
        {
            goto normal_gl;
        }
    }
    else
    {
    normal_gl:
        for (int i = 0; i < 4; i++)
        {
            glfwWindowHint(GLFW_CLIENT_API, OPENGL_VERSIONS_GLES[i] ? GLFW_OPENGL_ES_API : GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSIONS_MAJOR[i]);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSIONS_MINOR[i]);
            if (OPENGL_VERSIONS_MAJOR[i] >= 3 && OPENGL_VERSIONS_MINOR[i] >= 2)
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);               // Required on Mac

            // Create window with graphics context
            window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);
            final_gl_version = i;
            if (window == NULL)
                logger->critical("Could not init GLFW Window with OpenGL %d.%d", OPENGL_VERSIONS_MAJOR[i], OPENGL_VERSIONS_MINOR[i]);
            else
                break;
        }
    }
#endif

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable vsync on loading screen - not needed since frames are only pushed on log updates, and not in a loop
                         // Vsync slows down init process when items are logged quickly
    if (glewInit() != GLEW_OK)
    {
        logger->critical("Failed to initialize OpenGL loader!");
        exit(1);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;

    logger->debug("Starting with OpenGL %s", (char *)glGetString(GL_VERSION));

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(OPENGL_VERSIONS_GLSL[final_gl_version]);

    // Setup Icon
    GLFWimage img;
    {
        image::Image<uint8_t> image;
        image.load_png(resources::getResourcePath("icon.png"));
        uint8_t *px = new uint8_t[image.width() * image.height() * 4];
        memset(px, 255, image.width() * image.height() * 4);
        img.height = image.height();
        img.width = image.width();

        if (image.channels() == 4)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 4; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.channel(c)[image.width() * y + x];
        }
        else if (image.channels() == 3)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 3; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.channel(c)[image.width() * y + x];
        }
        image.clear();

        img.pixels = px;
    }

#ifndef _WIN32
    glfwSetWindowIcon(window, 1, &img);
#endif

    //Handle DPI changes
    float display_scale;
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwSetWindowContentScaleCallback(window, window_content_scale_callback);
    glfwGetWindowContentScale(window, &display_scale, nullptr);
    display_scale /= style::macos_framebuffer_scale();
#else
    display_scale = 1.0f;
#endif

    //Set font
    style::setFonts(display_scale);

    // Init Loading Screen
    std::shared_ptr<satdump::LoadingScreenSink> loading_screen_sink = std::make_shared<satdump::LoadingScreenSink>(window, display_scale, &img);
    logger->add_sink(loading_screen_sink);

    // Init SatDump
    satdump::tle_do_update_on_init = false;
    satdump::initSatdump();

    // Init UI
    satdump::initMainUI(display_scale);

    //Shut down loading screen
    logger->del_sink(loading_screen_sink);
    loading_screen_sink.reset();
    glfwSwapInterval(1); // Enable vsync for the rest of the program

    //Set font again to adjust for DPI
    style::setFonts();
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    if (satdump::processing::is_processing)
    {
        satdump::ui_thread_pool.push([&](int)
                                     { satdump::processing::process(downlink_pipeline, input_level, input_file, output_file, parameters); });
    }

    // TLE
    satdump::ui_thread_pool.push([&](int){ satdump::autoUpdateTLE(satdump::user_path + "/satdump_tles.txt"); });

    // Attach signal
    signal(SIGINT, sig_handler_ui);

    // Main loop
    do
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int wwidth, wheight;
        glfwGetWindowSize(window, &wwidth, &wheight);
        // std::cout<<wwidth<<std::endl;

        // User rendering
        satdump::renderMainUI(wwidth, wheight);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        if (satdump::light_theme)
            glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        else
            glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        // glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (!glfwWindowShouldClose(window) && !signal_caught);

    satdump::exitMainUI();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    logger->info("UI Exit");

    // If we're doing live processing, we want this to kill all threads quickly. Hence don't call destructors
    if (satdump::processing::is_processing)
#ifdef __APPLE__
        exit(0);
#else
        quick_exit(0);
#endif

    satdump::ui_thread_pool.stop();

    for (int i = 0; i < satdump::ui_thread_pool.size(); i++)
    {
        if (satdump::ui_thread_pool.get_thread(i).joinable())
            satdump::ui_thread_pool.get_thread(i).join();
    }

    logger->info("Exiting!");
}
