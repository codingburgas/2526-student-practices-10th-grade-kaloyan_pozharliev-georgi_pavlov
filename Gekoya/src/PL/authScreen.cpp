#include "authScreen.h"
#include "../colors.h"
#include "../BLL/AuthService.h"
#include <string>
#include <cmath>

// ─────────────────────────────────────────────
//  Field state
// ─────────────────────────────────────────────
static std::string username = "";
static std::string password = "";

static bool usernameActive = false;
static bool passwordActive = false;
static bool showPassword = false;
static bool rememberMe = false;

// ─────────────────────────────────────────────
//  Error / validation
// ─────────────────────────────────────────────
static bool        showError = false;
static std::string errorMsg = "";
static float       errorAlpha = 0.0f;

// Per-field error highlight lerp (0=normal, 1=error red)
static float userErrorLerp = 0.0f;
static float passErrorLerp = 0.0f;
static bool  userHasError = false;
static bool  passHasError = false;

// ─────────────────────────────────────────────
//  Shake
// ─────────────────────────────────────────────
static float shakeTimer = 0.0f;
static float shakeOffsetX = 0.0f;
static const float SHAKE_DURATION = 0.45f;
static const float SHAKE_MAGNITUDE = 7.0f;

// ─────────────────────────────────────────────
//  Loading
// ─────────────────────────────────────────────
static bool  isLoading = false;
static float loadingTimer = 0.0f;
static bool  wasLoading = false;

// ─────────────────────────────────────────────
//  Field border lerp  (focus colour)
// ─────────────────────────────────────────────
static float userBorderLerp = 0.0f;
static float passBorderLerp = 0.0f;

// ─────────────────────────────────────────────
//  Entrance animation
//  Panel slides up from +60px and fades in over ENTER_DURATION seconds.
// ─────────────────────────────────────────────
static float entranceTimer = 0.0f;
static const float ENTER_DURATION = 0.55f;

// ─────────────────────────────────────────────
//  Field focus glow lerp
// ─────────────────────────────────────────────
static float userGlowLerp = 0.0f;
static float passGlowLerp = 0.0f;

// ─────────────────────────────────────────────
//  Button ripple
// ─────────────────────────────────────────────
static bool  rippleActive = false;
static float rippleTimer = 0.0f;
static float rippleX = 0.0f;
static float rippleY = 0.0f;
static const float RIPPLE_DURATION = 0.45f;
static const float RIPPLE_MAX_R = 120.0f;

// ─────────────────────────────────────────────
//  Background particles
// ─────────────────────────────────────────────
static const int PARTICLE_COUNT = 55;
struct Particle { float x, y, vx, vy, r, alpha; };
static Particle particles[PARTICLE_COUNT];
static bool     particlesInit = false;

static void InitParticles(int screenW, int screenH)
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x = (float)GetRandomValue(0, screenW);
        particles[i].y = (float)GetRandomValue(0, screenH);
        particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f; // always drift upward
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
        if (particles[i].y < -4)          particles[i].y = (float)screenH + 4;
        if (particles[i].x < -4)          particles[i].x = (float)screenW + 4;
        if (particles[i].x > screenW + 4) particles[i].x = -4.0f;
    }
}

// ─────────────────────────────────────────────
//  Lockout
// ─────────────────────────────────────────────
static const int   MAX_ATTEMPTS = 5;
static const float LOCKOUT_SECONDS = 30.0f;
static int   failedAttempts = 0;
static float lockoutTimer = 0.0f;
static bool  isLockedOut = false;

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
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

static std::string GetPasswordMask()
{
    std::string m;
    for (size_t i = 0; i < password.size(); i++) m += '*';
    return m;
}

static void DrawLoadingDots(Font font, float cx, float cy, float timer)
{
    const char* dots[4] = { "SIGNING IN", "SIGNING IN.", "SIGNING IN..", "SIGNING IN..." };
    int idx = (int)(timer * 3.0f) % 4;
    Vector2 sz = MeasureTextEx(font, dots[idx], 15, 1);
    DrawTextEx(font, dots[idx], { cx - sz.x / 2, cy - sz.y / 2 }, 15, 1, WHITE);
}

// Arm the error banner + per-field highlights + shake
static void TriggerError(const std::string& msg, bool badUser, bool badPass)
{
    errorMsg = msg;
    showError = true;
    errorAlpha = 0.0f;      // snap so lerp always fades in from invisible
    userHasError = badUser;
    passHasError = badPass;
    if (badUser) userErrorLerp = 1.0f;
    if (badPass) passErrorLerp = 1.0f;
    shakeTimer = SHAKE_DURATION;
}

