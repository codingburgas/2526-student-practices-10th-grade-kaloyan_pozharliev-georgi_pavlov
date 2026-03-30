#include "registerScreen.h"
#include "../colors.h"
#include "../BLL/AuthService.h"
#include <string>
#include <cmath>

static std::string regUsername = "";
static std::string regEmail = "";
static std::string regPassword = "";
static std::string regConfirm = "";

static bool usernameActive = false;
static bool emailActive = false;
static bool passwordActive = false;
static bool confirmActive = false;
static bool showPassword = false;
static bool showConfirm = false;

static bool showError = false;
static std::string errorMsg = "";

static void HandleInput()
{
    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (usernameActive && !regUsername.empty()) regUsername.pop_back();
        if (emailActive && !regEmail.empty()) regEmail.pop_back();
        if (passwordActive && !regPassword.empty()) regPassword.pop_back();
        if (confirmActive && !regConfirm.empty()) regConfirm.pop_back();
    }

    int key = GetCharPressed();
    while (key > 0)
    {
        if (usernameActive && regUsername.length() < 32) regUsername += (char)key;
        if (emailActive && regEmail.length() < 48) regEmail += (char)key;
        if (passwordActive && regPassword.length() < 48) regPassword += (char)key;
        if (confirmActive && regConfirm.length() < 48) regConfirm += (char)key;
        key = GetCharPressed();
    }
}

static std::string GetMask(const std::string& input)
{
    std::string mask = "";
    for (int i = 0; i < (int)input.length(); i++) mask += "*";
    return mask;
}

