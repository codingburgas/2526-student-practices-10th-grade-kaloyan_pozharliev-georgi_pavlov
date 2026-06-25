#include "profileScreen.h"
#include <iostream>
#include "../colors.h"
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

// ─── Nav ────────────────────────────────────────────────────────────────────
static const char* prof_navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int         prof_activeNav = 3;

// ─── Entrance animation ──────────────────────────────────────────────────────
static float prof_entranceTimer = 0.0f;
static const float PROF_ENTER_DURATION = 0.55f;
static float Prof_EaseOutCubic(float t) { float inv = 1.0f - t; return 1.0f - inv * inv * inv; }

// ─── Particles ───────────────────────────────────────────────────────────────
static const int PROF_PARTICLE_COUNT = 55;
struct ProfParticle { float x, y, vx, vy, r, alpha; };
static ProfParticle prof_particles[PROF_PARTICLE_COUNT];
static bool prof_particlesInit = false;

static void Prof_InitParticles(int w, int h)
{
    for (int i = 0; i < PROF_PARTICLE_COUNT; i++)
    {
        prof_particles[i].x = (float)GetRandomValue(0, w);
        prof_particles[i].y = (float)GetRandomValue(0, h);
        prof_particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        prof_particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f;
        prof_particles[i].r = (float)GetRandomValue(1, 3);
        prof_particles[i].alpha = (float)GetRandomValue(20, 70) / 255.0f;
    }
    prof_particlesInit = true;
}


static void Prof_UpdateParticles(float dt, int w, int h)
{
    for (int i = 0; i < PROF_PARTICLE_COUNT; i++)
    {
        prof_particles[i].x += prof_particles[i].vx * dt * 60.0f;
        prof_particles[i].y += prof_particles[i].vy * dt * 60.0f;
        if (prof_particles[i].y < -4)    prof_particles[i].y = (float)h + 4;
        if (prof_particles[i].x < -4)    prof_particles[i].x = (float)w + 4;
        if (prof_particles[i].x > w + 4) prof_particles[i].x = -4.0f;
    }
}

// ─── Toast ───────────────────────────────────────────────────────────────────
static std::string prof_toastMsg = "";
static float       prof_toastTimer = 0.0f;
static const float PROF_TOAST_DURATION = 2.8f;

static void Prof_ShowToast(const std::string& msg)
{
    prof_toastMsg = msg;
    prof_toastTimer = PROF_TOAST_DURATION;
}

// ─── Ripple ───────────────────────────────────────────────────────────────────
static bool  prof_rippleActive = false;
static float prof_rippleTimer = 0.0f;
static float prof_rippleX = 0, prof_rippleY = 0;
static const float PROF_RIPPLE_DURATION = 0.5f;
static const float PROF_RIPPLE_MAX_R = 140.0f;

// ─── Color helpers ────────────────────────────────────────────────────────────
static Color Prof_LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

// ─── Edit-field state ─────────────────────────────────────────────────────────
static std::string prof_editEmail = "";
static std::string prof_editUsername = "";
static bool        prof_editMode = false;

static bool  prof_emailActive = false;
static bool  prof_usernameActive = false;

static float prof_emailBorderLerp = 0.0f;
static float prof_usernameBorderLerp = 0.0f;
static float prof_emailGlowLerp = 0.0f;
static float prof_usernameGlowLerp = 0.0f;

// ─── Password change state ────────────────────────────────────────────────────
static bool        prof_pwSection = false;   // password section expanded
static std::string prof_pwCurrent = "";
static std::string prof_pwNew = "";
static std::string prof_pwConfirm = "";
static bool        prof_pwCurrActive = false;
static bool        prof_pwNewActive = false;
static bool        prof_pwConfActive = false;
static bool        prof_showCurr = false;
static bool        prof_showNew = false;
static bool        prof_showConf = false;

static float prof_pwSectionHeight = 0.0f;    // animated expand
static const float PROF_PW_EXPANDED = 200.0f;

// ─── Stats data ───────────────────────────────────────────────────────────────
struct ProfStat { const char* label; const char* value; Color accent; };
static ProfStat prof_stats[] = {
    { "FILMS BOOKED",   "12",    {80,  130, 255, 255} },
    { "HOURS WATCHED",  "34.5h", {255, 140,  60, 255} },
    { "FAVOURITE GENRE","SCI-FI",{180,  80, 255, 255} },
    { "MEMBER SINCE",   "2024",  { 80, 220, 160, 255} },
};

// ─── Recent activity ──────────────────────────────────────────────────────────
struct ProfActivity { const char* title; const char* date; const char* seat; Color accent; };
static ProfActivity prof_activity[] = {
    { "DUNE: PART TWO",  "Mar 15, 2025", "GOLD   20:15",  {80,  130, 255, 255} },
    { "OPPENHEIMER",     "Feb 28, 2025", "PLATINUM 18:00",{255, 140,  60, 255} },
    { "THE BATMAN",      "Jan 10, 2025", "SILVER  13:00",  {60,  180, 255, 255} },
    { "INTERSTELLAR",    "Dec 05, 2024", "GOLD   14:30",  {100, 200, 255, 255} },
};

// ─── Error / validation ───────────────────────────────────────────────────────
static bool        prof_showError = false;
static std::string prof_errorMsg = "";
static float       prof_errorAlpha = 0.0f;