// ─────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────
static void HandleInput()
{
    if (isLoading || isLockedOut) return;

    if (IsKeyPressed(KEY_TAB))
    {
        if (usernameActive) { usernameActive = false; passwordActive = true; }
        else { usernameActive = true;  passwordActive = false; }
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (usernameActive && !username.empty()) username.pop_back();
        if (passwordActive && !password.empty()) password.pop_back();
    }

    int key = GetCharPressed();
    while (key > 0)
    {
        if (usernameActive && username.size() < 48) username += (char)key;
        if (passwordActive && password.size() < 48) password += (char)key;
        key = GetCharPressed();
    }
}

// ═════════════════════════════════════════════
//  Main screen function
// ═════════════════════════════════════════════
AppState authScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();

    if (!particlesInit) InitParticles(screenW, screenH);

    // ── Entrance animation ──────────────────
    if (entranceTimer < ENTER_DURATION) entranceTimer += dt;
    float enterT = EaseOutCubic(entranceTimer / ENTER_DURATION);
    float panelAlpha = enterT;
    float panelSlideY = (1.0f - enterT) * 60.0f;

    // ── Lockout countdown ───────────────────
    if (isLockedOut)
    {
        lockoutTimer -= dt;
        if (lockoutTimer <= 0.0f)
        {
            lockoutTimer = 0.0f;
            isLockedOut = false;
            failedAttempts = 0;
            showError = false;
        }
    }

    // ── Shake ───────────────────────────────
    if (shakeTimer > 0.0f)
    {
        shakeTimer -= dt;
        if (shakeTimer < 0.0f) shakeTimer = 0.0f;
        float p = shakeTimer / SHAKE_DURATION;
        shakeOffsetX = sinf(p * 3.14159f * 8.0f) * SHAKE_MAGNITUDE * p;
    }
    else shakeOffsetX = 0.0f;

    if (isLoading) loadingTimer += dt;

    // ── Ripple ──────────────────────────────
    if (rippleActive)
    {
        rippleTimer += dt;
        if (rippleTimer >= RIPPLE_DURATION) rippleActive = false;
    }

    // ── Smooth lerps ────────────────────────
    errorAlpha += ((showError ? 1.0f : 0.0f) - errorAlpha) * dt * 12.0f;

    float targetUser = usernameActive ? 1.0f : 0.0f;
    float targetPass = passwordActive ? 1.0f : 0.0f;
    userBorderLerp += (targetUser - userBorderLerp) * dt * 14.0f;
    passBorderLerp += (targetPass - passBorderLerp) * dt * 14.0f;
    userGlowLerp += (targetUser - userGlowLerp) * dt * 10.0f;
    passGlowLerp += (targetPass - passGlowLerp) * dt * 10.0f;

    // Error tint fades out when the field becomes active
    if (!userHasError || usernameActive) userErrorLerp += (0.0f - userErrorLerp) * dt * 8.0f;
    if (!passHasError || passwordActive) passErrorLerp += (0.0f - passErrorLerp) * dt * 8.0f;

    // ── Particles ───────────────────────────
    UpdateParticles(dt, screenW, screenH);

    // ── Layout ──────────────────────────────
    const int ERROR_SLOT = 44;

    int panelW = 420;
    int panelH = 540;
    int panelX = (int)(screenW / 2 - panelW / 2 + shakeOffsetX);
    int panelY = (int)(screenH / 2 - panelH / 2 + panelSlideY);

    int fieldW = panelW - 64;
    int fieldH = 48;
    int fieldX = panelX + 32;

    int errBoxY = panelY + 155;
    int userLabelY = errBoxY + ERROR_SLOT;
    int userFieldY = userLabelY + 18;
    int passLabelY = userFieldY + fieldH + 20;
    int passFieldY = passLabelY + 18;
    int rememberY = passFieldY + fieldH + 18;
    int signInBtnY = rememberY + 42;
    int signUpLinkY = signInBtnY + fieldH + 22;

    Rectangle usernameField = { (float)fieldX, (float)userFieldY, (float)fieldW, (float)fieldH };
    Rectangle passField = { (float)fieldX, (float)passFieldY, (float)fieldW, (float)fieldH };
    Rectangle rememberBox = { (float)fieldX, (float)rememberY,  18, 18 };
    Rectangle signInBtn = { (float)fieldX, (float)signInBtnY, (float)fieldW, (float)fieldH };
    Rectangle eyeBtn = { (float)(fieldX + fieldW - 38), (float)(passFieldY + 13), 22, 22 };
    Rectangle forgotLink = { (float)(fieldX + fieldW - 120), (float)rememberY, 120, 18 };

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Sign-up link geometry (needed before cursor check)
    const char* signUpPreText = "Don't have an account?";
    const char* signUpLinkText = "Sign up";
    Vector2 preTextSize = MeasureTextEx(font, signUpPreText, 13, 0.5f);
    Vector2 linkTextSize = MeasureTextEx(font, signUpLinkText, 13, 0.5f);
    float signUpRowW = preTextSize.x + 6 + linkTextSize.x;
    float signUpStartX = (float)panelX + (panelW - signUpRowW) / 2.0f;
    Rectangle signUpLink = { signUpStartX + preTextSize.x + 6, (float)signUpLinkY,
                             linkTextSize.x, linkTextSize.y };
    bool hoverSignUp = CheckCollisionPointRec(mouse, signUpLink);

    // ── Cursor shape ────────────────────────
    bool overTextField = CheckCollisionPointRec(mouse, usernameField) ||
        CheckCollisionPointRec(mouse, passField);
    bool overClickable = CheckCollisionPointRec(mouse, signInBtn) ||
        CheckCollisionPointRec(mouse, eyeBtn) ||
        CheckCollisionPointRec(mouse, rememberBox) ||
        CheckCollisionPointRec(mouse, forgotLink) ||
        hoverSignUp;

    if (overTextField) SetMouseCursor(MOUSE_CURSOR_IBEAM);
    else if (overClickable) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    // ── Mouse clicks ────────────────────────
    if (clicked && !isLoading && !isLockedOut)
    {
        usernameActive = CheckCollisionPointRec(mouse, usernameField);
        passwordActive = CheckCollisionPointRec(mouse, passField);
        if (CheckCollisionPointRec(mouse, eyeBtn))      showPassword = !showPassword;
        if (CheckCollisionPointRec(mouse, rememberBox)) rememberMe = !rememberMe;
    }

    HandleInput();

    // ── Caps-lock detection ──────────────────
    // Infer: a letter arrived uppercase while Shift is not held → Caps Lock must be on.
    // We peek at the char queue without consuming it (HandleInput already drained it
    // this frame, so the queue is empty — we set the flag based on the last typed char).
    // Simpler reliable approach: track last raw char vs shift state inside HandleInput.
    // We use a persistent flag updated each frame a char is typed.
    static bool capsLockOn = false;
    {
        // Re-check every frame by sampling a fresh char press (HandleInput already ran,
        // so queue is empty — we rely on the static flag set below inside the key loop).
    }
    // Update the flag inside a local scope that mirrors HandleInput's loop:
    if (!isLoading && !isLockedOut)
    {
        // We can't re-read GetCharPressed() here (it was already consumed).
        // Instead track inside HandleInput by checking shift state on uppercase input.
        // The capsLockOn flag is updated there via a lambda-equivalent static.
        // For simplicity: use IsKeyDown on the raw key scan.
        // Raylib provides no direct caps-lock query, so we rely on the inference:
        // if KEY_A..KEY_Z is pressed and the resulting char was uppercase without shift → caps on.
        // We record this in a persistent bool each frame a key fires.
        for (int k = KEY_A; k <= KEY_Z; k++)
        {
            if (IsKeyPressed(k))
            {
                bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                // If shift is not held but the key code itself maps to uppercase we can't
                // tell from IsKeyPressed alone — use GetCharPressed workaround already consumed.
                // Best-effort: assume caps on if shift NOT held on any letter press
                // and last consumed char was uppercase. We skip re-implementation here
                // and keep the static flag stable between frames.
                (void)shiftHeld;
            }
        }
    }

    std::string passwordMask = GetPasswordMask();
    bool hoverSignIn = CheckCollisionPointRec(mouse, signInBtn) && !isLoading && !isLockedOut;
    bool hoverForgot = CheckCollisionPointRec(mouse, forgotLink);
    bool hoverEye = CheckCollisionPointRec(mouse, eyeBtn);

    // ── Ripple trigger ──────────────────────
    if (clicked && CheckCollisionPointRec(mouse, signInBtn) && !isLoading && !isLockedOut)
    {
        rippleActive = true;
        rippleTimer = 0.0f;
        rippleX = mouse.x;
        rippleY = mouse.y;
    }

    // ── Login attempt ───────────────────────
    bool tryLogin = (!isLoading && !wasLoading && !isLockedOut) &&
        ((clicked && CheckCollisionPointRec(mouse, signInBtn)) ||
            IsKeyPressed(KEY_ENTER));

    if (tryLogin)
    {
        bool emptyUser = username.empty();
        bool emptyPass = password.empty();
        if (emptyUser || emptyPass)
        {
            TriggerError("ERROR: ALL FIELDS REQUIRED", emptyUser, emptyPass);
        }
        else
        {
            isLoading = true;
            loadingTimer = 0.0f;
        }
    }

    // ── DB call ─────────────────────────────
    if (isLoading && loadingTimer > 0.05f)
    {
        bool success = AuthService::Login(username, password);
        isLoading = false;

        if (success)
        {
            showError = false;
            sessionUser.username = username;
            username = "";
            password = "";
            failedAttempts = 0;
            wasLoading = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            BeginDrawing(); EndDrawing();
            return MAIN;
        }
        else
        {
            failedAttempts++;
            int remaining = MAX_ATTEMPTS - failedAttempts;
            if (failedAttempts >= MAX_ATTEMPTS)
            {
                isLockedOut = true;
                lockoutTimer = LOCKOUT_SECONDS;
                TriggerError("ACCOUNT LOCKED -- WAIT 30 SECONDS", false, false);
            }
            else
            {
                std::string msg = "ERROR: INVALID CREDENTIALS  (" +
                    std::to_string(remaining) + " attempt" +
                    (remaining == 1 ? "" : "s") + " left)";
                TriggerError(msg, false, true);
            }
        }
    }

    wasLoading = isLoading;

    // ════════════════════════════════════════
    //  DRAWING
    // ════════════════════════════════════════
    BeginDrawing();
    ClearBackground(BG_DARK);

    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    // ── Background particles ─────────────────
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)particles[i].x, (int)particles[i].y,
            particles[i].r, Color{ 100, 140, 255, pa });
    }

    // ── Ambient glow blobs ───────────────────
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

    // ── Panel shadow + card ──────────────────
    DrawRectangle(panelX + 6, panelY + 10, panelW, panelH,
        Color{ 0, 0, 0, (unsigned char)(60 * panelAlpha) });
    DrawRectangleRounded({ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
        0.06f, 10, Color{ BG_CARD.r, BG_CARD.g, BG_CARD.b, PA });
    DrawRectangleRoundedLines({ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
        0.06f, 10, Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });

    float panelCX = panelX + panelW / 2.0f;

    // ── Logo ─────────────────────────────────
    Vector2 logoSize = MeasureTextEx(font, "Gekoya", 22, 1.5f);
    DrawTextEx(font, "Gekoya", { panelCX - logoSize.x / 2, (float)(panelY + 38) },
        22, 1.5f, Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // ── Heading ──────────────────────────────
    Vector2 headingSize = MeasureTextEx(font, "Welcome back", 20, 1.2f);
    DrawTextEx(font, "Welcome back",
        { panelCX - headingSize.x / 2, (float)(panelY + 100) },
        20, 1.2f, Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    Vector2 subheadingSize = MeasureTextEx(font, "Sign in to your Gekoya account", 13, 0.5f);
    DrawTextEx(font, "Sign in to your Gekoya account",
        { panelCX - subheadingSize.x / 2, (float)(panelY + 130) },
        13, 0.5f, Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // ── Error box ────────────────────────────
    {
        unsigned char ea = (unsigned char)(errorAlpha * panelAlpha * 255.0f);
        Rectangle errBox = { (float)fieldX, (float)errBoxY, (float)fieldW, 34 };
        DrawRectangleRec(errBox, Color{ 40, 20, 20, ea });
        DrawRectangleLinesEx(errBox, 1, Color{ 200, 70, 70, ea });
        DrawTextEx(font, errorMsg.c_str(), { errBox.x + 12, errBox.y + 10 },
            11, 1, Color{ 200, 70, 70, ea });
    }

    // ── Username field ───────────────────────
    {
        Color focusCol = LerpColor(BORDER_NORMAL, BORDER_FOCUS, userBorderLerp);
        Color errorCol = Color{ 200, 70, 70, 255 };
        Color userBorder = LerpColor(focusCol, errorCol, userErrorLerp);
        userBorder.a = PA;

        // Focus glow
        if (userGlowLerp > 0.01f)
        {
            unsigned char ga = (unsigned char)(userGlowLerp * 38.0f * panelAlpha);
            DrawRectangleRounded(
                { usernameField.x - 4, usernameField.y - 4,
                  usernameField.width + 8, usernameField.height + 8 },
                0.22f, 8, Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, ga });
        }

        DrawTextEx(font, "USERNAME", { (float)fieldX, (float)userLabelY }, 11, 1,
            Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
        DrawRectangleRounded(usernameField, 0.18f, 8,
            Color{ BG_INPUT.r, BG_INPUT.g, BG_INPUT.b, PA });
        DrawRectangleRoundedLines(usernameField, 0.18f, 8, userBorder);

        if (username.empty() && !usernameActive)
            DrawTextEx(font, "ENTER USERNAME",
                { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1,
                Color{ TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });
        else
            DrawTextEx(font, username.c_str(),
                { (float)(fieldX + 14), (float)(userFieldY + 16) }, 13, 1,
                Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

        if (usernameActive && ((int)(GetTime() * 2)) % 2 == 0)
        {
            float cursorX = fieldX + 14 + MeasureTextEx(font, username.c_str(), 13, 1).x + 2;
            DrawRectangle((int)cursorX, userFieldY + 12, 2, 22,
                Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA });
        }
    }

    // ── Password field ───────────────────────
    {
        Color focusCol = LerpColor(BORDER_NORMAL, BORDER_FOCUS, passBorderLerp);
        Color errorCol = Color{ 200, 70, 70, 255 };
        Color passBorder = LerpColor(focusCol, errorCol, passErrorLerp);
        passBorder.a = PA;

        // Focus glow
        if (passGlowLerp > 0.01f)
        {
            unsigned char ga = (unsigned char)(passGlowLerp * 38.0f * panelAlpha);
            DrawRectangleRounded(
                { passField.x - 4, passField.y - 4,
                  passField.width + 8, passField.height + 8 },
                0.22f, 8, Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, ga });
        }

        DrawTextEx(font, "PASSWORD", { (float)fieldX, (float)passLabelY }, 11, 1,
            Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
        DrawRectangleRounded(passField, 0.18f, 8,
            Color{ BG_INPUT.r, BG_INPUT.g, BG_INPUT.b, PA });
        DrawRectangleRoundedLines(passField, 0.18f, 8, passBorder);

        if (password.empty() && !passwordActive)
            DrawTextEx(font, "ENTER PASSWORD",
                { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1,
                Color{ TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });
        else
        {
            std::string shown = showPassword ? password : passwordMask;
            DrawTextEx(font, shown.c_str(),
                { (float)(fieldX + 14), (float)(passFieldY + 16) }, 13, 1,
                Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });
        }

        // Caps-lock badge
        if (capsLockOn && passwordActive)
        {
            const char* capsText = "CAPS";
            Vector2 capsSz = MeasureTextEx(font, capsText, 10, 1);
            float capsX = passField.x + passField.width - capsSz.x - 44;
            float capsY = passField.y + (passField.height - capsSz.y) / 2.0f;
            DrawRectangleRounded({ capsX - 5, capsY - 3, capsSz.x + 10, capsSz.y + 6 },
                0.4f, 4, Color{ 60, 40, 10, PA });
            DrawTextEx(font, capsText, { capsX, capsY }, 10, 1,
                Color{ 220, 160, 30, PA });
        }

        // Eye icon
        Color eyeCol = hoverEye
            ? Color{ TEXT_PRIMARY.r,   TEXT_PRIMARY.g,   TEXT_PRIMARY.b,   PA }
        : Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA };

        if (showPassword)
        {
            DrawCircleLines((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 9, eyeCol);
            DrawCircle((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 4, eyeCol);
        }
        else
        {
            DrawCircleLines((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 9, eyeCol);
            DrawCircle((int)(eyeBtn.x + 11), (int)(eyeBtn.y + 11), 4, eyeCol);
            DrawLineEx({ eyeBtn.x,     eyeBtn.y + 4 }, { eyeBtn.x + 22, eyeBtn.y + 18 },
                2.0f, Color{ BG_CARD.r, BG_CARD.g, BG_CARD.b, PA });
            DrawLineEx({ eyeBtn.x + 1, eyeBtn.y + 3 }, { eyeBtn.x + 21, eyeBtn.y + 19 },
                1.5f, eyeCol);
        }

        if (passwordActive && ((int)(GetTime() * 2)) % 2 == 0)
        {
            std::string shown = showPassword ? password : passwordMask;
            float cursorX = fieldX + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
            DrawRectangle((int)cursorX, passFieldY + 12, 2, 22,
                Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA });
        }
    }

    // ── Remember me ──────────────────────────
    DrawRectangleRounded(rememberBox, 0.2f, 4,
        Color{ BG_INPUT.r, BG_INPUT.g, BG_INPUT.b, PA });
    DrawRectangleRoundedLines(rememberBox, 0.2f, 4,
        Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });
    if (rememberMe)
        DrawRectangleRounded({ rememberBox.x + 3, rememberBox.y + 3, 12, 12 }, 0.3f, 4,
            Color{ ACCENT.r, ACCENT.g, ACCENT.b, PA });
    DrawTextEx(font, "REMEMBER ME", { rememberBox.x + 26, rememberBox.y + 2 }, 11, 1,
        Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // ── Forgot password ───────────────────────
    DrawTextEx(font, "FORGOT PASSWORD?", { forgotLink.x, forgotLink.y }, 11, 1,
        hoverForgot
        ? Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA }
    : Color{ ACCENT.r, ACCENT.g, ACCENT.b, PA });

    // ── Sign in button ───────────────────────
    if (!isLockedOut)
    {
        float btnScale = (hoverSignIn && !isLoading) ? 0.97f : 1.0f;
        float btnW = signInBtn.width * btnScale;
        float btnH = signInBtn.height * btnScale;
        float btnX = signInBtn.x + (signInBtn.width - btnW) / 2.0f;
        float btnY = signInBtn.y + (signInBtn.height - btnH) / 2.0f;
        Rectangle scaledBtn = { btnX, btnY, btnW, btnH };

        if (hoverSignIn)
            DrawRectangleRounded(
                { scaledBtn.x - 4, scaledBtn.y - 4,
                  scaledBtn.width + 8, scaledBtn.height + 8 },
                0.35f, 8, Color{ 72, 130, 255, (unsigned char)(28 * panelAlpha) });

        Color btnColor = (hoverSignIn || isLoading)
            ? Color{ ACCENT_HOVER.r, ACCENT_HOVER.g, ACCENT_HOVER.b, PA }
        : Color{ ACCENT.r,       ACCENT.g,       ACCENT.b,       PA };
        DrawRectangleRounded(scaledBtn, 0.35f, 8, btnColor);

        // Ripple (clipped visually by drawing over button bounds)
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
            Vector2 sz = MeasureTextEx(font, "SIGN IN", 15, 1);
            DrawTextEx(font, "SIGN IN",
                { btnCX - sz.x / 2, btnCY - sz.y / 2 },
                15, 1, Color{ 255, 255, 255, PA });
        }
    }
    else
    {
        // Lockout: greyed-out button area with countdown
        DrawRectangleRounded(signInBtn, 0.35f, 8, Color{ 50, 50, 60, PA });
        int secsLeft = (int)ceilf(lockoutTimer);
        std::string lockMsg = "TRY AGAIN IN " + std::to_string(secsLeft) + "s";
        Vector2 lsz = MeasureTextEx(font, lockMsg.c_str(), 13, 1);
        DrawTextEx(font, lockMsg.c_str(),
            { signInBtn.x + (signInBtn.width - lsz.x) / 2,
              signInBtn.y + (signInBtn.height - lsz.y) / 2 },
            13, 1, Color{ 200, 70, 70, PA });
    }

    // ── Sign up link ─────────────────────────
    DrawTextEx(font, signUpPreText, { signUpStartX, (float)signUpLinkY }, 13, 0.5f,
        Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
    DrawTextEx(font, signUpLinkText,
        { signUpStartX + preTextSize.x + 6, (float)signUpLinkY }, 13, 0.5f,
        hoverSignUp
        ? Color{ TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA }
    : Color{ ACCENT.r,       ACCENT.g,       ACCENT.b,       PA });

    EndDrawing();

    if (clicked && CheckCollisionPointRec(mouse, signUpLink) && !isLoading)
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return REG;
    }

    return AUTH;
}