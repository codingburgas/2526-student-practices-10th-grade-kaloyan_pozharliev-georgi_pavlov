#include "app.h"
#include <raylib.h>
#include "PL/authScreen.h"

App::App(Vector2 setScreen, std::string setName)
{
    screen = setScreen;
    name = setName;
    appState = AUTH;

    InitWindow((int)screen.x, (int)screen.y, name.c_str());
    SetTargetFPS(60);

    mainFont = LoadFontEx("fonts/AldotheApache.ttf", 32, 0, 250);
}

App::~App()
{
    UnloadFont(mainFont);
    CloseWindow();
}

void App::Run()
{
    while (!WindowShouldClose())
    {
        switch (appState)
        {
        case AUTH:
            appState = authScreen(mainFont, sessionUser);
            break;
        }
    }
}