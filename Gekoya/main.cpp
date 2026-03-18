#include "raylib.h"
#include <string>
#include <cmath>

#define BG_DARK         Color{ 10,  13,  20,  255 }
#define BG_CARD         Color{ 22,  27,  40,  255 }
#define BG_INPUT        Color{ 14,  17,  28,  255 }
#define BORDER_NORMAL   Color{ 42,  50,  72,  255 }
#define BORDER_FOCUS    Color{ 82, 140, 255,  255 }
#define ACCENT          Color{ 72, 130, 255,  255 }
#define ACCENT_HOVER    Color{ 100, 155, 255,  255 }
#define TEXT_PRIMARY    Color{ 235, 238, 250,  255 }
#define TEXT_SECONDARY  Color{ 120, 132, 158,  255 }
#define TEXT_MUTED      Color{ 65,  78, 108,  255 }

static bool usernameActive = false;
static bool passwordActive = false;
static bool showPassword = false;
static bool rememberMe = false;
static std::string username = "";
static std::string password = "";

int main()
{
    InitWindow(1280, 780, "Gekoya - Sign In");
    SetTargetFPS(60);

    Font font = LoadFontEx("fonts/AldotheApache.ttf", 32, 0, 250);

    while (!WindowShouldClose())
    {
        int SW = GetScreenWidth();
        int SH = GetScreenHeight();

        float time = (float)GetTime();

        int panelW = 420;
        int panelH = 500;
        int panelX = SW / 2 - panelW / 2;
        int panelY = SH / 2 - panelH / 2;

        int fieldW = panelW - 64;
        int fieldH = 48;
        int fieldX = panelX + 32;

        int emailLabelY = panelY + 172;
        int emailFieldY = emailLabelY + 18;
        int passLabelY = emailFieldY + fieldH + 20;
        int passFieldY = passLabelY + 18;
        int rememberY = passFieldY + fieldH + 18;
        int signBtnY = rememberY + 42;
        int signUpY = signBtnY + fieldH + 22;

        Rectangle emailRect = { (float)fieldX, (float)emailFieldY, (float)fieldW, (float)fieldH };
        Rectangle passRect = { (float)fieldX, (float)passFieldY,  (float)fieldW, (float)fieldH };
        Rectangle rememberRect = { (float)fieldX, (float)rememberY,   18, 18 };
        Rectangle signBtnRect = { (float)fieldX, (float)signBtnY,    (float)fieldW, (float)fieldH };
        Rectangle eyeRect = { (float)(fieldX + fieldW - 38), (float)(passFieldY + 13), 22, 22 };
        Rectangle forgotRect = { (float)(fieldX + fieldW - 120), (float)rememberY, 120, 18 };

        Vector2 mouse = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (clicked)
        {
            usernameActive = CheckCollisionPointRec(mouse, emailRect);
            passwordActive = CheckCollisionPointRec(mouse, passRect);
            if (CheckCollisionPointRec(mouse, eyeRect))      showPassword = !showPassword;
            if (CheckCollisionPointRec(mouse, rememberRect)) rememberMe = !rememberMe;
        }

        if (IsKeyPressed(KEY_BACKSPACE))
        {
            if (usernameActive && !username.empty()) username.pop_back();
            if (passwordActive && !password.empty()) password.pop_back();
        }

        int ch = GetCharPressed();
        while (ch > 0)
        {
            if (usernameActive && username.size() < 48) username += (char)ch;
            if (passwordActive && password.size() < 48) password += (char)ch;
            ch = GetCharPressed();
        }

        std::string passDots = "";
        for (int i = 0; i < (int)password.size(); i++) passDots += "*";

        bool hoverBtn = CheckCollisionPointRec(mouse, signBtnRect);
        bool hoverForgot = CheckCollisionPointRec(mouse, forgotRect);

        const char* preText = "Don't have an account?";
        const char* noAccText = "Sign up";
        Vector2 preSz = MeasureTextEx(font, preText, 13, 0.5f);
        Vector2 noAccSz = MeasureTextEx(font, noAccText, 13, 0.5f);
        float signUpRowW = preSz.x + 6 + noAccSz.x;
        float signUpX = panelX + (panelW - signUpRowW) / 2.0f;
        Rectangle signUpRect = { signUpX + preSz.x + 6, (float)signUpY, noAccSz.x, noAccSz.y };
        bool hoverSignUp = CheckCollisionPointRec(mouse, signUpRect);

        BeginDrawing();
        ClearBackground(BG_DARK);

        float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

        for (int r = 280; r >= 0; r -= 14)
        {
            float t = 1.0f - (float)r / 280.0f;
            unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f));
            DrawCircle((int)(SW * 0.12f), (int)(SH * 0.18f), (float)r,
                Color{ 40, 90, 255, a });
        }

        for (int r = 260; r >= 0; r -= 14)
        {
            float t = 1.0f - (float)r / 260.0f;
            unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f));
            DrawCircle((int)(SW * 0.88f), (int)(SH * 0.82f), (float)r,
                Color{ 50, 80, 220, a });
        }

        for (int r = 200; r >= 0; r -= 14)
        {
            float t = 1.0f - (float)r / 200.0f;
            unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f));
            DrawCircle((int)(SW * 0.85f), (int)(SH * 0.15f), (float)r,
                Color{ 80, 50, 200, a });
        }

        for (int r = 180; r >= 0; r -= 14)
        {
            float t = 1.0f - (float)r / 180.0f;
            unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f));
            DrawCircle((int)(SW * 0.14f), (int)(SH * 0.80f), (float)r,
                Color{ 30, 70, 200, a });
        }

        for (int r = 340; r >= 0; r -= 14)
        {
            float t = 1.0f - (float)r / 340.0f;
            unsigned char a = (unsigned char)(t * t * 22.0f);
            DrawCircle(SW / 2, SH / 2, (float)r, Color{ 55, 95, 210, a });
        }

        DrawRectangle(panelX + 6, panelY + 10, panelW, panelH, Color{ 0, 0, 0, 60 });
        DrawRectangleRounded({ (float)panelX, (float)panelY, (float)panelW, (float)panelH }, 0.06f, 10, BG_CARD);
        DrawRectangleRoundedLines({ (float)panelX, (float)panelY, (float)panelW, (float)panelH }, 0.06f, 10, BORDER_NORMAL);

        Vector2 logoSz = MeasureTextEx(font, "Gekoya", 22, 1.5f);
        DrawTextEx(font, "Gekoya", { (float)(SW / 2) - logoSz.x / 2, (float)(panelY + 38) }, 22, 1.5f, TEXT_PRIMARY);

        Vector2 h1Sz = MeasureTextEx(font, "Welcome back", 20, 1.2f);
        DrawTextEx(font, "Welcome back", { (float)(SW / 2) - h1Sz.x / 2, (float)(panelY + 100) }, 20, 1.2f, TEXT_PRIMARY);
        Vector2 h2Sz = MeasureTextEx(font, "Sign in to your Gekoya account", 13, 0.5f);
        DrawTextEx(font, "Sign in to your Gekoya account", { (float)(SW / 2) - h2Sz.x / 2, (float)(panelY + 130) }, 13, 0.5f, TEXT_SECONDARY);

        DrawTextEx(font, "Email Address", { (float)fieldX, (float)emailLabelY }, 12, 0.5f, TEXT_SECONDARY);
        DrawRectangleRounded(emailRect, 0.18f, 8, BG_INPUT);
        DrawRectangleRoundedLines(emailRect, 0.18f, 8, usernameActive ? BORDER_FOCUS : BORDER_NORMAL);
        DrawTextEx(font, "@", { (float)(fieldX + 14), (float)(emailFieldY + 15) }, 16, 0, TEXT_MUTED);
        if (username.empty() && !usernameActive)
            DrawTextEx(font, "you@example.com", { (float)(fieldX + 38), (float)(emailFieldY + 16) }, 13, 0.5f, TEXT_MUTED);
        else
            DrawTextEx(font, username.c_str(), { (float)(fieldX + 38), (float)(emailFieldY + 16) }, 13, 0.5f, TEXT_PRIMARY);
        if (usernameActive && ((int)(GetTime() * 2)) % 2 == 0)
        {
            float cx = fieldX + 38 + MeasureTextEx(font, username.c_str(), 13, 0.5f).x + 2;
            DrawRectangle((int)cx, emailFieldY + 12, 2, 22, BORDER_FOCUS);
        }

        DrawTextEx(font, "Password", { (float)fieldX, (float)passLabelY }, 12, 0.5f, TEXT_SECONDARY);
        DrawRectangleRounded(passRect, 0.18f, 8, BG_INPUT);
        DrawRectangleRoundedLines(passRect, 0.18f, 8, passwordActive ? BORDER_FOCUS : BORDER_NORMAL);
        DrawTextEx(font, "~", { (float)(fieldX + 14), (float)(passFieldY + 14) }, 18, 0, TEXT_MUTED);
        if (password.empty() && !passwordActive)
            DrawTextEx(font, "........", { (float)(fieldX + 38), (float)(passFieldY + 16) }, 13, 0.5f, TEXT_MUTED);
        else
        {
            std::string shown = showPassword ? password : passDots;
            DrawTextEx(font, shown.c_str(), { (float)(fieldX + 38), (float)(passFieldY + 16) }, 13, 0.5f, TEXT_PRIMARY);
        }
        DrawTextEx(font, showPassword ? "o" : "O", { eyeRect.x + 3, eyeRect.y + 2 }, 16, 0,
            CheckCollisionPointRec(mouse, eyeRect) ? TEXT_PRIMARY : TEXT_SECONDARY);
        if (passwordActive && ((int)(GetTime() * 2)) % 2 == 0)
        {
            std::string shown = showPassword ? password : passDots;
            float cx = fieldX + 38 + MeasureTextEx(font, shown.c_str(), 13, 0.5f).x + 2;
            DrawRectangle((int)cx, passFieldY + 12, 2, 22, BORDER_FOCUS);
        }

        DrawRectangleRounded(rememberRect, 0.2f, 4, BG_INPUT);
        DrawRectangleRoundedLines(rememberRect, 0.2f, 4, BORDER_NORMAL);
        if (rememberMe)
            DrawRectangleRounded({ rememberRect.x + 3, rememberRect.y + 3, 12, 12 }, 0.3f, 4, ACCENT);
        DrawTextEx(font, "Remember me", { rememberRect.x + 26, rememberRect.y + 2 }, 13, 0.5f, TEXT_SECONDARY);

        DrawTextEx(font, "Forgot password?", { forgotRect.x, forgotRect.y }, 13, 0.5f, hoverForgot ? TEXT_PRIMARY : ACCENT);

        if (hoverBtn)
            DrawRectangleRounded({ signBtnRect.x - 4, signBtnRect.y - 4, signBtnRect.width + 8, signBtnRect.height + 8 },
                0.35f, 8, Color{ 72, 130, 255, 28 });
        DrawRectangleRounded(signBtnRect, 0.35f, 8, hoverBtn ? ACCENT_HOVER : ACCENT);
        Vector2 btnTxtSz = MeasureTextEx(font, "Sign In", 15, 1.2f);
        DrawTextEx(font, "Sign In",
            { signBtnRect.x + signBtnRect.width / 2 - btnTxtSz.x / 2,
              signBtnRect.y + signBtnRect.height / 2 - btnTxtSz.y / 2 },
            15, 1.2f, WHITE);

        DrawTextEx(font, preText, { signUpX,                (float)signUpY }, 13, 0.5f, TEXT_SECONDARY);
        DrawTextEx(font, noAccText, { signUpX + preSz.x + 6, (float)signUpY }, 13, 0.5f, hoverSignUp ? TEXT_PRIMARY : ACCENT);

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}