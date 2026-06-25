#include "profileScreen.h"
#include <iostream>
#include "../colors.h"
#include "../DAL/userRepository.h"
#include "../DAL/BookingRepository.h"
#include "../BLL/bookingService.h"
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>

// Nav
static const char* prof_navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int         prof_activeNav = 3;

// Entrance animation
static float prof_entranceTimer = 0.0f;
static const float PROF_ENTER_DURATION = 0.55f;
static float Prof_EaseOutCubic(float t) { float inv = 1.0f - t; return 1.0f - inv * inv * inv; }

// Particles
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

// Toast
static std::string prof_toastMsg = "";
static float       prof_toastTimer = 0.0f;
static bool        prof_toastIsError = false;
static const float PROF_TOAST_DURATION = 2.8f;

static void Prof_ShowToast(const std::string& msg, bool isError = false)
{
    prof_toastMsg = msg;
    prof_toastTimer = PROF_TOAST_DURATION;
    prof_toastIsError = isError;
}

// Ripple
static bool  prof_rippleActive = false;
static float prof_rippleTimer = 0.0f;
static float prof_rippleX = 0, prof_rippleY = 0;
static const float PROF_RIPPLE_DURATION = 0.5f;
static const float PROF_RIPPLE_MAX_R = 140.0f;

// Color helpers
static Color Prof_LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

// Edit-field state
static std::string prof_editEmail = "";
static std::string prof_editUsername = "";
static bool        prof_editMode = false;

static bool  prof_emailActive = false;
static bool  prof_usernameActive = false;

static float prof_emailBorderLerp = 0.0f;
static float prof_usernameBorderLerp = 0.0f;
static float prof_emailGlowLerp = 0.0f;
static float prof_usernameGlowLerp = 0.0f;

// Password change state
static bool        prof_pwSection = false;
static std::string prof_pwCurrent = "";
static std::string prof_pwNew = "";
static std::string prof_pwConfirm = "";
static bool        prof_pwCurrActive = false;
static bool        prof_pwNewActive = false;
static bool        prof_pwConfActive = false;
static bool        prof_showCurr = false;
static bool        prof_showNew = false;
static bool        prof_showConf = false;

// Enough height for 3 fields (each ~48+34=82) + button (48) + labels (18 each) + padding
static float prof_pwSectionHeight = 0.0f;
static const float PROF_PW_EXPANDED = 340.0f;

// Live DB data
static bool                  prof_dataLoaded = false;
static std::vector<Booking>  prof_bookings;

// Computed stats
static int         prof_totalBookings = 0;
static float       prof_totalHours = 0.0f;
static std::string prof_favGenre = "—";
static std::string prof_memberSince = "—";

// Genre mapping (movie title substring → genre)
static const std::pair<const char*, const char*> GENRE_MAP[] = {
    {"DUNE",          "SCI-FI"},
    {"INTERSTELLAR",  "SCI-FI"},
    {"OPPENHEIMER",   "DRAMA"},
    {"BATMAN",        "ACTION"},
    {"PAST LIVES",    "ROMANCE"},
    {"POOR THINGS",   "DRAMA"},
    {"KILLER",        "WESTERN"},
};

static std::string Prof_GuessGenre(const std::string& title)
{
    std::string up = title;
    std::transform(up.begin(), up.end(), up.begin(), ::toupper);
    for (auto& kv : GENRE_MAP)
        if (up.find(kv.first) != std::string::npos)
            return kv.second;
    return "MIXED";
}

// Average movie runtime in hours used for "hours watched" estimate
static const float AVG_RUNTIME_H = 2.1f;

