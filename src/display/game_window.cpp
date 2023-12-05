#include "display/game_window.hpp"
#include "shaders/shader.hpp"
#include <iostream>

// Template stuff
Shader s;
unsigned int EBO;
// global defined indices for OpenGL
GLuint VAO;        // vertex array object
GLuint VBO;        // vertex buffer object
GLuint FBO;        // frame buffer object
GLuint RBO;        // rendering buffer object
GLuint texture_id; // the texture id we'll need later to create a texture
                   // out of our framebuffer

// Called whenever the window or framebuffer's size is changed
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// 1. The first thing that is run when starting the window
void GameWindow::Initialize()
{
    // Set GLFW stuff before window is created
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

// 2. Run after the window has been created, as well as the OpenGL context
void GameWindow::LoadContent()
{
    // Set callback
    glfwSetFramebufferSizeCallback(this->windowHandle, FramebufferSizeCallback);

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplGlfw_InitForOpenGL(this->windowHandle, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    std::cout << "INFO::IMGUI::SUCCESSFULLY_INITIALIZED" << std::endl;

    // Load the template shader
    s = Shader::LoadShader("../resources/shaders/testing.vs", "../resources/shaders/testing.fs");

    // Vertices needed for a square
    float vertices[] = {
        0.5f, 0.5f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f   // top left
    };

    // float vertices[] = {
    //     -0.5f, -0.5f, 0.0f,
    //     0.5f, -0.5f, 0.0f,
    //     0.0f, 0.5f, 0.0f};

    // Indices for rendering the above square
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    // Create Vertex Array object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); // And bind it

    // Create Vertex Buffer object
    glGenBuffers(1, &VBO);
    // And bind it (this also includes it into the VAO)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Fill the VBO with vertex data, simply positions
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // layout = 0 should contain these positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0); // Enable that shit

    // Create index buffer
    glGenBuffers(1, &EBO);
    // And bind it (also included in VAO)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // Fill with indices!
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GameWindow::CreateFramebuffer();
}

void GameWindow::Update()
{
    // Performs hot-reload of shader. Only reloads whenever it has been modified - so not every frame.
    s.ReloadFromFile();
}

void GameWindow::Render()
{
    glfwPollEvents();

    // Make sure we're using the correct shader program.
    // Must be done per-frame, since the shader program id might change when hot-reloading
    glUseProgram(s.programID);

    // Clear the window
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Recreate framebuffer.
    //  If you dont recreate you get weird scaling/position/stretching issues.
    //  I think this is because it is updating window position/dims but not updating the render texture size accordingly and is
    //  acting strangely.
    GameWindow::CreateFramebuffer();

    // Create new imgui frames
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // we access the ImGui window size
    const float window_width = ImGui::GetContentRegionAvail().x;
    const float window_height = ImGui::GetContentRegionAvail().y;

    // we rescale the framebuffer to the actual window size here and reset the glViewport
    GameWindow::RescaleFramebuffer(window_width, window_height);
    glViewport(0, 0, window_width, window_height);

    // we get the screen position of the window
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Draw imgui
    // ImGui::ShowDemoWindow();
    GameWindow::GenerateImGui(texture_id);

    ImGui::Render();

    GameWindow::BindFramebuffer();

    // Bind the VAO
    glBindVertexArray(VAO);

    // Draw the square
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // glDrawArrays(GL_TRIANGLES, 0, 3);

    GameWindow::UnbindFramebuffer();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Could be causing large copy instructions on gpu. When viewports on, graphics card copy goes to 100%
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &texture_id);

    // Swap double buffers and poll OS-events
    glfwSwapBuffers(this->windowHandle);
}

void GameWindow::GenerateImGui(unsigned int color)
{
    static bool show_tool_style_editor = true;

    // Create DockSpace. Allows snapping to parts of the screen.
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    if (ImGui::Begin("Game Window", &show_tool_style_editor))
    {
        ImGui::BeginChild("GameRenderer");

        ImVec2 wsize = ImGui::GetWindowSize();

        // ImGui::Image((ImTextureID)color, wsize, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::Image(
            (void *)texture_id,
            ImVec2(wsize),
            ImVec2(0, 1),
            ImVec2(1, 0));
        ImGui::EndChild();
    }
    ImGui::End();
}

void GameWindow::CreateFramebuffer()
{
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->windowWidth, this->windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

    // glGenRenderbuffers(1, &RBO);
    // glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->windowWidth, this->windowHeight);
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

// here we bind our framebuffer
void GameWindow::BindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

// here we unbind our framebuffer
void GameWindow::UnbindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// and we rescale the buffer, so we're able to resize the window
void GameWindow::RescaleFramebuffer(float width, float height)
{
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
}

void GameWindow::Unload()
{
    // Destroy imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

GLuint GameWindow::LoadTexture(unsigned char *data)
{
    // Create an openGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->windowWidth, this->windowHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    // Read the file, call glTexImage2D with the right parameters
    // glfwLoadTexture2D(imagepath, 0);

    // Nice trilinear filtering.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glGenerateMipmap(GL_TEXTURE_2D);

    return textureID;
}