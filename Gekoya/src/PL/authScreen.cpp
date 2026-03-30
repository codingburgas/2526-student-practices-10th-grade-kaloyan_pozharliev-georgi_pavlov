#include "authScreen.h"
#include "../colors.h"
#include "../BLL/AuthService.h"
#include <string>
#include <cmath>

static std::string username = "";
static std::string password = "";

static bool usernameActive = false;
static bool passwordActive = false;
static bool showPassword = false;
static bool rememberMe = false;

static bool showError = false;
static std::string errorMsg = "";

static void HandleInput()
{
    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (usernameActive && !username.empty()) username.pop_back();
        if (passwordActive && !password.empty()) password.pop_back();
    }

    int key = GetCharPressed();
    while (key > 0)
    {
        if (usernameActive && username.length() < 48) username += (char)key;
        if (passwordActive && password.length() < 48) password += (char)key;
        key = GetCharPressed();
    }
}

static std::string GetPasswordMask()
{
    std::string mask = "";
    for (int i = 0; i < (int)password.length(); i++) mask += "*";
    return mask;
}

AppState authScreen(Font font, SessionUser& sessionUser)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float time = (float)GetTime();

    int panelW = 420;
    int panelH = 500;
    int panelX = screenW / 2 - panelW / 2;
    int panelY = screenH / 2 - panelH / 2;

    int fieldW = panelW - 64;
    int fieldH = 48;
    int fieldX = panelX + 32;

    int userLabelY = panelY + 172;
    int userFieldY = userLabelY + 18;
    int passLabelY = userFieldY + fieldH + 20;
    int passFieldY = passLabelY + 18;
    int rememberY = passFieldY + fieldH + 18;
    int signInBtnY = rememberY + 42;
    int signUpLinkY = signInBtnY + fieldH + 22;

    Rectangle usernameField = { (float)fieldX, (float)userFieldY, (float)fieldW, (float)fieldH };
    Rectangle passField = { (float)fieldX, (float)passFieldY, (float)fieldW, (float)fieldH };
    Rectangle rememberBox = { (float)fieldX, (float)rememberY, 18, 18 };
    Rectangle signInBtn = { (float)fieldX, (float)signInBtnY, (float)fieldW, (float)fieldH };
    Rectangle eyeBtn = { (float)(fieldX + fieldW - 38), (float)(passFieldY + 13), 22, 22 };
    Rectangle forgotLink = { (float)(fieldX + fieldW - 120), (float)rememberY, 120, 18 };

    Vector2 mouse = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked)
    {
        usernameActive = CheckCollisionPointRec(mouse, usernameField);
        passwordActive = CheckCollisionPointRec(mouse, passField);
        if (CheckCollisionPointRec(mouse, eyeBtn)) showPassword = !showPassword;
        if (CheckCollisionPointRec(mouse, rememberBox)) rememberMe = !rememberMe;
    }

    HandleInput();

    std::string passwordMask = GetPasswordMask();

    bool hoverSignIn = CheckCollisionPointRec(mouse, signInBtn);
    bool hoverForgot = CheckCollisionPointRec(mouse, forgotLink);

    const char* signUpPreText = "Don't have an account?";
    const char* signUpLinkText = "Sign up";
    Vector2 preTextSize = MeasureTextEx(font, signUpPreText, 13, 0.5f);
    Vector2 linkTextSize = MeasureTextEx(font, signUpLinkText, 13, 0.5f);
    float signUpRowW = preTextSize.x + 6 + linkTextSize.x;
    float signUpStartX = panelX + (panelW - signUpRowW) / 2.0f;
    Rectangle signUpLink = { signUpStartX + preTextSize.x + 6, (float)signUpLinkY, linkTextSize.x, linkTextSize.y };
    bool hoverSignUp = CheckCollisionPointRec(mouse, signUpLink);

    BeginDrawing();
    ClearBackground(BG_DARK);

    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    for (int r = 280; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 280.0f;
        unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f));
        DrawCircle((int)(screenW * 0.12f), (int)(screenH * 0.18f), (float)r, Color{ 40, 90, 255, a });
    }
    for (int r = 260; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 260.0f;
        unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f));
        DrawCircle((int)(screenW * 0.88f), (int)(screenH * 0.82f), (float)r, Color{ 50, 80, 220, a });
    }
    for (int r = 200; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 200.0f;
        unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f));
        DrawCircle((int)(screenW * 0.85f), (int)(screenH * 0.15f), (float)r, Color{ 80, 50, 200, a });
    }
    for (int r = 180; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 180.0f;
        unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f));
        DrawCircle((int)(screenW * 0.14f), (int)(screenH * 0.80f), (float)r, Color{ 30, 70, 200, a });
    }
    for (int r = 340; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 340.0f;
        unsigned char a = (unsigned char)(t * t * 22.0f);
        DrawCircle(screenW / 2, screenH / 2, (float)r, Color{ 55, 95, 210, a });
    }

    DrawRectangle(panelX + 6, panelY + 10, panelW, panelH, Color{ 0, 0, 0, 60 });
    DrawRectangleRounded({ (float)panelX, (float)panelY, (float)panelW, (float)panelH }, 0.06f, 10, BG_CARD);
    DrawRectangleRoundedLines({ (float)panelX, (float)panelY, (float)panelW, (float)panelH }, 0.06f, 10, BORDER_NORMAL);

    Vector2 logoSize = MeasureTextEx(font, "Gekoya", 22, 1.5f);
    DrawTextEx(font, "Gekoya", { (float)(screenW / 2) - logoSize.x / 2, (float)(panelY + 38) }, 22, 1.5f, TEXT_PRIMARY);

    Vector2 headingSize = MeasureTextEx(font, "Welcome back", 20, 1.2f);
    DrawTextEx(font, "Welcome back", { (float)(screenW / 2) - headingSize.x / 2, (float)(panelY + 100) }, 20, 1.2f, TEXT_PRIMARY);

    Vector2 subheadingSize = MeasureTextEx(font, "Sign in to your Gekoya account", 13, 0.5f);
    DrawTextEx(font, "Sign in to your Gekoya account", { (float)(screenW / 2) - subheadingSize.x / 2, (float)(panelY + 130) }, 13, 0.5f, TEXT_SECONDARY);

    if (showError)
    {
        Rectangle errBox = { (float)fieldX, (float)(panelY + 155), (float)fieldW, 34 };
        DrawRectangleRec(errBox, Color{ 40, 20, 20, 255 });
        DrawRectangleLinesEx(errBox, 1, Color{ 200, 70, 70, 255 });
        DrawTextEx(font, errorMsg.c_str(), { errBox.x + 12, errBox.y + 10 }, 11, 1, Color{ 200, 70, 70, 255 });
    }

    DrawTextEx(font, "USERNAME", { (float)fieldX, (float)userLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(usernameField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(usernameField, 0.18f, 8, usernameActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (username.empty() && !usernameActive)
        DrawTextEx(font, "ENTER USERNAME", { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
        DrawTextEx(font, username.c_str(), { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    if (usernameActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        float cursorX = fieldX + 14 + MeasureTextEx(font, username.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, userFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    DrawTextEx(font, "PASSWORD", { (float)fieldX, (float)passLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(passField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(passField, 0.18f, 8, passwordActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (password.empty() && !passwordActive)
        DrawTextEx(font, "ENTER PASSWORD", { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
    {
        std::string shown = showPassword ? password : passwordMask;
        DrawTextEx(font, shown.c_str(), { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    }
    DrawTextEx(font, showPassword ? "o" : "O", { eyeBtn.x + 3, eyeBtn.y + 2 }, 16, 0,
        CheckCollisionPointRec(mouse, eyeBtn) ? TEXT_PRIMARY : TEXT_SECONDARY);
    if (passwordActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        std::string shown = showPassword ? password : passwordMask;
        float cursorX = fieldX + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, passFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    DrawRectangleRounded(rememberBox, 0.2f, 4, BG_INPUT);
    DrawRectangleRoundedLines(rememberBox, 0.2f, 4, BORDER_NORMAL);
    if (rememberMe)
        DrawRectangleRounded({ rememberBox.x + 3, rememberBox.y + 3, 12, 12 }, 0.3f, 4, ACCENT);
    DrawTextEx(font, "REMEMBER ME", { rememberBox.x + 26, rememberBox.y + 2 }, 11, 1, TEXT_SECONDARY);

    DrawTextEx(font, "FORGOT PASSWORD?", { forgotLink.x, forgotLink.y }, 11, 1, hoverForgot ? TEXT_PRIMARY : ACCENT);

    if (hoverSignIn)
        DrawRectangleRounded({ signInBtn.x - 4, signInBtn.y - 4, signInBtn.width + 8, signInBtn.height + 8 }, 0.35f, 8, Color{ 72, 130, 255, 28 });
    DrawRectangleRounded(signInBtn, 0.35f, 8, hoverSignIn ? ACCENT_HOVER : ACCENT);
    Vector2 signInTextSize = MeasureTextEx(font, "SIGN IN", 15, 1);
    DrawTextEx(font, "SIGN IN", { signInBtn.x + signInBtn.width / 2 - signInTextSize.x / 2, signInBtn.y + signInBtn.height / 2 - signInTextSize.y / 2 }, 15, 1, WHITE);

    DrawTextEx(font, signUpPreText, { signUpStartX, (float)signUpLinkY }, 13, 0.5f, TEXT_SECONDARY);
    DrawTextEx(font, signUpLinkText, { signUpStartX + preTextSize.x + 6, (float)signUpLinkY }, 13, 0.5f, hoverSignUp ? TEXT_PRIMARY : ACCENT);

    EndDrawing();

    if (clicked && CheckCollisionPointRec(mouse, signUpLink))
        return REG;

    if (clicked && CheckCollisionPointRec(mouse, signInBtn))
    {
        if (username.empty() || password.empty())
        {
            showError = true;
            errorMsg = "ERROR: ALL FIELDS REQUIRED";
        }
        else
        {
            bool success = AuthService::Login(username, password);
            if (success)
            {
                showError = false;
                sessionUser.username = username;
                username = "";
                password = "";
                return AUTH;
            }
            else
            {
                showError = true;
                errorMsg = "ERROR: INVALID CREDENTIALS";
            }
        }
    }

    return AUTH;
}