static void Prof_TriggerError(const std::string& msg)
{
    prof_errorMsg = msg;
    prof_showError = true;
    prof_errorAlpha = 0.0f;
}

// ─── Avatar pulse ─────────────────────────────────────────────────────────────
static float prof_avatarPulse = 0.0f;

// ─── Shake ────────────────────────────────────────────────────────────────────
static float prof_shakeTimer = 0.0f;
static float prof_shakeOffsetX = 0.0f;
static const float PROF_SHAKE_DURATION = 0.45f;
static const float PROF_SHAKE_MAGNITUDE = 6.0f;

// ─── Helper: draw a masked password string ────────────────────────────────────
static std::string Prof_Mask(const std::string& s)
{
    std::string m; for (auto& c : s) { (void)c; m += '*'; } return m;
}

// ─── Helper: draw eye toggle icon ─────────────────────────────────────────────
static void Prof_DrawEye(bool visible, Rectangle btn, Color col, unsigned char PA)
{
    Color c = { col.r, col.g, col.b, PA };
    DrawCircleLines((int)(btn.x + 11), (int)(btn.y + 11), 9, c);
    DrawCircle((int)(btn.x + 11), (int)(btn.y + 11), 4, c);
    if (!visible)
    {
        DrawLineEx({ btn.x,      btn.y + 4 }, { btn.x + 22, btn.y + 18 }, 2.0f,
            Color{ BG_CARD.r, BG_CARD.g, BG_CARD.b, PA });
        DrawLineEx({ btn.x + 1,  btn.y + 3 }, { btn.x + 21, btn.y + 19 }, 1.5f, c);
    }
}

