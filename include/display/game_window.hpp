#include "base_window.hpp"

class GameWindow : public BaseWindow
{
public:
    GameWindow(int width, int height, std::string title) : BaseWindow(width, height, title){};
    void Initialize();
    void LoadContent();
    void Update();
    void Render();
    void GenerateImGui(unsigned int);
    void Unload();
    GLuint LoadTexture(unsigned char *);
    void CreateFramebuffer();
    void BindFramebuffer();
    void UnbindFramebuffer();
    void RescaleFramebuffer(float, float);

protected:
    GLuint texture;
    GLuint textureFrameBuffer;
    GLuint vertex_buffer;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLint mvp_location, vpos_location, vcol_location;
};