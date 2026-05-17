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

static bool        showError = false;
static std::string errorMsg = "";
static float       errorAlpha = 0.0f;

static float userErrorLerp = 0.0f;
static float emailErrorLerp = 0.0f;
static float passErrorLerp = 0.0f;
static float confirmErrorLerp = 0.0f;
static bool  userHasError = false;
static bool  emailHasError = false;
static bool  passHasError = false;
static bool  confirmHasError = false;

static float shakeTimer = 0.0f;
static float shakeOffsetX = 0.0f;
static const float SHAKE_DURATION = 0.45f;
static const float SHAKE_MAGNITUDE = 7.0f;

static bool  isLoading = false;
static float loadingTimer = 0.0f;
static bool  wasLoading = false;

static float userBorderLerp = 0.0f;
static float emailBorderLerp = 0.0f;
static float passBorderLerp = 0.0f;
static float confirmBorderLerp = 0.0f;

static float entranceTimer = 0.0f;
static const float ENTER_DURATION = 0.55f;

static float userGlowLerp = 0.0f;
static float emailGlowLerp = 0.0f;
static float passGlowLerp = 0.0f;
static float confirmGlowLerp = 0.0f;

static bool  rippleActive = false;
static float rippleTimer = 0.0f;
static float rippleX = 0.0f;
static float rippleY = 0.0f;
static const float RIPPLE_DURATION = 0.45f;
static const float RIPPLE_MAX_R = 120.0f;

static const int PARTICLE_COUNT = 55;
struct RegParticle { float x, y, vx, vy, r, alpha; };
static RegParticle particles[PARTICLE_COUNT];
static bool        particlesInit = false;

static void InitParticles(int screenW, int screenH)
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x = (float)GetRandomValue(0, screenW);
        particles[i].y = (float)GetRandomValue(0, screenH);
        particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f;
        particles[i].r = (float)GetRandomValue(1, 3);
        particles[i].alpha = (float)GetRandomValue(20, 70) / 255.0f;
    }
    particlesInit = true;
}

