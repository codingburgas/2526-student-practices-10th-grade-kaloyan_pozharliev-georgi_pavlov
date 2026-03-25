#pragma once
#include "utils.h"

class App
{
public:
    App(Vector2 setScreen, std::string setName);
    ~App();

public:
    void Run();

private:
    AppState appState;
    Vector2 screen;
    std::string name;
    Font mainFont;
    SessionUser sessionUser;
};