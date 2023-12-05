#include "display/game_window.hpp"

int main()
{
    // Create the game window with the specified size and title
    GameWindow gw = GameWindow{2800, 1400, "wolfgang engine"};
    return gw.Run();
}