static void UpdateParticles(float dt, int screenW, int screenH)
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x += particles[i].vx * dt * 60.0f;
        particles[i].y += particles[i].vy * dt * 60.0f;
        if (particles[i].y < -4)           particles[i].y = (float)screenH + 4;
        if (particles[i].x < -4)           particles[i].x = (float)screenW + 4;
        if (particles[i].x > screenW + 4)  particles[i].x = -4.0f;
    }
}

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static float EaseOutCubic(float t)
{
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static std::string GetMask(const std::string& input)
{
    std::string mask = "";
    for (int i = 0; i < (int)input.length(); i++) mask += "*";
    return mask;
}

static void DrawLoadingDots(Font font, float cx, float cy, float timer)
{
    const char* dots[4] = { "CREATING", "CREATING.", "CREATING..", "CREATING..." };
    int idx = (int)(timer * 3.0f) % 4;
    Vector2 sz = MeasureTextEx(font, dots[idx], 15, 1);
    DrawTextEx(font, dots[idx], { cx - sz.x / 2, cy - sz.y / 2 }, 15, 1, WHITE);
}

static void TriggerError(const std::string& msg,
    bool badUser, bool badEmail, bool badPass, bool badConfirm)
{
    errorMsg = msg;
    showError = true;
    errorAlpha = 0.0f;
    userHasError = badUser;
    emailHasError = badEmail;
    passHasError = badPass;
    confirmHasError = badConfirm;
    if (badUser)    userErrorLerp = 1.0f;
    if (badEmail)   emailErrorLerp = 1.0f;
    if (badPass)    passErrorLerp = 1.0f;
    if (badConfirm) confirmErrorLerp = 1.0f;
    shakeTimer = SHAKE_DURATION;
}

static void HandleInput()
{
    if (isLoading) return;

    if (IsKeyPressed(KEY_TAB))
    {
        if (usernameActive) { usernameActive = false; emailActive = true;  passwordActive = false; confirmActive = false; }
        else if (emailActive) { usernameActive = false; emailActive = false; passwordActive = true;  confirmActive = false; }
        else if (passwordActive) { usernameActive = false; emailActive = false; passwordActive = false; confirmActive = true; }
        else { usernameActive = true;  emailActive = false; passwordActive = false; confirmActive = false; }
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (usernameActive && !regUsername.empty()) regUsername.pop_back();
        if (emailActive && !regEmail.empty())    regEmail.pop_back();
        if (passwordActive && !regPassword.empty()) regPassword.pop_back();
        if (confirmActive && !regConfirm.empty())  regConfirm.pop_back();
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

AppState registerScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();

    if (!particlesInit) InitParticles(screenW, screenH);

    if (entranceTimer < ENTER_DURATION) entranceTimer += dt;
    float enterT = EaseOutCubic(entranceTimer / ENTER_DURATION);
    float panelAlpha = enterT;
    float panelSlideY = (1.0f - enterT) * 60.0f;

    if (shakeTimer > 0.0f)
    {
        shakeTimer -= dt;
        if (shakeTimer < 0.0f) shakeTimer = 0.0f;
        float p = shakeTimer / SHAKE_DURATION;
        shakeOffsetX = sinf(p * 3.14159f * 8.0f) * SHAKE_MAGNITUDE * p;
    }
    else shakeOffsetX = 0.0f;

    if (isLoading) loadingTimer += dt;

    if (rippleActive)
    {
        rippleTimer += dt;
        if (rippleTimer >= RIPPLE_DURATION) rippleActive = false;
    }

    errorAlpha += ((showError ? 1.0f : 0.0f) - errorAlpha) * dt * 12.0f;

    userBorderLerp += ((usernameActive ? 1.0f : 0.0f) - userBorderLerp) * dt * 14.0f;
    emailBorderLerp += ((emailActive ? 1.0f : 0.0f) - emailBorderLerp) * dt * 14.0f;
    passBorderLerp += ((passwordActive ? 1.0f : 0.0f) - passBorderLerp) * dt * 14.0f;
    confirmBorderLerp += ((confirmActive ? 1.0f : 0.0f) - confirmBorderLerp) * dt * 14.0f;

    userGlowLerp += ((usernameActive ? 1.0f : 0.0f) - userGlowLerp) * dt * 10.0f;
    emailGlowLerp += ((emailActive ? 1.0f : 0.0f) - emailGlowLerp) * dt * 10.0f;
    passGlowLerp += ((passwordActive ? 1.0f : 0.0f) - passGlowLerp) * dt * 10.0f;
    confirmGlowLerp += ((confirmActive ? 1.0f : 0.0f) - confirmGlowLerp) * dt * 10.0f;

    if (!userHasError || usernameActive) userErrorLerp += (0.0f - userErrorLerp) * dt * 8.0f;
    if (!emailHasError || emailActive)    emailErrorLerp += (0.0f - emailErrorLerp) * dt * 8.0f;
    if (!passHasError || passwordActive) passErrorLerp += (0.0f - passErrorLerp) * dt * 8.0f;
    if (!confirmHasError || confirmActive)  confirmErrorLerp += (0.0f - confirmErrorLerp) * dt * 8.0f;

    UpdateParticles(dt, screenW, screenH);

    const int ERROR_SLOT = 44;

    int panelW = 420;
    int panelH = 640;
    int panelX = (int)(screenW / 2 - panelW / 2 + shakeOffsetX);
    int panelY = (int)(screenH / 2 - panelH / 2 + panelSlideY);

    int fieldW = panelW - 64;
    int fieldH = 48;
    int fieldX = panelX + 32;

    int errBoxY = panelY + 120;
    int userLabelY = errBoxY + ERROR_SLOT;
    int userFieldY = userLabelY + 18;
    int emailLabelY = userFieldY + fieldH + 20;
    int emailFieldY = emailLabelY + 18;
    int passLabelY = emailFieldY + fieldH + 20;
    int passFieldY = passLabelY + 18;
    int confLabelY = passFieldY + fieldH + 20;
    int confFieldY = confLabelY + 18;
    int signUpBtnY = confFieldY + fieldH + 24;
    int signInLinkY = signUpBtnY + fieldH + 22;

    Rectangle usernameField = { (float)fieldX, (float)userFieldY,  (float)fieldW, (float)fieldH };
    Rectangle emailField = { (float)fieldX, (float)emailFieldY, (float)fieldW, (float)fieldH };
    Rectangle passField = { (float)fieldX, (float)passFieldY,  (float)fieldW, (float)fieldH };
    Rectangle confirmField = { (float)fieldX, (float)confFieldY,  (float)fieldW, (float)fieldH };
    Rectangle signUpBtn = { (float)fieldX, (float)signUpBtnY,  (float)fieldW, (float)fieldH };
    Rectangle eyePassBtn = { (float)(fieldX + fieldW - 38), (float)(passFieldY + 13), 22, 22 };
    Rectangle eyeConfBtn = { (float)(fieldX + fieldW - 38), (float)(confFieldY + 13), 22, 22 };

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    const char* signInPreText = "Already have an account?";
    const char* signInLinkText = "Sign in";
    Vector2 preTextSize = MeasureTextEx(font, signInPreText, 13, 0.5f);
    Vector2 linkTextSize = MeasureTextEx(font, signInLinkText, 13, 0.5f);
    float   signInRowW = preTextSize.x + 6 + linkTextSize.x;
    float   signInStartX = (float)panelX + (panelW - signInRowW) / 2.0f;
    Rectangle signInLink = { signInStartX + preTextSize.x + 6, (float)signInLinkY, linkTextSize.x, linkTextSize.y };
    bool hoverSignIn = CheckCollisionPointRec(mouse, signInLink);

    bool overTextField = CheckCollisionPointRec(mouse, usernameField) ||
        CheckCollisionPointRec(mouse, emailField) ||
        CheckCollisionPointRec(mouse, passField) ||
        CheckCollisionPointRec(mouse, confirmField);
    bool overClickable = CheckCollisionPointRec(mouse, signUpBtn) ||
        CheckCollisionPointRec(mouse, eyePassBtn) ||
        CheckCollisionPointRec(mouse, eyeConfBtn) ||
        hoverSignIn;

    if (overTextField) SetMouseCursor(MOUSE_CURSOR_IBEAM);
    else if (overClickable) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    if (clicked && !isLoading)
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

    bool hoverSignUp = CheckCollisionPointRec(mouse, signUpBtn) && !isLoading;
    bool hoverEyePass = CheckCollisionPointRec(mouse, eyePassBtn);
    bool hoverEyeConf = CheckCollisionPointRec(mouse, eyeConfBtn);

    if (clicked && CheckCollisionPointRec(mouse, signUpBtn) && !isLoading)
    {
        rippleActive = true;
        rippleTimer = 0.0f;
        rippleX = mouse.x;
        rippleY = mouse.y;
    }

    bool tryRegister = (!isLoading && !wasLoading) &&
        ((clicked && CheckCollisionPointRec(mouse, signUpBtn)) || IsKeyPressed(KEY_ENTER));

    if (tryRegister)
    {
        bool emptyUser = regUsername.empty();
        bool emptyEmail = regEmail.empty();
        bool emptyPass = regPassword.empty();
        bool emptyConfirm = regConfirm.empty();

        if (emptyUser || emptyEmail || emptyPass || emptyConfirm)
            TriggerError("ERROR: ALL FIELDS REQUIRED", emptyUser, emptyEmail, emptyPass, emptyConfirm);
        else if (regPassword != regConfirm)
            TriggerError("ERROR: PASSWORDS DO NOT MATCH", false, false, true, true);
        else
        {
            isLoading = true;
            loadingTimer = 0.0f;
        }
    }

    if (isLoading && loadingTimer > 0.05f)
    {
        bool success = AuthService::Register(regUsername, regPassword, regEmail, 1);
        isLoading = false;

        if (success)
        {
            showError = false;
            sessionUser.username = regUsername;
            sessionUser.email = regEmail;
            regUsername = "";
            regEmail = "";
            regPassword = "";
            regConfirm = "";
            wasLoading = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            BeginDrawing(); EndDrawing();
            return AUTH;
        }
        else
        {
            TriggerError("ERROR: USERNAME ALREADY EXISTS", true, false, false, false);
        }
    }

    wasLoading = isLoading;

    BeginDrawing();
    ClearBackground(BG_DARK);

    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)particles[i].x, (int)particles[i].y,
            particles[i].r, Color{ 100, 140, 255, pa });
    }

    for (int r = 280; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 280.0f;
        unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.12f), (int)(screenH * 0.18f), (float)r, Color{ 40, 90, 255, a });
    }
    for (int r = 260; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 260.0f;
        unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.88f), (int)(screenH * 0.82f), (float)r, Color{ 50, 80, 220, a });
    }
    for (int r = 200; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 200.0f;
        unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.85f), (int)(screenH * 0.15f), (float)r, Color{ 80, 50, 200, a });
    }
    for (int r = 180; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 180.0f;
        unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.14f), (int)(screenH * 0.80f), (float)r, Color{ 30, 70, 200, a });
    }
    for (int r = 340; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 340.0f;
        unsigned char a = (unsigned char)(t * t * 22.0f * panelAlpha);
        DrawCircle(screenW / 2, screenH / 2, (float)r, Color{ 55, 95, 210, a });
    }

    DrawRectangle(panelX + 6, panelY + 10, panelW, panelH,
        Color{ 0, 0, 0, (unsigned char)(60 * panelAlpha) });
    DrawRectangleRounded({ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
        0.06f, 10, Color{ BG_CARD.r, BG_CARD.g, BG_CARD.b, PA });
    DrawRectangleRoundedLines({ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
        0.06f, 10, Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });

    float panelCX = panelX + panelW / 2.0f;

    Vector2 logoSize = MeasureTextEx(font, "Gekoya", 22, 1.5f);
    DrawTextEx(font, "Gekoya", { panelCX - logoSize.x / 2, (float)(panelY + 24) },
        22, 1.5f, Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    Vector2 headingSize = MeasureTextEx(font, "Create account", 20, 1.2f);
    DrawTextEx(font, "Create account",
        { panelCX - headingSize.x / 2, (float)(panelY + 70) },
        20, 1.2f, Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    Vector2 subheadingSize = MeasureTextEx(font, "Sign up for a new Gekoya account", 13, 0.5f);
    DrawTextEx(font, "Sign up for a new Gekoya account",
        { panelCX - subheadingSize.x / 2, (float)(panelY + 98) },
        13, 0.5f, Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    {
        unsigned char ea = (unsigned char)(errorAlpha * panelAlpha * 255.0f);
        Rectangle errBox = { (float)fieldX, (float)errBoxY, (float)fieldW, 34 };
        DrawRectangleRec(errBox, Color{ 40, 20, 20, ea });
        DrawRectangleLinesEx(errBox, 1, Color{ 200, 70, 70, ea });
        DrawTextEx(font, errorMsg.c_str(), { errBox.x + 12, errBox.y + 10 },
            11, 1, Color{ 200, 70, 70, ea });
    }

    Color errorCol = Color{ 200, 70, 70, 255 };

    auto DrawField = [&](
        const char* label, Rectangle field,
        const std::string& value, bool active,
        float borderLerp, float glowLerp, float errLerp,
        const char* placeholder, int labelY, int fieldY,
        bool isMasked, bool showMask)
        {
            Color focusCol = LerpColor(BORDER_NORMAL, BORDER_FOCUS, borderLerp);
            Color fieldBorder = LerpColor(focusCol, errorCol, errLerp);
            fieldBorder.a = PA;

            if (glowLerp > 0.01f)
            {
                unsigned char ga = (unsigned char)(glowLerp * 38.0f * panelAlpha);
                DrawRectangleRounded(
                    { field.x - 4, field.y - 4, field.width + 8, field.height + 8 },
                    0.22f, 8, Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, ga });
            }

            DrawTextEx(font, label, { (float)fieldX, (float)labelY }, 11, 1,
                Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
            DrawRectangleRounded(field, 0.18f, 8,
                Color{ BG_INPUT.r, BG_INPUT.g, BG_INPUT.b, PA });
            DrawRectangleRoundedLines(field, 0.18f, 8, fieldBorder);

            if (value.empty() && !active)
                DrawTextEx(font, placeholder, { (float)(fieldX + 14), (float)(fieldY + 16) },
                    13, 1, Color{ TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });
            else
            {
                std::string shown = (isMasked && !showMask) ? GetMask(value) : value;
                DrawTextEx(font, shown.c_str(), { (float)(fieldX + 14), (float)(fieldY + 16) },
                    13, 1, Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });
            }

            if (active && ((int)(GetTime() * 2)) % 2 == 0)
            {
                std::string shown = (isMasked && !showMask) ? GetMask(value) : value;
                float cursorX = fieldX + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
                DrawRectangle((int)cursorX, fieldY + 12, 2, 22,
                    Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA });
            }
        };

    DrawField("USERNAME", usernameField, regUsername, usernameActive,
        userBorderLerp, userGlowLerp, userErrorLerp,
        "ENTER USERNAME", userLabelY, userFieldY, false, false);

    DrawField("EMAIL ADDRESS", emailField, regEmail, emailActive,
        emailBorderLerp, emailGlowLerp, emailErrorLerp,
        "ENTER EMAIL", emailLabelY, emailFieldY, false, false);

    DrawField("PASSWORD", passField, regPassword, passwordActive,
        passBorderLerp, passGlowLerp, passErrorLerp,
        "ENTER PASSWORD", passLabelY, passFieldY, true, showPassword);

    DrawField("CONFIRM PASSWORD", confirmField, regConfirm, confirmActive,
        confirmBorderLerp, confirmGlowLerp, confirmErrorLerp,
        "REPEAT PASSWORD", confLabelY, confFieldY, true, showConfirm);

    auto DrawEye = [&](Rectangle eyeBtn, bool show, bool hover)
        {
            Color eyeCol = hover
                ? Color{ TEXT_PRIMARY.r,   TEXT_PRIMARY.g,   TEXT_PRIMARY.b,   PA }
            : Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA };

            DrawCircleLines((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 9, eyeCol);
            DrawCircle((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 4, eyeCol);
            if (!show)
            {
                DrawLineEx({ eyeBtn.x,     eyeBtn.y + 4 }, { eyeBtn.x + 22, eyeBtn.y + 18 },
                    2.0f, Color{ BG_CARD.r, BG_CARD.g, BG_CARD.b, PA });
                DrawLineEx({ eyeBtn.x + 1, eyeBtn.y + 3 }, { eyeBtn.x + 21, eyeBtn.y + 19 },
                    1.5f, eyeCol);
            }
        };

    DrawEye(eyePassBtn, showPassword, hoverEyePass);
    DrawEye(eyeConfBtn, showConfirm, hoverEyeConf);

    {
        float btnScale = hoverSignUp ? 0.97f : 1.0f;
        float btnW = signUpBtn.width * btnScale;
        float btnH = signUpBtn.height * btnScale;
        float btnX = signUpBtn.x + (signUpBtn.width - btnW) / 2.0f;
        float btnY = signUpBtn.y + (signUpBtn.height - btnH) / 2.0f;
        Rectangle scaledBtn = { btnX, btnY, btnW, btnH };

        if (hoverSignUp)
            DrawRectangleRounded(
                { scaledBtn.x - 4, scaledBtn.y - 4,
                  scaledBtn.width + 8, scaledBtn.height + 8 },
                0.35f, 8, Color{ 72, 130, 255, (unsigned char)(28 * panelAlpha) });

        Color btnColor = (hoverSignUp || isLoading)
            ? Color{ ACCENT_HOVER.r, ACCENT_HOVER.g, ACCENT_HOVER.b, PA }
        : Color{ ACCENT.r,       ACCENT.g,       ACCENT.b,       PA };
        DrawRectangleRounded(scaledBtn, 0.35f, 8, btnColor);

        if (rippleActive)
        {
            float rp = rippleTimer / RIPPLE_DURATION;
            float rad = rp * RIPPLE_MAX_R;
            unsigned char ra = (unsigned char)((1.0f - rp) * 55.0f * panelAlpha);
            DrawCircle((int)rippleX, (int)rippleY, rad, Color{ 255, 255, 255, ra });
        }

        float btnCX = scaledBtn.x + scaledBtn.width / 2.0f;
        float btnCY = scaledBtn.y + scaledBtn.height / 2.0f;
        if (isLoading)
            DrawLoadingDots(font, btnCX, btnCY, loadingTimer);
        else
        {
            Vector2 sz = MeasureTextEx(font, "CREATE ACCOUNT", 15, 1);
            DrawTextEx(font, "CREATE ACCOUNT",
                { btnCX - sz.x / 2, btnCY - sz.y / 2 },
                15, 1, Color{ 255, 255, 255, PA });
        }
    }

    DrawTextEx(font, signInPreText, { signInStartX,                    (float)signInLinkY }, 13, 0.5f,
        Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
    DrawTextEx(font, signInLinkText, { signInStartX + preTextSize.x + 6, (float)signInLinkY }, 13, 0.5f,
        hoverSignIn
        ? Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA }
    : Color{ ACCENT.r,       ACCENT.g,       ACCENT.b,       PA });

    EndDrawing();

    if (clicked && CheckCollisionPointRec(mouse, signInLink) && !isLoading)
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return AUTH;
    }

    return REG;
}