static void Prof_LoadData(const std::string& username)
{
    prof_bookings = BookingRepository::GetBookingsByUser(username);
    prof_totalBookings = (int)prof_bookings.size();
    prof_totalHours = prof_totalBookings * AVG_RUNTIME_H;

    // Favourite genre: most common among booked movies
    std::map<std::string, int> genreCount;
    for (auto& b : prof_bookings)
        genreCount[Prof_GuessGenre(b.movieTitle)]++;
    if (!genreCount.empty())
    {
        auto it = std::max_element(genreCount.begin(), genreCount.end(),
            [](auto& a, auto& b) { return a.second < b.second; });
        prof_favGenre = it->first;
    }
    else
        prof_favGenre = "—";

    // Member since: extract year from earliest booking date (YYYY-MM-DD or similar)
    if (!prof_bookings.empty())
    {
        std::string earliest = prof_bookings[0].bookingDate;
        for (auto& b : prof_bookings)
            if (b.bookingDate < earliest) earliest = b.bookingDate;
        prof_memberSince = (earliest.size() >= 4) ? earliest.substr(0, 4) : "—";
    }
    else
        prof_memberSince = "—";

    prof_dataLoaded = true;
}

// Error / validation
static bool        prof_showError = false;
static std::string prof_errorMsg = "";
static float       prof_errorAlpha = 0.0f;

static void Prof_TriggerError(const std::string& msg)
{
    prof_errorMsg = msg;
    prof_showError = true;
    prof_errorAlpha = 0.0f;
}

// Avatar pulse
static float prof_avatarPulse = 0.0f;

// Shake
static float prof_shakeTimer = 0.0f;
static float prof_shakeOffsetX = 0.0f;
static const float PROF_SHAKE_DURATION = 0.45f;
static const float PROF_SHAKE_MAGNITUDE = 6.0f;

// Helper: masked string
static std::string Prof_Mask(const std::string& s)
{
    std::string m; for (auto& c : s) { (void)c; m += '*'; } return m;
}

// Helper: eye icon
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

// Helper: input field row
static void Prof_DrawField(Font font, const char* label,
    const std::string& value, bool active, bool masked,
    Rectangle field, float borderLerp, float glowLerp,
    unsigned char PA, float time)
{
    if (glowLerp > 0.01f)
    {
        unsigned char ga = (unsigned char)(glowLerp * 38.0f * (PA / 255.0f));
        DrawRectangleRounded(
            { field.x - 4, field.y - 4, field.width + 8, field.height + 8 },
            0.22f, 8, { BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, ga });
    }

    Color border = Prof_LerpColor(BORDER_NORMAL, BORDER_FOCUS, borderLerp);
    border.a = PA;

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

    if (active && ((int)(time * 2)) % 2 == 0)
    {
        float cx = field.x + 14 + MeasureTextEx(font, shown.c_str(), 13, 1).x + 2;
        DrawRectangle((int)cx, (int)(field.y + 10), 2, 22,
            { BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA });
    }
}

// Helper: stat cell
static void Prof_DrawStat(Font font, const char* label, const std::string& value,
    Color accent, float sx, float sy, unsigned char PA, float panelAlpha)
{
    DrawTextEx(font, value.c_str(), { sx, sy + 4 }, 15, 1,
        { accent.r, accent.g, accent.b, PA });
    DrawTextEx(font, label, { sx, sy + 24 }, 9, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, (unsigned char)(160 * panelAlpha) });
}