// ─── Helper: draw an input field row ──────────────────────────────────────────
static void Prof_DrawField(Font font, const char* label,
    const std::string& value, bool active, bool masked,
    Rectangle field, float borderLerp, float glowLerp,
    unsigned char PA, float time)
{
    // Glow
    if (glowLerp > 0.01f)
    {
        unsigned char ga = (unsigned char)(glowLerp * 38.0f * (PA / 255.0f));
        DrawRectangleRounded(
            { field.x - 4, field.y - 4, field.width + 8, field.height + 8 },
            0.22f, 8, { BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, ga });
    }

    Color border = Prof_LerpColor(BORDER_NORMAL, BORDER_FOCUS, borderLerp);
    border.a = PA;

    // Label
    DrawTextEx(font, label, { field.x, field.y - 18 }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    DrawRectangleRounded(field, 0.18f, 8, { BG_INPUT.r, BG_INPUT.g, BG_INPUT.b, PA });
    DrawRectangleRoundedLines(field, 0.18f, 8, border);

    std::string shown = masked ? Prof_Mask(value) : value;
    if (value.empty() && !active)
    {
        std::string ph = std::string("ENTER ") + label;
        DrawTextEx(font, ph.c_str(), { field.x + 14, field.y + 15 }, 13, 1,
            { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });
    }
    else
        DrawTextEx(font, shown.c_str(), { field.x + 14, field.y + 15 }, 13, 1,
            { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // Cursor
    if (active && ((int)(time * 2)) % 2 == 0)
    {
        float cx = field.x + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cx, (int)(field.y + 10), 2, 22,
            { BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA });
    }
}

// ════════════════════════════════════════════════════════════════════════════
AppState profileScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    if (!prof_particlesInit) Prof_InitParticles(screenW, screenH);

    // Entrance
    if (prof_entranceTimer < PROF_ENTER_DURATION) prof_entranceTimer += dt;
    float enterT = Prof_EaseOutCubic(prof_entranceTimer / PROF_ENTER_DURATION);
    float panelAlpha = enterT;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    Prof_UpdateParticles(dt, screenW, screenH);

    // Toast / ripple
    if (prof_toastTimer > 0) prof_toastTimer -= dt;
    if (prof_rippleActive)
    {
        prof_rippleTimer += dt;
        if (prof_rippleTimer >= PROF_RIPPLE_DURATION) prof_rippleActive = false;
    }

    // Error fade
    prof_errorAlpha += ((prof_showError ? 1.0f : 0.0f) - prof_errorAlpha) * dt * 12.0f;

    // Shake
    if (prof_shakeTimer > 0.0f)
    {
        prof_shakeTimer -= dt;
        if (prof_shakeTimer < 0.0f) prof_shakeTimer = 0.0f;
        float p = prof_shakeTimer / PROF_SHAKE_DURATION;
        prof_shakeOffsetX = sinf(p * 3.14159f * 8.0f) * PROF_SHAKE_MAGNITUDE * p;
    }
    else prof_shakeOffsetX = 0.0f;

    // Avatar pulse
    prof_avatarPulse += dt;

    // Password section expand/collapse
    float pwTarget = prof_pwSection ? PROF_PW_EXPANDED : 0.0f;
    prof_pwSectionHeight += (pwTarget - prof_pwSectionHeight) * dt * 14.0f;

    // Border lerp
    prof_emailBorderLerp += ((prof_emailActive ? 1.0f : 0.0f) - prof_emailBorderLerp) * dt * 14.0f;
    prof_usernameBorderLerp += ((prof_usernameActive ? 1.0f : 0.0f) - prof_usernameBorderLerp) * dt * 14.0f;
    prof_emailGlowLerp += ((prof_emailActive ? 1.0f : 0.0f) - prof_emailGlowLerp) * dt * 10.0f;
    prof_usernameGlowLerp += ((prof_usernameActive ? 1.0f : 0.0f) - prof_usernameGlowLerp) * dt * 10.0f;

    // Init edit buffers from session when entering edit mode
    if (prof_editMode && prof_editEmail.empty() && prof_editUsername.empty())
    {
        prof_editEmail = sessionUser.email;
        prof_editUsername = sessionUser.username;
    }

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // ── Keyboard input ───────────────────────────────────────────────────────
    if (prof_editMode)
    {
        if (IsKeyPressed(KEY_TAB))
        {
            if (prof_usernameActive) { prof_usernameActive = false; prof_emailActive = true; }
            else { prof_emailActive = false;    prof_usernameActive = true; }
        }
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            if (prof_usernameActive && !prof_editUsername.empty()) prof_editUsername.pop_back();
            if (prof_emailActive && !prof_editEmail.empty())    prof_editEmail.pop_back();
        }
        int k = GetCharPressed();
        while (k > 0)
        {
            if (prof_usernameActive && prof_editUsername.size() < 48) prof_editUsername += (char)k;
            if (prof_emailActive && prof_editEmail.size() < 64) prof_editEmail += (char)k;
            k = GetCharPressed();
        }
    }

    if (prof_pwSection)
    {
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            if (prof_pwCurrActive && !prof_pwCurrent.empty()) prof_pwCurrent.pop_back();
            if (prof_pwNewActive && !prof_pwNew.empty())     prof_pwNew.pop_back();
            if (prof_pwConfActive && !prof_pwConfirm.empty()) prof_pwConfirm.pop_back();
        }
        int k = GetCharPressed();
        while (k > 0)
        {
            if (prof_pwCurrActive && prof_pwCurrent.size() < 48) prof_pwCurrent += (char)k;
            if (prof_pwNewActive && prof_pwNew.size() < 48) prof_pwNew += (char)k;
            if (prof_pwConfActive && prof_pwConfirm.size() < 48) prof_pwConfirm += (char)k;
            k = GetCharPressed();
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  LAYOUT CONSTANTS
    // ─────────────────────────────────────────────────────────────────────────
    int navH = 64;

    // ── Nav click ────────────────────────────────────────────────────────────
    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        Rectangle navRect = { navX, 0, 130, (float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
        {
            prof_activeNav = i;
            prof_entranceTimer = 0.0f;
            switch (i)
            {
            case 0: return MAIN;
            case 1: return CINEMAS;
            case 2: return TICKETS;
            case 3: return PROFILE;
            default: break;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  CONTENT LAYOUT
    // ─────────────────────────────────────────────────────────────────────────
    int contentY = navH + 18;
    int contentH = screenH - navH - 34 - 18;  // 34 = status bar
    int margin = 36;

    // Left column  ─ avatar + stats + activity
    int leftW = 310;
    int leftX = margin;

    // Right column ─ profile form
    int rightX = leftX + leftW + 24;
    int rightW = screenW - rightX - margin;

    // ─ Avatar card ──────────────────────────────────────────────────────────
    int avatarCardH = 180;
    Rectangle avatarCard = { (float)leftX, (float)contentY, (float)leftW, (float)avatarCardH };

    // ─ Stats card ───────────────────────────────────────────────────────────
    int statsCardY = contentY + avatarCardH + 14;
    int statsCardH = 130;
    Rectangle statsCard = { (float)leftX, (float)statsCardY, (float)leftW, (float)statsCardH };

    // ─ Activity card ─────────────────────────────────────────────────────────
    int actCardY = statsCardY + statsCardH + 14;
    int actCardH = contentY + contentH - actCardY;
    Rectangle actCard = { (float)leftX, (float)actCardY, (float)leftW, (float)actCardH };

    // ─ Profile form card ─────────────────────────────────────────────────────
    int formCardH = contentH;
    float formCardX = (float)(rightX + prof_shakeOffsetX);
    Rectangle formCard = { formCardX, (float)contentY, (float)rightW, (float)formCardH };

    // Field geometry (inside form card)
    int fPad = 32;
    int fW = rightW - fPad * 2;
    int fH = 48;
    int fX = (int)formCardX + fPad;

    // Row Y positions inside form card
    int rowBase = contentY + 56;
    int userFieldY = rowBase;
    int emailFieldY = userFieldY + fH + 38;
    int saveBtnY = emailFieldY + fH + 28;
    int dividerY = saveBtnY + fH + 22;

    // Password section toggle
    int pwToggleY = dividerY + 12;
    int pwBaseY = pwToggleY + 36;  // fields start here if expanded

    Rectangle usernameField = { (float)fX, (float)userFieldY,  (float)fW, (float)fH };
    Rectangle emailField = { (float)fX, (float)emailFieldY, (float)fW, (float)fH };
    Rectangle saveBtn = { (float)fX, (float)saveBtnY,    (float)fW, (float)fH };

    // Password field rects (rendered only when expanded)
    Rectangle pwCurrField = { (float)fX, (float)(pwBaseY + 26),             (float)fW, (float)fH };
    Rectangle pwNewField = { (float)fX, (float)(pwBaseY + 26 + fH + 34),   (float)fW, (float)fH };
    Rectangle pwConfField = { (float)fX, (float)(pwBaseY + 26 + (fH + 34) * 2), (float)fW, (float)fH };
    Rectangle pwUpdateBtn = { (float)fX, (float)(pwBaseY + 26 + (fH + 34) * 3 + 8), (float)fW, (float)fH };

    // Eye buttons for password fields
    Rectangle eyeCurr = { pwCurrField.x + pwCurrField.width - 38, pwCurrField.y + 13, 22, 22 };
    Rectangle eyeNew = { pwNewField.x + pwNewField.width - 38, pwNewField.y + 13, 22, 22 };
    Rectangle eyeConf = { pwConfField.x + pwConfField.width - 38, pwConfField.y + 13, 22, 22 };

    // Password toggle button
    Rectangle pwToggleBtn = { (float)fX, (float)pwToggleY, (float)fW, 30 };

    // ── Logout button ─────────────────────────────────────────────────────────
    Rectangle logoutBtn = { (float)(screenW - 105), (float)(navH / 2 - 14), 88, 28 };
    bool hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);

    // ── Click handling ────────────────────────────────────────────────────────
    if (clicked)
    {
        // Field focus (only when in edit mode)
        if (prof_editMode)
        {
            prof_usernameActive = CheckCollisionPointRec(mouse, usernameField);
            prof_emailActive = CheckCollisionPointRec(mouse, emailField);
        }

        // Password field focus
        if (prof_pwSection && prof_pwSectionHeight > 10.0f)
        {
            prof_pwCurrActive = CheckCollisionPointRec(mouse, pwCurrField);
            prof_pwNewActive = CheckCollisionPointRec(mouse, pwNewField);
            prof_pwConfActive = CheckCollisionPointRec(mouse, pwConfField);

            if (CheckCollisionPointRec(mouse, eyeCurr)) prof_showCurr = !prof_showCurr;
            if (CheckCollisionPointRec(mouse, eyeNew))  prof_showNew = !prof_showNew;
            if (CheckCollisionPointRec(mouse, eyeConf)) prof_showConf = !prof_showConf;
        }

        // Edit / save toggle
        if (CheckCollisionPointRec(mouse, saveBtn))
        {
            if (prof_editMode)
            {
                // Validate
                if (prof_editUsername.empty() || prof_editEmail.empty())
                {
                    Prof_TriggerError("ERROR: ALL FIELDS REQUIRED");
                    prof_shakeTimer = PROF_SHAKE_DURATION;
                }
                else
                {
                    sessionUser.username = prof_editUsername;
                    sessionUser.email = prof_editEmail;
                    prof_editMode = false;
                    prof_usernameActive = false;
                    prof_emailActive = false;
                    prof_showError = false;
                    prof_rippleActive = true;
                    prof_rippleTimer = 0.0f;
                    prof_rippleX = mouse.x;
                    prof_rippleY = mouse.y;
                    Prof_ShowToast("PROFILE UPDATED SUCCESSFULLY");
                }
            }
            else
            {
                prof_editMode = true;
                prof_editEmail = sessionUser.email;
                prof_editUsername = sessionUser.username;
                prof_usernameActive = true;
                prof_showError = false;
            }
        }

        // Password section toggle
        if (CheckCollisionPointRec(mouse, pwToggleBtn))
        {
            prof_pwSection = !prof_pwSection;
            prof_pwCurrActive = false;
            prof_pwNewActive = false;
            prof_pwConfActive = false;
        }

        // Update password button
        if (prof_pwSection && prof_pwSectionHeight > 10.0f &&
            CheckCollisionPointRec(mouse, pwUpdateBtn))
        {
            if (prof_pwCurrent.empty() || prof_pwNew.empty() || prof_pwConfirm.empty())
            {
                Prof_TriggerError("ERROR: ALL PASSWORD FIELDS REQUIRED");
                prof_shakeTimer = PROF_SHAKE_DURATION;
            }
            else if (prof_pwNew != prof_pwConfirm)
            {
                Prof_TriggerError("ERROR: NEW PASSWORDS DO NOT MATCH");
                prof_shakeTimer = PROF_SHAKE_DURATION;
            }
            else if (prof_pwNew.size() < 6)
            {
                Prof_TriggerError("ERROR: PASSWORD MUST BE AT LEAST 6 CHARACTERS");
                prof_shakeTimer = PROF_SHAKE_DURATION;
            }
            else
            {
                prof_pwCurrent = ""; prof_pwNew = ""; prof_pwConfirm = "";
                prof_pwSection = false;
                prof_showError = false;
                Prof_ShowToast("PASSWORD UPDATED SUCCESSFULLY");
            }
        }

        // Logout
        if (hoverLogout)
        {
            prof_editMode = false;
            prof_pwSection = false;
            prof_pwCurrent = "";
            prof_pwNew = "";
            prof_pwConfirm = "";
            prof_editEmail = "";
            prof_editUsername = "";
            prof_showError = false;
            prof_entranceTimer = 0.0f;
            prof_particlesInit = false;
            sessionUser.username = "";
            sessionUser.email = "";
            return AUTH;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  DRAWING
    // ─────────────────────────────────────────────────────────────────────────
    BeginDrawing();
    ClearBackground(BG_DARK);

    // ── Particles ─────────────────────────────────────────────────────────────
    for (int i = 0; i < PROF_PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(prof_particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)prof_particles[i].x, (int)prof_particles[i].y,
            prof_particles[i].r, { 100, 140, 255, pa });
    }

    // ── Ambient glow blobs (identical to main/auth) ────────────────────────────
    for (int r = 280; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 280.0f;
        unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.12f), (int)(screenH * 0.18f), (float)r, { 40, 90, 255, a });
    }
    for (int r = 260; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 260.0f;
        unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.88f), (int)(screenH * 0.82f), (float)r, { 50, 80, 220, a });
    }
    for (int r = 200; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 200.0f;
        unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.85f), (int)(screenH * 0.15f), (float)r, { 80, 50, 200, a });
    }
    for (int r = 180; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 180.0f;
        unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f) * panelAlpha);
        DrawCircle((int)(screenW * 0.14f), (int)(screenH * 0.80f), (float)r, { 30, 70, 200, a });
    }
    for (int r = 340; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 340.0f;
        unsigned char a = (unsigned char)(t * t * 22.0f * panelAlpha);
        DrawCircle(screenW / 2, screenH / 2, (float)r, { 55, 95, 210, a });
    }

    // Scanlines
    for (int sy = 0; sy < screenH; sy += 4)
        DrawRectangle(0, sy, screenW, 1, { 0, 0, 0, 12 });

    // ─────────────────────────────────────────────────────────────────────────
    //  NAVIGATION BAR  (drawn twice like mainScreen — keeps the look identical)
    // ─────────────────────────────────────────────────────────────────────────
    DrawRectangle(0, 0, screenW, navH, { 10, 12, 28, 220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "Gekoya", { 32, (float)(navH / 2) - 11 }, 22, 1.5f,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        float navY = (float)(navH / 2) - 7;
        bool  isA = (prof_activeNav == i);
        DrawTextEx(font, prof_navItems[i], { navX, navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,   TEXT_PRIMARY.g,   TEXT_PRIMARY.b,   PA }
        : Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
        if (isA) DrawRectangle((int)navX, navH - 2,
            (int)MeasureTextEx(font, prof_navItems[i], 12, 1).x, 2, ACCENT);
    }

    // Greeting badge
    std::string greeting = "HI, " + sessionUser.username;
    Vector2 greetSz = MeasureTextEx(font, greeting.c_str(), 13, 1);
    DrawRectangleRounded(
        { (float)(screenW - 220), (float)(navH / 2 - 14), greetSz.x + 20, 28 },
        0.3f, 6, { 30, 40, 70, 180 });
    DrawTextEx(font, greeting.c_str(),
        { (float)(screenW - 210), (float)(navH / 2 - 7) }, 13, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // Logout button
    DrawRectangleRounded(logoutBtn, 0.3f, 6,
        hoverLogout ? Color{ 180, 50, 50, 220 } : Color{ 80, 30, 30, 180 });
    DrawRectangleRoundedLines(logoutBtn, 0.3f, 6,
        hoverLogout ? Color{ 220, 80, 80, 255 } : Color{ 140, 50, 50, 200 });
    Vector2 loSz = MeasureTextEx(font, "LOG OUT", 11, 1);
    DrawTextEx(font, "LOG OUT",
        { logoutBtn.x + logoutBtn.width / 2 - loSz.x / 2,
          logoutBtn.y + logoutBtn.height / 2 - loSz.y / 2 },
        11, 1, hoverLogout ? WHITE : Color{ 200, 100, 100, 255 });

    // ─────────────────────────────────────────────────────────────────────────
    //  LEFT COLUMN
    // ─────────────────────────────────────────────────────────────────────────

    // ── Avatar card ───────────────────────────────────────────────────────────
    DrawRectangle((int)avatarCard.x + 4, (int)avatarCard.y + 6,
        (int)avatarCard.width, (int)avatarCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(avatarCard, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines(avatarCard, 0.08f, 8, BORDER_NORMAL);
    // Accent top bar
    DrawRectangleRounded({ avatarCard.x, avatarCard.y, avatarCard.width, 5 }, 0.5f, 4,
        { 80, 130, 255, PA });

    // Avatar circle with animated ring
    float avCX = avatarCard.x + 70;
    float avCY = avatarCard.y + avatarCardH / 2.0f;
    float avR = 44.0f;

    // Pulsing outer ring
    float ringPulse = (sinf(prof_avatarPulse * 1.2f) + 1.0f) / 2.0f;
    unsigned char ringA = (unsigned char)((0.3f + ringPulse * 0.4f) * PA);
    DrawCircle((int)avCX, (int)avCY, (int)(avR + 8), { 80, 130, 255, ringA });
    DrawCircle((int)avCX, (int)avCY, (int)(avR + 5), { 20, 25, 48, PA });

    // Avatar fill
    DrawCircle((int)avCX, (int)avCY, (int)avR, { 25, 30, 60, PA });
    DrawCircleLines((int)avCX, (int)avCY, avR, { 80, 130, 255, PA });

    // Initials inside avatar
    std::string initials = "";
    if (!sessionUser.username.empty()) initials += (char)toupper(sessionUser.username[0]);
    if (sessionUser.username.size() > 1) initials += (char)toupper(sessionUser.username[1]);
    Vector2 initSz = MeasureTextEx(font, initials.c_str(), 22, 1);
    DrawTextEx(font, initials.c_str(),
        { avCX - initSz.x / 2, avCY - initSz.y / 2 }, 22, 1,
        { 80, 130, 255, PA });

    // Username + role badge
    int infoX = (int)avCX + (int)avR + 18;
    int infoY = (int)avCY - 28;
    DrawTextEx(font, sessionUser.username.c_str(), { (float)infoX, (float)infoY }, 16, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    const char* roleTxt = "MEMBER";
    Vector2 roleSz = MeasureTextEx(font, roleTxt, 10, 1);
    DrawRectangleRounded({ (float)infoX, (float)(infoY + 24), roleSz.x + 14, 18 },
        0.4f, 4, { 80, 130, 255, (unsigned char)(50 * panelAlpha) });
    DrawTextEx(font, roleTxt, { (float)(infoX + 7), (float)(infoY + 28) }, 10, 1,
        { 80, 130, 255, PA });

    // Email (truncated)
    std::string emailDisplay = sessionUser.email.empty() ? "No email set" : sessionUser.email;
    if (emailDisplay.size() > 26) emailDisplay = emailDisplay.substr(0, 24) + "..";
    DrawTextEx(font, emailDisplay.c_str(), { (float)infoX, (float)(infoY + 50) }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // ── Stats card ────────────────────────────────────────────────────────────
    DrawRectangle((int)statsCard.x + 4, (int)statsCard.y + 6,
        (int)statsCard.width, (int)statsCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(statsCard, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines(statsCard, 0.08f, 8, BORDER_NORMAL);
    DrawRectangleRounded({ statsCard.x, statsCard.y, statsCard.width, 5 }, 0.5f, 4,
        { 255, 140, 60, PA });

    DrawTextEx(font, "OVERVIEW", { statsCard.x + 14, statsCard.y + 14 }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
    DrawRectangle((int)statsCard.x + 14, (int)statsCard.y + 28,
        (int)statsCard.width - 28, 1, BORDER_NORMAL);

    // Two-column stats grid
    int sCols = 2;
    int sRows = 2;
    float sCellW = (statsCard.width - 14) / sCols;
    float sCellH = (statsCardH - 38.0f) / sRows;
    for (int si = 0; si < 4; si++)
    {
        int col = si % sCols;
        int row = si / sCols;
        float sx = statsCard.x + 14 + col * sCellW;
        float sy = statsCard.y + 36 + row * sCellH;

        DrawTextEx(font, prof_stats[si].value,
            { sx, sy + 4 }, 15, 1, { prof_stats[si].accent.r, prof_stats[si].accent.g,
                                     prof_stats[si].accent.b, PA });
        DrawTextEx(font, prof_stats[si].label,
            { sx, sy + 24 }, 9, 1,
            { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, (unsigned char)(160 * panelAlpha) });
    }

    // ── Activity card ─────────────────────────────────────────────────────────
    DrawRectangle((int)actCard.x + 4, (int)actCard.y + 6,
        (int)actCard.width, (int)actCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(actCard, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines(actCard, 0.08f, 8, BORDER_NORMAL);
    DrawRectangleRounded({ actCard.x, actCard.y, actCard.width, 5 }, 0.5f, 4,
        { 180, 80, 255, PA });

    DrawTextEx(font, "RECENT BOOKINGS", { actCard.x + 14, actCard.y + 14 }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
    DrawRectangle((int)actCard.x + 14, (int)actCard.y + 28,
        (int)actCard.width - 28, 1, BORDER_NORMAL);

    int maxItems = std::min((int)(sizeof(prof_activity) / sizeof(prof_activity[0])),
        (int)((actCardH - 38) / 52));

    for (int ai = 0; ai < maxItems; ai++)
    {
        float ay = actCard.y + 38 + ai * 52.0f;

        // Row background on hover
        Rectangle rowRect = { actCard.x + 10, ay, actCard.width - 20, 46 };
        bool rowHov = CheckCollisionPointRec(mouse, rowRect);
        if (rowHov)
            DrawRectangleRounded(rowRect, 0.12f, 6, { 30, 35, 60, (unsigned char)(120 * panelAlpha) });

        // Accent dot
        DrawCircle((int)(actCard.x + 22), (int)(ay + 14),
            5, { prof_activity[ai].accent.r, prof_activity[ai].accent.g,
                 prof_activity[ai].accent.b, PA });

        // Title
        std::string actTitle = prof_activity[ai].title;
        if (actTitle.size() > 22) actTitle = actTitle.substr(0, 20) + "..";
        DrawTextEx(font, actTitle.c_str(), { actCard.x + 34, ay + 6 }, 12, 1,
            { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

        // Date + seat
        DrawTextEx(font, prof_activity[ai].date, { actCard.x + 34, ay + 24 }, 10, 1,
            { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, (unsigned char)(160 * panelAlpha) });

        Vector2 seatSz = MeasureTextEx(font, prof_activity[ai].seat, 10, 1);
        DrawTextEx(font, prof_activity[ai].seat,
            { actCard.x + actCard.width - seatSz.x - 14, ay + 24 }, 10, 1,
            { prof_activity[ai].accent.r, prof_activity[ai].accent.g,
              prof_activity[ai].accent.b, (unsigned char)(180 * panelAlpha) });

        // Divider (not last)
        if (ai < maxItems - 1)
            DrawRectangle((int)(actCard.x + 34), (int)(ay + 46),
                (int)(actCard.width - 48), 1, BORDER_NORMAL);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  RIGHT COLUMN – PROFILE FORM CARD
    // ─────────────────────────────────────────────────────────────────────────
    DrawRectangle((int)formCard.x + 5, (int)formCard.y + 8,
        (int)formCard.width, (int)formCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(formCard, 0.04f, 10, BG_CARD);
    DrawRectangleRoundedLines(formCard, 0.04f, 10, BORDER_NORMAL);
    DrawRectangleRounded({ formCard.x, formCard.y, formCard.width, 6 }, 0.04f, 4,
        { 80, 130, 255, PA });

    // Card title row
    DrawTextEx(font, "ACCOUNT SETTINGS",
        { formCard.x + fPad, formCard.y + 18 }, 14, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // Edit / View mode badge next to title
    {
        const char* modeTxt = prof_editMode ? "EDITING" : "VIEW ONLY";
        Color modeCol = prof_editMode ? Color{ 255, 180, 50, 255 } : Color{ 80, 200, 130, 255 };
        Vector2 mtSz = MeasureTextEx(font, modeTxt, 10, 1);
        float mtX = formCard.x + fPad + MeasureTextEx(font, "ACCOUNT SETTINGS", 14, 1).x + 14;
        DrawRectangleRounded({ mtX, formCard.y + 20, mtSz.x + 14, 18 }, 0.4f, 4,
            { modeCol.r, modeCol.g, modeCol.b, (unsigned char)(40 * panelAlpha) });
        DrawTextEx(font, modeTxt, { mtX + 7, formCard.y + 24 }, 10, 1,
            { modeCol.r, modeCol.g, modeCol.b, PA });
    }

    DrawRectangle((int)formCard.x + fPad, (int)formCard.y + 40,
        (int)formCard.width - fPad * 2, 1, BORDER_NORMAL);

    // ─ Error banner ──────────────────────────────────────────────────────────
    {
        unsigned char ea = (unsigned char)(prof_errorAlpha * panelAlpha * 255.0f);
        Rectangle errBox = { (float)fX, (float)(rowBase - 34), (float)fW, 28 };
        DrawRectangleRec(errBox, { 40, 20, 20, ea });
        DrawRectangleLinesEx(errBox, 1, { 200, 70, 70, ea });
        DrawTextEx(font, prof_errorMsg.c_str(), { errBox.x + 12, errBox.y + 8 }, 11, 1,
            { 200, 70, 70, ea });
    }

    // ─ Username field ─────────────────────────────────────────────────────────
    const std::string& dispUsername = prof_editMode ? prof_editUsername : sessionUser.username;
    Prof_DrawField(font, "USERNAME", dispUsername,
        prof_editMode && prof_usernameActive, false,
        usernameField, prof_editMode ? prof_usernameBorderLerp : 0.0f,
        prof_editMode ? prof_usernameGlowLerp : 0.0f, PA, time);

    // Lock icon overlay when not in edit mode
    if (!prof_editMode)
    {
        DrawTextEx(font, "LOCKED", { usernameField.x + usernameField.width - 60, usernameField.y + 16 },
            10, 1, { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(120 * panelAlpha) });
    }

    // ─ Email field ────────────────────────────────────────────────────────────
    const std::string& dispEmail = prof_editMode ? prof_editEmail : sessionUser.email;
    Prof_DrawField(font, "EMAIL ADDRESS", dispEmail,
        prof_editMode && prof_emailActive, false,
        emailField, prof_editMode ? prof_emailBorderLerp : 0.0f,
        prof_editMode ? prof_emailGlowLerp : 0.0f, PA, time);

    if (!prof_editMode)
    {
        DrawTextEx(font, "LOCKED", { emailField.x + emailField.width - 60, emailField.y + 16 },
            10, 1, { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(120 * panelAlpha) });
    }

    // ─ Save / Edit button ─────────────────────────────────────────────────────
    bool hoverSave = CheckCollisionPointRec(mouse, saveBtn);
    Color saveBg = prof_editMode
        ? (hoverSave ? ACCENT_HOVER : ACCENT)
        : (hoverSave ? Color{ 50, 60, 90, 220 } : Color{ 30, 35, 60, 200 });
    DrawRectangleRounded(saveBtn, 0.3f, 8, { saveBg.r, saveBg.g, saveBg.b, PA });
    DrawRectangleRoundedLines(saveBtn, 0.3f, 8,
        hoverSave ? Color{ ACCENT.r, ACCENT.g, ACCENT.b, PA }
    : Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });

    // Ripple on save
    if (prof_rippleActive)
    {
        float rp = prof_rippleTimer / PROF_RIPPLE_DURATION;
        DrawCircle((int)prof_rippleX, (int)prof_rippleY, rp * PROF_RIPPLE_MAX_R,
            { 255, 255, 255, (unsigned char)((1.0f - rp) * 50.0f) });
    }

    const char* saveLbl = prof_editMode ? "SAVE CHANGES" : "EDIT PROFILE";
    Vector2 saveSz = MeasureTextEx(font, saveLbl, 13, 1);
    DrawTextEx(font, saveLbl,
        { saveBtn.x + saveBtn.width / 2 - saveSz.x / 2,
          saveBtn.y + saveBtn.height / 2 - saveSz.y / 2 },
        13, 1, { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // ─ Divider ────────────────────────────────────────────────────────────────
    DrawRectangle(fX, dividerY, fW, 1, BORDER_NORMAL);

    // ─ Password section toggle ────────────────────────────────────────────────
    bool hoverPwToggle = CheckCollisionPointRec(mouse, pwToggleBtn);
    DrawRectangleRounded(pwToggleBtn, 0.2f, 6,
        hoverPwToggle ? Color{ 30, 35, 60, (unsigned char)(200 * panelAlpha) }
    : Color{ 20, 22, 40, (unsigned char)(160 * panelAlpha) });
    DrawRectangleRoundedLines(pwToggleBtn, 0.2f, 6,
        hoverPwToggle ? Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA }
    : Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });

    DrawTextEx(font, "CHANGE PASSWORD",
        { pwToggleBtn.x + 14, pwToggleBtn.y + 8 }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // Chevron (animated rotate)
    float chevAngle = prof_pwSection ? 1.0f : 0.0f;
    const char* chev = chevAngle > 0.5f ? "v" : ">";
    Vector2 chevSz = MeasureTextEx(font, chev, 11, 1);
    DrawTextEx(font, chev,
        { pwToggleBtn.x + pwToggleBtn.width - chevSz.x - 14, pwToggleBtn.y + 8 },
        11, 1, { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // ─ Password fields (clipped to animated height) ───────────────────────────
    if (prof_pwSectionHeight > 1.0f)
    {
        BeginScissorMode(fX, (int)pwToggleBtn.y + 30,
            fW, (int)prof_pwSectionHeight);

        // Current password
        Prof_DrawField(font, "CURRENT PASSWORD", prof_pwCurrent,
            prof_pwCurrActive, !prof_showCurr,
            pwCurrField, prof_pwCurrActive ? 1.0f : 0.0f,
            prof_pwCurrActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showCurr, eyeCurr, TEXT_SECONDARY, PA);

        // New password
        Prof_DrawField(font, "NEW PASSWORD", prof_pwNew,
            prof_pwNewActive, !prof_showNew,
            pwNewField, prof_pwNewActive ? 1.0f : 0.0f,
            prof_pwNewActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showNew, eyeNew, TEXT_SECONDARY, PA);

        // Confirm password
        Prof_DrawField(font, "CONFIRM NEW PASSWORD", prof_pwConfirm,
            prof_pwConfActive, !prof_showConf,
            pwConfField, prof_pwConfActive ? 1.0f : 0.0f,
            prof_pwConfActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showConf, eyeConf, TEXT_SECONDARY, PA);

        // Update password button
        bool hoverPwUpd = CheckCollisionPointRec(mouse, pwUpdateBtn);
        DrawRectangleRounded(pwUpdateBtn, 0.3f, 8,
            hoverPwUpd ? Color{ 180, 80, 80, PA } : Color{ 100, 40, 40, PA });
        DrawRectangleRoundedLines(pwUpdateBtn, 0.3f, 8,
            hoverPwUpd ? Color{ 220, 100, 100, PA } : Color{ 160, 60, 60, PA });
        Vector2 pwuSz = MeasureTextEx(font, "UPDATE PASSWORD", 13, 1);
        DrawTextEx(font, "UPDATE PASSWORD",
            { pwUpdateBtn.x + pwUpdateBtn.width / 2 - pwuSz.x / 2,
              pwUpdateBtn.y + pwUpdateBtn.height / 2 - pwuSz.y / 2 },
            13, 1, { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

        EndScissorMode();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  STATUS BAR
    // ─────────────────────────────────────────────────────────────────────────
    int barY = screenH - 34;
    DrawRectangle(0, barY, screenW, 34, { 10, 12, 28, 210 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);

    DrawTextEx(font, "ACCOUNT SETTINGS", { 32, (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, "YOUR PROFILE", { 220,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, "12 BOOKINGS", { 360,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    float dotP = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotP * 75);
    DrawCircle(screenW - 120, barY + 17, 5, { 80, 220, 120, dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110), (float)(barY + 10) }, 11, 1,
        { 80, 220, 120, 255 });

    // ─────────────────────────────────────────────────────────────────────────
    //  TOAST
    // ─────────────────────────────────────────────────────────────────────────
    if (prof_toastTimer > 0.0f)
    {
        float fadeIn = std::min(1.0f, (PROF_TOAST_DURATION - prof_toastTimer) / 0.2f);
        float fadeOut = std::min(1.0f, prof_toastTimer / 0.3f);
        float alpha = std::min(fadeIn, fadeOut);
        unsigned char ta = (unsigned char)(alpha * 240.0f);

        Vector2 tSz = MeasureTextEx(font, prof_toastMsg.c_str(), 12, 1);
        float tw = tSz.x + 32, th = 42;
        float tx = (float)(screenW / 2) - tw / 2, ty = (float)(barY - th - 12);

        DrawRectangleRounded({ tx, ty, tw, th }, 0.3f, 8, { 20, 120, 60, ta });
        DrawRectangleRoundedLines({ tx, ty, tw, th }, 0.3f, 8, { 60, 200, 100, ta });
        DrawTextEx(font, prof_toastMsg.c_str(), { tx + 16, ty + 13 }, 12, 1,
            { 200, 255, 220, ta });
    }

    EndDrawing();
    return PROFILE;
}