#include "app.h"
#include <raylib.h>
#include "PL/authScreen.h"
#include "PL/registerScreen.h"
#include "PL/mainScreen.h"
#include "PL/profileScreen.h"
#include "PL/cinemasScreen.h"
#include "DAL/Database.h"

App::App(Vector2 setScreen, std::string setName)
{
    screen = setScreen;
    name = setName;
    appState = AUTH;

    InitWindow((int)screen.x, (int)screen.y, name.c_str());
    SetTargetFPS(60);

    mainFont = LoadFontEx("fonts/AldotheApache.ttf", 32, 0, 250);
    Database::Get();
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
        case REG:
            appState = registerScreen(mainFont, sessionUser);
            break;
        case MAIN:
            appState = mainScreen(mainFont, sessionUser);
            break;
        case PROFILE:
            appState = profileScreen(mainFont, sessionUser);
            break;
        case CINEMAS:
            appState = cinemasScreen(mainFont, sessionUser);
			break;
        }
    }
}