AppState profileScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    if (!prof_particlesInit) Prof_InitParticles(screenW, screenH);

    // Load live data from MongoDB on first frame
    if (!prof_dataLoaded && !sessionUser.username.empty())
    {
        // Also fetch email from DB in case session doesn't have it
        if (sessionUser.email.empty())
            sessionUser.email = UserRepository::GetUserEmail(sessionUser.username);

        Prof_LoadData(sessionUser.username);
    }

    // Entrance
    if (prof_entranceTimer < PROF_ENTER_DURATION) prof_entranceTimer += dt;
    float enterT = Prof_EaseOutCubic(prof_entranceTimer / PROF_ENTER_DURATION);
    float panelAlpha = enterT;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    Prof_UpdateParticles(dt, screenW, screenH);

    if (prof_toastTimer > 0) prof_toastTimer -= dt;
    if (prof_rippleActive)
    {
        prof_rippleTimer += dt;
        if (prof_rippleTimer >= PROF_RIPPLE_DURATION) prof_rippleActive = false;
    }

    prof_errorAlpha += ((prof_showError ? 1.0f : 0.0f) - prof_errorAlpha) * dt * 12.0f;

    if (prof_shakeTimer > 0.0f)
    {
        prof_shakeTimer -= dt;
        if (prof_shakeTimer < 0.0f) prof_shakeTimer = 0.0f;
        float p = prof_shakeTimer / PROF_SHAKE_DURATION;
        prof_shakeOffsetX = sinf(p * 3.14159f * 8.0f) * PROF_SHAKE_MAGNITUDE * p;
    }
    else prof_shakeOffsetX = 0.0f;

    prof_avatarPulse += dt;

    float pwTarget = prof_pwSection ? PROF_PW_EXPANDED : 0.0f;
    prof_pwSectionHeight += (pwTarget - prof_pwSectionHeight) * dt * 14.0f;

    prof_emailBorderLerp += ((prof_emailActive ? 1.0f : 0.0f) - prof_emailBorderLerp) * dt * 14.0f;
    prof_usernameBorderLerp += ((prof_usernameActive ? 1.0f : 0.0f) - prof_usernameBorderLerp) * dt * 14.0f;
    prof_emailGlowLerp += ((prof_emailActive ? 1.0f : 0.0f) - prof_emailGlowLerp) * dt * 10.0f;
    prof_usernameGlowLerp += ((prof_usernameActive ? 1.0f : 0.0f) - prof_usernameGlowLerp) * dt * 10.0f;

    if (prof_editMode && prof_editEmail.empty() && prof_editUsername.empty())
    {
        prof_editEmail = sessionUser.email;
        prof_editUsername = sessionUser.username;
    }

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Keyboard input
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

    int navH = 64;

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

    int contentY = navH + 18;
    int contentH = screenH - navH - 34 - 18;
    int margin = 36;

    int leftW = 310;
    int leftX = margin;

    int rightX = leftX + leftW + 24;
    int rightW = screenW - rightX - margin;

    // Avatar card
    int avatarCardH = 180;
    Rectangle avatarCard = { (float)leftX, (float)contentY, (float)leftW, (float)avatarCardH };

    // Stats card
    int statsCardY = contentY + avatarCardH + 14;
    int statsCardH = 148;
    Rectangle statsCard = { (float)leftX, (float)statsCardY, (float)leftW, (float)statsCardH };

    // Activity card
    int actCardY = statsCardY + statsCardH + 14;
    int actCardH = contentY + contentH - actCardY;
    Rectangle actCard = { (float)leftX, (float)actCardY, (float)leftW, (float)actCardH };

    // Form card
    int   formCardH = contentH;
    float formCardX = (float)(rightX + prof_shakeOffsetX);
    Rectangle formCard = { formCardX, (float)contentY, (float)rightW, (float)formCardH };

    int fPad = 32;
    int fW = rightW - fPad * 2;
    int fH = 48;
    int fX = (int)formCardX + fPad;

    int rowBase = contentY + 56;
    int userFieldY = rowBase;
    int emailFieldY = userFieldY + fH + 38;
    int saveBtnY = emailFieldY + fH + 28;
    int dividerY = saveBtnY + fH + 22;
    int pwToggleY = dividerY + 12;
    int pwBaseY = pwToggleY + 36;

    Rectangle usernameField = { (float)fX, (float)userFieldY,  (float)fW, (float)fH };
    Rectangle emailField = { (float)fX, (float)emailFieldY, (float)fW, (float)fH };
    Rectangle saveBtn = { (float)fX, (float)saveBtnY,    (float)fW, (float)fH };

    // Password fields – each row is: 18px label + 48px field = 66px; add 16px gap between rows
    int pwRowH = fH + 34;   // 82px per row (label 18 + field 48 + gap 16)
    Rectangle pwCurrField = { (float)fX, (float)(pwBaseY + 26),               (float)fW, (float)fH };
    Rectangle pwNewField = { (float)fX, (float)(pwBaseY + 26 + pwRowH),      (float)fW, (float)fH };
    Rectangle pwConfField = { (float)fX, (float)(pwBaseY + 26 + pwRowH * 2),  (float)fW, (float)fH };
    Rectangle pwUpdateBtn = { (float)fX, (float)(pwBaseY + 26 + pwRowH * 3 + 10), (float)fW, (float)fH };

    Rectangle eyeCurr = { pwCurrField.x + pwCurrField.width - 38, pwCurrField.y + 13, 22, 22 };
    Rectangle eyeNew = { pwNewField.x + pwNewField.width - 38, pwNewField.y + 13, 22, 22 };
    Rectangle eyeConf = { pwConfField.x + pwConfField.width - 38, pwConfField.y + 13, 22, 22 };

    Rectangle pwToggleBtn = { (float)fX, (float)pwToggleY, (float)fW, 30 };

    Rectangle logoutBtn = { (float)(screenW - 105), (float)(navH / 2 - 14), 88, 28 };
    bool hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);

    // Click handling
    if (clicked)
    {
        if (prof_editMode)
        {
            prof_usernameActive = CheckCollisionPointRec(mouse, usernameField);
            prof_emailActive = CheckCollisionPointRec(mouse, emailField);
        }

        if (prof_pwSection && prof_pwSectionHeight > 10.0f)
        {
            prof_pwCurrActive = CheckCollisionPointRec(mouse, pwCurrField);
            prof_pwNewActive = CheckCollisionPointRec(mouse, pwNewField);
            prof_pwConfActive = CheckCollisionPointRec(mouse, pwConfField);

            if (CheckCollisionPointRec(mouse, eyeCurr)) prof_showCurr = !prof_showCurr;
            if (CheckCollisionPointRec(mouse, eyeNew))  prof_showNew = !prof_showNew;
            if (CheckCollisionPointRec(mouse, eyeConf)) prof_showConf = !prof_showConf;
        }

        // Save / Edit
        if (CheckCollisionPointRec(mouse, saveBtn))
        {
            if (prof_editMode)
            {
                if (prof_editUsername.empty() || prof_editEmail.empty())
                {
                    Prof_TriggerError("ERROR: ALL FIELDS REQUIRED");
                    prof_shakeTimer = PROF_SHAKE_DURATION;
                }
                else
                {
                    // Persist email to MongoDB (username is immutable)
                    bool ok = UserRepository::UpdateUser(sessionUser.username,
                        prof_editEmail, "");
                    sessionUser.email = prof_editEmail;
                    prof_editMode = false;
                    prof_usernameActive = false;
                    prof_emailActive = false;
                    prof_showError = false;
                    prof_rippleActive = true;
                    prof_rippleTimer = 0.0f;
                    prof_rippleX = mouse.x;
                    prof_rippleY = mouse.y;
                    Prof_ShowToast(ok ? "PROFILE UPDATED IN DATABASE"
                        : "SAVED LOCALLY (DB OFFLINE)", !ok);
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

        // Update password
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
                // Validate current password then persist new one
                bool valid = UserRepository::ValidateUser(sessionUser.username, prof_pwCurrent);
                if (!valid)
                {
                    Prof_TriggerError("ERROR: CURRENT PASSWORD IS INCORRECT");
                    prof_shakeTimer = PROF_SHAKE_DURATION;
                }
                else
                {
                    bool ok = UserRepository::UpdateUser(sessionUser.username,
                        "", prof_pwNew);
                    prof_pwCurrent = "";
                    prof_pwNew = "";
                    prof_pwConfirm = "";
                    prof_pwSection = false;
                    prof_showError = false;
                    Prof_ShowToast(ok ? "PASSWORD UPDATED IN DATABASE"
                        : "SAVED LOCALLY (DB OFFLINE)", !ok);
                }
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
            prof_dataLoaded = false;
            prof_bookings.clear();
            sessionUser.username = "";
            sessionUser.email = "";
            return AUTH;
        }
    }

    BeginDrawing();
    ClearBackground(BG_DARK);

    // Particles
    for (int i = 0; i < PROF_PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(prof_particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)prof_particles[i].x, (int)prof_particles[i].y,
            prof_particles[i].r, { 100, 140, 255, pa });
    }

    // Ambient glow blobs
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

    // Navigation bar
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


    // Avatar card
    DrawRectangle((int)avatarCard.x + 4, (int)avatarCard.y + 6,
        (int)avatarCard.width, (int)avatarCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(avatarCard, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines(avatarCard, 0.08f, 8, BORDER_NORMAL);
    DrawRectangleRounded({ avatarCard.x, avatarCard.y, avatarCard.width, 5 }, 0.5f, 4,
        { 80, 130, 255, PA });

    float avCX = avatarCard.x + 70;
    float avCY = avatarCard.y + avatarCardH / 2.0f;
    float avR = 44.0f;

    float ringPulse = (sinf(prof_avatarPulse * 1.2f) + 1.0f) / 2.0f;
    unsigned char ringA = (unsigned char)((0.3f + ringPulse * 0.4f) * PA);
    DrawCircle((int)avCX, (int)avCY, (int)(avR + 8), { 80, 130, 255, ringA });
    DrawCircle((int)avCX, (int)avCY, (int)(avR + 5), { 20, 25, 48, PA });
    DrawCircle((int)avCX, (int)avCY, (int)avR, { 25, 30, 60, PA });
    DrawCircleLines((int)avCX, (int)avCY, avR, { 80, 130, 255, PA });

    std::string initials = "";
    if (!sessionUser.username.empty()) initials += (char)toupper(sessionUser.username[0]);
    if (sessionUser.username.size() > 1) initials += (char)toupper(sessionUser.username[1]);
    Vector2 initSz = MeasureTextEx(font, initials.c_str(), 22, 1);
    DrawTextEx(font, initials.c_str(),
        { avCX - initSz.x / 2, avCY - initSz.y / 2 }, 22, 1, { 80, 130, 255, PA });

    int infoX = (int)avCX + (int)avR + 18;
    int infoY = (int)avCY - 30;
    DrawTextEx(font, sessionUser.username.c_str(), { (float)infoX, (float)infoY }, 16, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    const char* roleTxt = sessionUser.accessLevel > 1 ? "ADMIN" : "MEMBER";
    Color roleCol = sessionUser.accessLevel > 1 ? Color{ 255, 180, 50, 255 } : Color{ 80, 130, 255, 255 };
    Vector2 roleSz = MeasureTextEx(font, roleTxt, 10, 1);
    DrawRectangleRounded({ (float)infoX, (float)(infoY + 22), roleSz.x + 14, 18 },
        0.4f, 4, { roleCol.r, roleCol.g, roleCol.b, (unsigned char)(50 * panelAlpha) });
    DrawTextEx(font, roleTxt, { (float)(infoX + 7), (float)(infoY + 26) }, 10, 1,
        { roleCol.r, roleCol.g, roleCol.b, PA });

    std::string emailDisplay = sessionUser.email.empty() ? "No email set" : sessionUser.email;
    if (emailDisplay.size() > 26) emailDisplay = emailDisplay.substr(0, 24) + "..";
    DrawTextEx(font, emailDisplay.c_str(), { (float)infoX, (float)(infoY + 48) }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // DB connection indicator
    {
        Color dbCol = prof_dataLoaded ? Color{ 80, 220, 120, 255 } : Color{ 200, 80, 80, 255 };
        const char* dbTxt = prof_dataLoaded ? "DB CONNECTED" : "DB OFFLINE";
        float dotPulse = (sinf(time * 3.0f) + 1.0f) / 2.0f;
        unsigned char dotAlpha = (unsigned char)(180 + dotPulse * 75);
        DrawCircle(infoX, (int)(infoY + 70), 4, { dbCol.r, dbCol.g, dbCol.b, dotAlpha });
        DrawTextEx(font, dbTxt, { (float)(infoX + 10), (float)(infoY + 64) }, 10, 1,
            { dbCol.r, dbCol.g, dbCol.b, (unsigned char)(180 * panelAlpha) });
    }

    // Stats card (live data)
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

    // Live values
    std::string sFilms = std::to_string(prof_totalBookings);
    std::string sHours = [&]() {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fh", prof_totalHours);
        return std::string(buf);
        }();
    std::string sGenre = prof_favGenre;
    std::string sSince = prof_memberSince;

    // 2×2 grid
    int   sCols = 2, sRows = 2;
    float sCellW = (statsCard.width - 14) / sCols;
    float sCellH = (statsCardH - 38.0f) / sRows;

    struct { const char* label; std::string value; Color accent; } liveStats[4] = {
        { "FILMS BOOKED",    sFilms, {  80, 130, 255, 255 } },
        { "HOURS WATCHED",   sHours, { 255, 140,  60, 255 } },
        { "FAVOURITE GENRE", sGenre, { 180,  80, 255, 255 } },
        { "MEMBER SINCE",    sSince, {  80, 220, 160, 255 } },
    };
    for (int si = 0; si < 4; si++)
    {
        int   col = si % sCols, row = si / sCols;
        float sx = statsCard.x + 14 + col * sCellW;
        float sy = statsCard.y + 36 + row * sCellH;
        Prof_DrawStat(font, liveStats[si].label, liveStats[si].value,
            liveStats[si].accent, sx, sy, PA, panelAlpha);
    }

    // Activity card (live bookings)
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

    // Accent colours cycling for booking rows
    static const Color ROW_ACCENTS[] = {
        {  80, 130, 255, 255 }, { 255, 140,  60, 255 },
        {  60, 180, 255, 255 }, { 180,  80, 255, 255 },
    };
    static const int NUM_ACCENTS = 4;

    if (prof_bookings.empty())
    {
        DrawTextEx(font, "NO BOOKINGS YET",
            { actCard.x + 14, actCard.y + 44 }, 11, 1,
            { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });
    }
    else
    {
        int maxItems = std::min((int)prof_bookings.size(),
            (int)((actCardH - 38) / 52));
        // Show most recent first
        int startIdx = std::max(0, (int)prof_bookings.size() - maxItems);

        for (int ai = 0; ai < maxItems; ai++)
        {
            const Booking& bk = prof_bookings[prof_bookings.size() - 1 - ai];
            Color accent = ROW_ACCENTS[ai % NUM_ACCENTS];
            float ay = actCard.y + 38 + ai * 52.0f;

            Rectangle rowRect = { actCard.x + 10, ay, actCard.width - 20, 46 };
            bool rowHov = CheckCollisionPointRec(mouse, rowRect);
            if (rowHov)
                DrawRectangleRounded(rowRect, 0.12f, 6, { 30, 35, 60, (unsigned char)(120 * panelAlpha) });

            DrawCircle((int)(actCard.x + 22), (int)(ay + 14), 5,
                { accent.r, accent.g, accent.b, PA });

            std::string actTitle = bk.movieTitle;
            if (actTitle.size() > 22) actTitle = actTitle.substr(0, 20) + "..";
            DrawTextEx(font, actTitle.c_str(), { actCard.x + 34, ay + 6 }, 12, 1,
                { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

            DrawTextEx(font, bk.bookingDate.c_str(), { actCard.x + 34, ay + 24 }, 10, 1,
                { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, (unsigned char)(160 * panelAlpha) });

            std::string seatLine = bk.seatType + "  " + bk.showtime;
            Vector2 seatSz = MeasureTextEx(font, seatLine.c_str(), 10, 1);
            DrawTextEx(font, seatLine.c_str(),
                { actCard.x + actCard.width - seatSz.x - 14, ay + 24 }, 10, 1,
                { accent.r, accent.g, accent.b, (unsigned char)(180 * panelAlpha) });

            if (ai < maxItems - 1)
                DrawRectangle((int)(actCard.x + 34), (int)(ay + 46),
                    (int)(actCard.width - 48), 1, BORDER_NORMAL);
        }
    }

    DrawRectangle((int)formCard.x + 5, (int)formCard.y + 8,
        (int)formCard.width, (int)formCard.height, { 0, 0, 0, 50 });
    DrawRectangleRounded(formCard, 0.04f, 10, BG_CARD);
    DrawRectangleRoundedLines(formCard, 0.04f, 10, BORDER_NORMAL);
    DrawRectangleRounded({ formCard.x, formCard.y, formCard.width, 6 }, 0.04f, 4,
        { 80, 130, 255, PA });

    DrawTextEx(font, "ACCOUNT SETTINGS",
        { formCard.x + fPad, formCard.y + 18 }, 14, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

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

    // Error banner
    {
        unsigned char ea = (unsigned char)(prof_errorAlpha * panelAlpha * 255.0f);
        Rectangle errBox = { (float)fX, (float)(rowBase - 34), (float)fW, 28 };
        DrawRectangleRec(errBox, { 40, 20, 20, ea });
        DrawRectangleLinesEx(errBox, 1, { 200, 70, 70, ea });
        DrawTextEx(font, prof_errorMsg.c_str(), { errBox.x + 12, errBox.y + 8 }, 11, 1,
            { 200, 70, 70, ea });
    }

    // Username field
    const std::string& dispUsername = prof_editMode ? prof_editUsername : sessionUser.username;
    Prof_DrawField(font, "USERNAME", dispUsername,
        prof_editMode && prof_usernameActive, false,
        usernameField, prof_editMode ? prof_usernameBorderLerp : 0.0f,
        prof_editMode ? prof_usernameGlowLerp : 0.0f, PA, time);

    if (!prof_editMode)
    {
        DrawTextEx(font, "LOCKED",
            { usernameField.x + usernameField.width - 60, usernameField.y + 16 },
            10, 1, { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(120 * panelAlpha) });
    }

    // Note: username is immutable in DB — only email is editable
    if (prof_editMode)
    {
        DrawTextEx(font, "(username cannot be changed)",
            { usernameField.x + 14, usernameField.y + usernameField.height + 3 }, 9, 1,
            { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(130 * panelAlpha) });
    }

    // Email field
    const std::string& dispEmail = prof_editMode ? prof_editEmail : sessionUser.email;
    Prof_DrawField(font, "EMAIL ADDRESS", dispEmail,
        prof_editMode && prof_emailActive, false,
        emailField, prof_editMode ? prof_emailBorderLerp : 0.0f,
        prof_editMode ? prof_emailGlowLerp : 0.0f, PA, time);

    if (!prof_editMode)
    {
        DrawTextEx(font, "LOCKED",
            { emailField.x + emailField.width - 60, emailField.y + 16 },
            10, 1, { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(120 * panelAlpha) });
    }

    // Save / Edit button
    bool hoverSave = CheckCollisionPointRec(mouse, saveBtn);
    Color saveBg = prof_editMode
        ? (hoverSave ? ACCENT_HOVER : ACCENT)
        : (hoverSave ? Color{ 50, 60, 90, 220 } : Color{ 30, 35, 60, 200 });
    DrawRectangleRounded(saveBtn, 0.3f, 8, { saveBg.r, saveBg.g, saveBg.b, PA });
    DrawRectangleRoundedLines(saveBtn, 0.3f, 8,
        hoverSave ? Color{ ACCENT.r, ACCENT.g, ACCENT.b, PA }
    : Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });

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

    // Divider
    DrawRectangle(fX, dividerY, fW, 1, BORDER_NORMAL);

    // Password section toggle
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

    const char* chev = prof_pwSection ? "v" : ">";
    Vector2 chevSz = MeasureTextEx(font, chev, 11, 1);
    DrawTextEx(font, chev,
        { pwToggleBtn.x + pwToggleBtn.width - chevSz.x - 14, pwToggleBtn.y + 8 },
        11, 1, { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // Password fields (animated expand)
    if (prof_pwSectionHeight > 1.0f)
    {
        BeginScissorMode(fX, (int)pwToggleBtn.y + 30,
            fW, (int)prof_pwSectionHeight);

        Prof_DrawField(font, "CURRENT PASSWORD", prof_pwCurrent,
            prof_pwCurrActive, !prof_showCurr,
            pwCurrField, prof_pwCurrActive ? 1.0f : 0.0f,
            prof_pwCurrActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showCurr, eyeCurr, TEXT_SECONDARY, PA);

        Prof_DrawField(font, "NEW PASSWORD", prof_pwNew,
            prof_pwNewActive, !prof_showNew,
            pwNewField, prof_pwNewActive ? 1.0f : 0.0f,
            prof_pwNewActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showNew, eyeNew, TEXT_SECONDARY, PA);

        Prof_DrawField(font, "CONFIRM NEW PASSWORD", prof_pwConfirm,
            prof_pwConfActive, !prof_showConf,
            pwConfField, prof_pwConfActive ? 1.0f : 0.0f,
            prof_pwConfActive ? 1.0f : 0.0f, PA, time);
        Prof_DrawEye(prof_showConf, eyeConf, TEXT_SECONDARY, PA);

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

    int barY = screenH - 34;
    DrawRectangle(0, barY, screenW, 34, { 10, 12, 28, 210 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);

    DrawTextEx(font, "ACCOUNT SETTINGS", { 32, (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, "YOUR PROFILE", { 220,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    // Live booking count in status bar
    std::string bkCountTxt = std::to_string(prof_totalBookings) + " BOOKING"
        + (prof_totalBookings != 1 ? "S" : "");
    DrawTextEx(font, bkCountTxt.c_str(), { 360,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    float dotP = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotP * 75);
    DrawCircle(screenW - 120, barY + 17, 5, { 80, 220, 120, dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110), (float)(barY + 10) }, 11, 1,
        { 80, 220, 120, 255 });

    if (prof_toastTimer > 0.0f)
    {
        float fadeIn = std::min(1.0f, (PROF_TOAST_DURATION - prof_toastTimer) / 0.2f);
        float fadeOut = std::min(1.0f, prof_toastTimer / 0.3f);
        float alpha = std::min(fadeIn, fadeOut);
        unsigned char ta = (unsigned char)(alpha * 240.0f);

        Vector2 tSz = MeasureTextEx(font, prof_toastMsg.c_str(), 12, 1);
        float tw = tSz.x + 32, th = 42;
        float tx = (float)(screenW / 2) - tw / 2, ty = (float)(barY - th - 12);

        Color toastBg = prof_toastIsError ? Color{ 120, 30, 30, ta } : Color{ 20, 120, 60, ta };
        Color toastBdr = prof_toastIsError ? Color{ 200, 80, 80, ta } : Color{ 60, 200, 100, ta };
        Color toastTxt = prof_toastIsError ? Color{ 255, 180, 180, ta } : Color{ 200, 255, 220, ta };

        DrawRectangleRounded({ tx, ty, tw, th }, 0.3f, 8, toastBg);
        DrawRectangleRoundedLines({ tx, ty, tw, th }, 0.3f, 8, toastBdr);
        DrawTextEx(font, prof_toastMsg.c_str(), { tx + 16, ty + 13 }, 12, 1, toastTxt);
    }

    EndDrawing();
    return PROFILE;
}