AppState registerScreen(Font font, SessionUser& sessionUser)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float time = (float)GetTime();

    int panelW = 420;
    int panelH = 620;
    int panelX = screenW / 2 - panelW / 2;
    int panelY = screenH / 2 - panelH / 2;

    int fieldW = panelW - 64;
    int fieldH = 48;
    int fieldX = panelX + 32;

    int userLabelY = panelY + 140;
    int userFieldY = userLabelY + 18;
    int emailLabelY = userFieldY + fieldH + 20;
    int emailFieldY = emailLabelY + 18;
    int passLabelY = emailFieldY + fieldH + 20;
    int passFieldY = passLabelY + 18;
    int confLabelY = passFieldY + fieldH + 20;
    int confFieldY = confLabelY + 18;
    int signUpBtnY = confFieldY + fieldH + 24;
    int signInLinkY = signUpBtnY + fieldH + 22;

    Rectangle usernameField = { (float)fieldX, (float)userFieldY, (float)fieldW, (float)fieldH };
    Rectangle emailField = { (float)fieldX, (float)emailFieldY, (float)fieldW, (float)fieldH };
    Rectangle passField = { (float)fieldX, (float)passFieldY, (float)fieldW, (float)fieldH };
    Rectangle confirmField = { (float)fieldX, (float)confFieldY, (float)fieldW, (float)fieldH };
    Rectangle signUpBtn = { (float)fieldX, (float)signUpBtnY, (float)fieldW, (float)fieldH };
    Rectangle eyePassBtn = { (float)(fieldX + fieldW - 38), (float)(passFieldY + 13), 22, 22 };
    Rectangle eyeConfBtn = { (float)(fieldX + fieldW - 38), (float)(confFieldY + 13), 22, 22 };

    Vector2 mouse = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked)
    {
        usernameActive = CheckCollisionPointRec(mouse, usernameField);
        emailActive = CheckCollisionPointRec(mouse, emailField);
        passwordActive = CheckCollisionPointRec(mouse, passField);
        confirmActive = CheckCollisionPointRec(mouse, confirmField);
        if (CheckCollisionPointRec(mouse, eyePassBtn)) showPassword = !showPassword;
        if (CheckCollisionPointRec(mouse, eyeConfBtn)) showConfirm = !showConfirm;
    }

    HandleInput();

    std::string passwordMask = GetMask(regPassword);
    std::string confirmMask = GetMask(regConfirm);

    bool hoverSignUp = CheckCollisionPointRec(mouse, signUpBtn);

    const char* signInPreText = "Already have an account?";
    const char* signInLinkText = "Sign in";
    Vector2 preTextSize = MeasureTextEx(font, signInPreText, 13, 0.5f);
    Vector2 linkTextSize = MeasureTextEx(font, signInLinkText, 13, 0.5f);
    float signInRowW = preTextSize.x + 6 + linkTextSize.x;
    float signInStartX = panelX + (panelW - signInRowW) / 2.0f;
    Rectangle signInLink = { signInStartX + preTextSize.x + 6, (float)signInLinkY, linkTextSize.x, linkTextSize.y };
    bool hoverSignIn = CheckCollisionPointRec(mouse, signInLink);

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
    DrawTextEx(font, "Gekoya", { (float)(screenW / 2) - logoSize.x / 2, (float)(panelY + 30) }, 22, 1.5f, TEXT_PRIMARY);

    Vector2 headingSize = MeasureTextEx(font, "Create account", 20, 1.2f);
    DrawTextEx(font, "Create account", { (float)(screenW / 2) - headingSize.x / 2, (float)(panelY + 78) }, 20, 1.2f, TEXT_PRIMARY);

    Vector2 subheadingSize = MeasureTextEx(font, "Sign up for a new Gekoya account", 13, 0.5f);
    DrawTextEx(font, "Sign up for a new Gekoya account", { (float)(screenW / 2) - subheadingSize.x / 2, (float)(panelY + 108) }, 13, 0.5f, TEXT_SECONDARY);

    if (showError)
    {
        Rectangle errBox = { (float)fieldX, (float)(panelY + 128), (float)fieldW, 34 };
        DrawRectangleRec(errBox, Color{ 40, 20, 20, 255 });
        DrawRectangleLinesEx(errBox, 1, Color{ 200, 70, 70, 255 });
        DrawTextEx(font, errorMsg.c_str(), { errBox.x + 12, errBox.y + 10 }, 11, 1, Color{ 200, 70, 70, 255 });
    }

    DrawTextEx(font, "USERNAME", { (float)fieldX, (float)userLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(usernameField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(usernameField, 0.18f, 8, usernameActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (regUsername.empty() && !usernameActive)
        DrawTextEx(font, "ENTER USERNAME", { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
        DrawTextEx(font, regUsername.c_str(), { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    if (usernameActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        float cursorX = fieldX + 14 + MeasureTextEx(font, regUsername.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, userFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    DrawTextEx(font, "EMAIL ADDRESS", { (float)fieldX, (float)emailLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(emailField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(emailField, 0.18f, 8, emailActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (regEmail.empty() && !emailActive)
        DrawTextEx(font, "ENTER EMAIL", { (float)(fieldX + 14), (float)(emailFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
        DrawTextEx(font, regEmail.c_str(), { (float)(fieldX + 14), (float)(emailFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    if (emailActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        float cursorX = fieldX + 14 + MeasureTextEx(font, regEmail.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, emailFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    DrawTextEx(font, "PASSWORD", { (float)fieldX, (float)passLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(passField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(passField, 0.18f, 8, passwordActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (regPassword.empty() && !passwordActive)
        DrawTextEx(font, "ENTER PASSWORD", { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
    {
        std::string shown = showPassword ? regPassword : passwordMask;
        DrawTextEx(font, shown.c_str(), { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    }
    DrawTextEx(font, showPassword ? "o" : "O", { eyePassBtn.x + 3, eyePassBtn.y + 2 }, 16, 0,
        CheckCollisionPointRec(mouse, eyePassBtn) ? TEXT_PRIMARY : TEXT_SECONDARY);
    if (passwordActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        std::string shown = showPassword ? regPassword : passwordMask;
        float cursorX = fieldX + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, passFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    DrawTextEx(font, "CONFIRM PASSWORD", { (float)fieldX, (float)confLabelY }, 11, 1, TEXT_SECONDARY);
    DrawRectangleRounded(confirmField, 0.18f, 8, BG_INPUT);
    DrawRectangleRoundedLines(confirmField, 0.18f, 8, confirmActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (regConfirm.empty() && !confirmActive)
        DrawTextEx(font, "REPEAT PASSWORD", { (float)(fieldX + 14), (float)(confFieldY + 16) }, 13, 1, TEXT_MUTED);
    else
    {
        std::string shown = showConfirm ? regConfirm : confirmMask;
        DrawTextEx(font, shown.c_str(), { (float)(fieldX + 14), (float)(confFieldY + 16) }, 13, 1, TEXT_PRIMARY);
    }
    DrawTextEx(font, showConfirm ? "o" : "O", { eyeConfBtn.x + 3, eyeConfBtn.y + 2 }, 16, 0,
        CheckCollisionPointRec(mouse, eyeConfBtn) ? TEXT_PRIMARY : TEXT_SECONDARY);
    if (confirmActive && ((int)(GetTime() * 2)) % 2 == 0)
    {
        std::string shown = showConfirm ? regConfirm : confirmMask;
        float cursorX = fieldX + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cursorX, confFieldY + 12, 2, 22, BORDER_FOCUS);
    }

    if (hoverSignUp)
        DrawRectangleRounded({ signUpBtn.x - 4, signUpBtn.y - 4, signUpBtn.width + 8, signUpBtn.height + 8 },
            0.35f, 8, Color{ 72, 130, 255, 28 });
    DrawRectangleRounded(signUpBtn, 0.35f, 8, hoverSignUp ? ACCENT_HOVER : ACCENT);
    Vector2 signUpTextSize = MeasureTextEx(font, "CREATE ACCOUNT", 15, 1);
    DrawTextEx(font, "CREATE ACCOUNT",
        { signUpBtn.x + signUpBtn.width / 2 - signUpTextSize.x / 2,
          signUpBtn.y + signUpBtn.height / 2 - signUpTextSize.y / 2 },
        15, 1, WHITE);

    DrawTextEx(font, signInPreText, { signInStartX, (float)signInLinkY }, 13, 0.5f, TEXT_SECONDARY);
    DrawTextEx(font, signInLinkText, { signInStartX + preTextSize.x + 6, (float)signInLinkY }, 13, 0.5f, hoverSignIn ? TEXT_PRIMARY : ACCENT);

    EndDrawing();

    if (clicked && CheckCollisionPointRec(mouse, signInLink))
        return AUTH;

    if (clicked && CheckCollisionPointRec(mouse, signUpBtn))
    {
        if (regUsername.empty() || regEmail.empty() || regPassword.empty() || regConfirm.empty())
        {
            showError = true;
            errorMsg = "ERROR: ALL FIELDS REQUIRED";
        }
        else if (regPassword != regConfirm)
        {
            showError = true;
            errorMsg = "ERROR: PASSWORDS DO NOT MATCH";
        }
        else
        {
            bool success = AuthService::Register(regUsername, regPassword, 1);
            if (!success)
            {
                showError = true;
                errorMsg = "ERROR: USERNAME ALREADY EXISTS";
            }
            else
            {
                showError = false;
                sessionUser.username = regUsername;
                sessionUser.email = regEmail;
                regUsername = "";
                regEmail = "";
                regPassword = "";
                regConfirm = "";
                return AUTH;
            }
        }
    }

    return REG;
}