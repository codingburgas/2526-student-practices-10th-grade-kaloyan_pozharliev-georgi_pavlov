#include "cinemasScreen.h"
#include "../colors.h"
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

// Nav
static const char* cin_navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int         cin_activeNav = 1;
static AppState    cin_pendingNav = CINEMAS;

// Entrance
static float       cin_entranceTimer = 0.0f;
static const float CIN_ENTER_DURATION = 0.55f;
static float Cin_EaseOutCubic(float t) { float inv = 1.0f - t; return 1.0f - inv * inv * inv; }

// Particles
static const int CIN_PARTICLE_COUNT = 55;
struct CinParticle { float x, y, vx, vy, r, alpha; };
static CinParticle cin_particles[CIN_PARTICLE_COUNT];
static bool        cin_particlesInit = false;

static void Cin_InitParticles(int w, int h)
{
    for (int i = 0; i < CIN_PARTICLE_COUNT; i++)
    {
        cin_particles[i].x = (float)GetRandomValue(0, w);
        cin_particles[i].y = (float)GetRandomValue(0, h);
        cin_particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        cin_particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f;
        cin_particles[i].r = (float)GetRandomValue(1, 3);
        cin_particles[i].alpha = (float)GetRandomValue(20, 70) / 255.0f;
    }
    cin_particlesInit = true;
}

static void Cin_UpdateParticles(float dt, int w, int h)
{
    for (int i = 0; i < CIN_PARTICLE_COUNT; i++)
    {
        cin_particles[i].x += cin_particles[i].vx * dt * 60.0f;
        cin_particles[i].y += cin_particles[i].vy * dt * 60.0f;
        if (cin_particles[i].y < -4)    cin_particles[i].y = (float)h + 4;
        if (cin_particles[i].x < -4)    cin_particles[i].x = (float)w + 4;
        if (cin_particles[i].x > w + 4) cin_particles[i].x = -4.0f;
    }
}

// Toast
static std::string cin_toastMsg = "";
static float       cin_toastTimer = 0.0f;
static const float CIN_TOAST_DURATION = 2.8f;
static void Cin_ShowToast(const std::string& msg) { cin_toastMsg = msg; cin_toastTimer = CIN_TOAST_DURATION; }

// Color lerp
static Color Cin_LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

// Data
struct CinScreen
{
    const char* name;
    int         seats;
    const char* type;   // "STANDARD" | "IMAX" | "4DX" | "VIP"
};

struct Cinema
{
    const char* name;
    const char* address;
    const char* distance;   // e.g. "1.2 km"
    const char* hours;
    float       rating;
    bool        parkingAvailable;
    bool        accessibleSeating;
    Color       accent;
    std::vector<CinScreen> screens;
};

static std::vector<Cinema> cin_cinemas = {
    {
        "GEKOYA GRAND",
        "12 Starlight Boulevard, City Centre",
        "1.2 km",
        "09:00 – 00:00",
        4.8f, true, true,
        { 80, 130, 255, 255 },
        {
            { "SCREEN 1",  220, "IMAX"     },
            { "SCREEN 2",  180, "STANDARD" },
            { "SCREEN 3",  180, "STANDARD" },
            { "SCREEN 4",   80, "VIP"      },
        }
    },
    {
        "GEKOYA WESTSIDE",
        "7 Harbour Walk, West District",
        "3.4 km",
        "10:00 – 23:30",
        4.5f, true, true,
        { 255, 140, 60, 255 },
        {
            { "SCREEN 1",  200, "STANDARD" },
            { "SCREEN 2",  200, "STANDARD" },
            { "SCREEN 3",  120, "4DX"      },
        }
    },
    {
        "GEKOYA NORTH PARK",
        "55 Elm Street, North Quarter",
        "6.1 km",
        "10:00 – 23:00",
        4.2f, false, true,
        { 80, 220, 160, 255 },
        {
            { "SCREEN 1",  160, "STANDARD" },
            { "SCREEN 2",  160, "STANDARD" },
        }
    },
    {
        "GEKOYA PLAZA",
        "Plaza Mall Level 3, East Side",
        "8.7 km",
        "11:00 – 00:30",
        4.6f, true, false,
        { 180, 80, 255, 255 },
        {
            { "SCREEN 1",  200, "IMAX"     },
            { "SCREEN 2",  180, "STANDARD" },
            { "SCREEN 3",  180, "STANDARD" },
            { "SCREEN 4",  100, "4DX"      },
            { "SCREEN 5",   60, "VIP"      },
        }
    },
};

// UI state
static int   cin_selectedCinema = -1;   // -1 = list view
static int   cin_hoveredCinema = -1;
static float cin_detailSlide = 0.0f; // 0 = fully on screen, screenW = off
static float cin_cardHoverY[8] = {};

// Search
static std::string cin_searchQuery = "";
static bool        cin_searchActive = false;

// Detail view scroll (for screen list)
static float cin_detailScrollY = 0.0f;
static float cin_detailScrollTarget = 0.0f;

// Helpers
static Color Cin_ScreenTypeColor(const char* type)
{
    std::string t = type;
    if (t == "IMAX")     return { 80,  130, 255, 255 };
    if (t == "4DX")      return { 255, 140,  60, 255 };
    if (t == "VIP")      return { 180,  80, 255, 255 };
    return { 100, 110, 140, 255 }; // STANDARD
}

static std::vector<int> Cin_FilteredIndices()
{
    std::vector<int> result;
    for (int i = 0; i < (int)cin_cinemas.size(); i++)
    {
        if (!cin_searchQuery.empty())
        {
            std::string q = cin_searchQuery;
            std::transform(q.begin(), q.end(), q.begin(), ::toupper);
            std::string n = cin_cinemas[i].name;
            std::string a = cin_cinemas[i].address;
            std::transform(n.begin(), n.end(), n.begin(), ::toupper);
            std::transform(a.begin(), a.end(), a.begin(), ::toupper);
            if (n.find(q) == std::string::npos && a.find(q) == std::string::npos)
                continue;
        }
        result.push_back(i);
    }
    return result;
}

// Draw one cinema card
static void Cin_DrawCard(Font font, int idx, float cx, float cy,
    int cardW, int cardH, bool hovered, bool selected,
    Vector2 mouse, bool clicked, float dt, float panelAlpha)
{
    const Cinema& c = cin_cinemas[idx];
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    float targetLift = hovered ? -7.0f : 0.0f;
    cin_cardHoverY[idx] += (targetLift - cin_cardHoverY[idx]) * dt * 14.0f;
    float drawY = cy + cin_cardHoverY[idx];

    // Shadow
    DrawRectangleRounded({ cx + 4, drawY + 8, (float)cardW, (float)cardH },
        0.07f, 8, { 0, 0, 0, (unsigned char)(hovered ? 90 : 50) });

    Color border = selected ? c.accent : (hovered ? BORDER_FOCUS : BORDER_NORMAL);
    DrawRectangleRounded({ cx, drawY, (float)cardW, (float)cardH }, 0.07f, 8, BG_CARD);
    DrawRectangleRoundedLines({ cx, drawY, (float)cardW, (float)cardH }, 0.07f, 8, border);

    // Accent top bar
    DrawRectangleRounded({ cx, drawY, (float)cardW, 5 }, 0.5f, 4, c.accent);

    // Cinema icon placeholder (film reel shape)
    Color iconBg = { (unsigned char)(c.accent.r / 5),
                     (unsigned char)(c.accent.g / 5),
                     (unsigned char)(c.accent.b / 5), 255 };
    DrawRectangle((int)cx + 16, (int)drawY + 14, cardW - 32, 90, iconBg);
    DrawRectangleLines((int)cx + 16, (int)drawY + 14, cardW - 32, 90, c.accent);

    // Reel circles
    float rCX = cx + cardW / 2.0f;
    float rCY = drawY + 14 + 45;
    DrawCircle((int)rCX, (int)rCY, 28, { c.accent.r, c.accent.g, c.accent.b, 40 });
    DrawCircleLines((int)rCX, (int)rCY, 28, c.accent);
    DrawCircle((int)rCX, (int)rCY, 10, { c.accent.r, c.accent.g, c.accent.b, 80 });
    DrawCircleLines((int)rCX, (int)rCY, 10, c.accent);
    // Spokes
    for (int sp = 0; sp < 6; sp++)
    {
        float angle = sp * 3.14159f / 3.0f;
        DrawLineEx(
            { rCX + cosf(angle) * 12, rCY + sinf(angle) * 12 },
            { rCX + cosf(angle) * 26, rCY + sinf(angle) * 26 },
            1.5f, { c.accent.r, c.accent.g, c.accent.b, 160 });
    }

    // Screen count badge
    char scBuf[16]; snprintf(scBuf, 16, "%d SCREENS", (int)c.screens.size());
    Vector2 scSz = MeasureTextEx(font, scBuf, 9, 1);
    DrawRectangleRounded({ cx + 16, drawY + 14 + 90 - 20, scSz.x + 10, 16 }, 0.4f, 4,
        { c.accent.r, c.accent.g, c.accent.b, 40 });
    DrawTextEx(font, scBuf, { cx + 21, drawY + 14 + 90 - 17 }, 9, 1, c.accent);

    // Name
    std::string nameStr = c.name;
    if (nameStr.size() > 18) nameStr = nameStr.substr(0, 16) + "..";
    DrawTextEx(font, nameStr.c_str(), { cx + 16, drawY + 112 }, 11, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // Distance
    char distBuf[32]; snprintf(distBuf, 32, "@ %s", c.distance);
    DrawTextEx(font, distBuf, { cx + 16, drawY + 128 }, 10, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // Rating
    char rBuf[16]; snprintf(rBuf, 16, "* %.1f", c.rating);
    DrawTextEx(font, rBuf, { cx + 16, drawY + 143 }, 10, 1, { 255, 200, 50, PA });

    // Hours
    DrawTextEx(font, c.hours, { cx + 16, drawY + 158 }, 9, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, (unsigned char)(160 * panelAlpha) });

    // Amenity dots
    float dotX = cx + 16;
    float dotY = drawY + 174;
    if (c.parkingAvailable)
    {
        DrawCircle((int)dotX + 4, (int)dotY + 4, 4, { 80, 220, 120, PA });
        DrawTextEx(font, "P", { dotX + 1, dotY }, 8, 1, { 20, 20, 30, PA });
        dotX += 18;
    }
    if (c.accessibleSeating)
    {
        DrawCircle((int)dotX + 4, (int)dotY + 4, 4, { 80, 160, 255, PA });
        DrawTextEx(font, "A", { dotX + 1, dotY }, 8, 1, { 20, 20, 30, PA });
    }

    // View button
    Rectangle viewBtn = { cx + 16, drawY + (float)cardH - 34, (float)cardW - 32, 26 };
    bool hoverView = CheckCollisionPointRec(mouse, viewBtn);
    Color viewBg = hoverView
        ? Color{ c.accent.r, c.accent.g, c.accent.b, 220 }
    : Color{ c.accent.r, c.accent.g, c.accent.b, 140 };
    DrawRectangleRounded(viewBtn, 0.35f, 6, viewBg);
    Vector2 vts = MeasureTextEx(font, "VIEW CINEMA", 10, 1);
    DrawTextEx(font, "VIEW CINEMA",
        { viewBtn.x + viewBtn.width / 2 - vts.x / 2,
          viewBtn.y + viewBtn.height / 2 - vts.y / 2 },
        10, 1, WHITE);
}

// Draw detail view
static void Cin_DrawDetail(Font font, int idx, int screenW, int screenH,
    float slideOffset, float time, Vector2 mouse, bool clicked,
    float dt, float panelAlpha)
{
    const Cinema& c = cin_cinemas[idx];
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);
    float ox = slideOffset;

    int panelX = (int)(40 + ox);
    int panelY = 84;
    int panelW = screenW - 80;
    int panelH = screenH - 130;

    // Shadow + panel
    DrawRectangle(panelX + 5, panelY + 8, panelW, panelH, { 0, 0, 0, 60 });
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW,(float)panelH },
        0.04f, 10, BG_CARD);
    DrawRectangleRoundedLines({ (float)panelX,(float)panelY,(float)panelW,(float)panelH },
        0.04f, 10, BORDER_NORMAL);
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW, 6 },
        0.04f, 4, c.accent);

    int leftX = panelX + 32;
    int topY = panelY + 28;

    // Left: large reel illustration
    Color iconBg = { (unsigned char)(c.accent.r / 5),
                     (unsigned char)(c.accent.g / 5),
                     (unsigned char)(c.accent.b / 5), 255 };
    DrawRectangle(leftX, topY, 200, 200, iconBg);
    DrawRectangleLines(leftX, topY, 200, 200, c.accent);

    float rCX = leftX + 100.0f;
    float rCY = topY + 100.0f;
    DrawCircle((int)rCX, (int)rCY, 70, { c.accent.r, c.accent.g, c.accent.b, 30 });
    DrawCircleLines((int)rCX, (int)rCY, 70, c.accent);
    DrawCircle((int)rCX, (int)rCY, 24, { c.accent.r, c.accent.g, c.accent.b, 60 });
    DrawCircleLines((int)rCX, (int)rCY, 24, c.accent);
    DrawCircle((int)rCX, (int)rCY, 8, BG_CARD);
    for (int sp = 0; sp < 6; sp++)
    {
        float angle = sp * 3.14159f / 3.0f + time * 0.3f;
        DrawLineEx(
            { rCX + cosf(angle) * 26, rCY + sinf(angle) * 26 },
            { rCX + cosf(angle) * 66, rCY + sinf(angle) * 66 },
            2.0f, { c.accent.r, c.accent.g, c.accent.b, 180 });
    }
    // Film strip ticks along border
    for (int tk = 0; tk < 8; tk++)
    {
        DrawRectangle(leftX + 4 + tk * 24, topY + 4, 16, 10,
            { c.accent.r, c.accent.g, c.accent.b, 60 });
        DrawRectangle(leftX + 4 + tk * 24, topY + 186, 16, 10,
            { c.accent.r, c.accent.g, c.accent.b, 60 });
    }

    // Badges below illustration
    Vector2 gs = MeasureTextEx(font, c.distance, 11, 1);
    DrawRectangleRounded({ (float)leftX, (float)(topY + 210), gs.x + 14, 20 }, 0.4f, 4,
        { c.accent.r, c.accent.g, c.accent.b, 50 });
    DrawTextEx(font, c.distance, { (float)(leftX + 7), (float)(topY + 214) }, 11, 1, c.accent);

    char scBuf[16]; snprintf(scBuf, 16, "%d SCREENS", (int)c.screens.size());
    Vector2 ss = MeasureTextEx(font, scBuf, 11, 1);
    DrawRectangleRounded({ (float)(leftX + (int)gs.x + 20), (float)(topY + 210), ss.x + 14, 20 },
        0.4f, 4, { 60, 60, 80, 180 });
    DrawTextEx(font, scBuf, { (float)(leftX + (int)gs.x + 27), (float)(topY + 214) }, 11, 1,
        TEXT_SECONDARY);

    // Rating + hours
    char rBuf[32]; snprintf(rBuf, 32, "* %.1f / 5.0", c.rating);
    DrawTextEx(font, rBuf, { (float)leftX, (float)(topY + 242) }, 13, 1, { 255, 200, 50, 255 });
    DrawTextEx(font, c.hours, { (float)leftX, (float)(topY + 264) }, 12, 1, TEXT_SECONDARY);

    // Amenities
    int amY = topY + 288;
    DrawTextEx(font, "AMENITIES", { (float)leftX, (float)amY }, 10, 1, TEXT_MUTED);
    amY += 16;
    if (c.parkingAvailable)
    {
        DrawCircle(leftX + 5, amY + 6, 5, { 80, 220, 120, PA });
        DrawTextEx(font, "PARKING AVAILABLE", { (float)(leftX + 14), (float)amY }, 11, 1,
            TEXT_SECONDARY);
        amY += 20;
    }
    if (c.accessibleSeating)
    {
        DrawCircle(leftX + 5, amY + 6, 5, { 80, 160, 255, PA });
        DrawTextEx(font, "ACCESSIBLE SEATING", { (float)(leftX + 14), (float)amY }, 11, 1,
            TEXT_SECONDARY);
    }

    // Right: name, address, screens table
    int rightX = leftX + 240;
    int rightW = panelW - 280;

    DrawTextEx(font, c.name, { (float)rightX, (float)topY }, 22, 1.5f, TEXT_PRIMARY);
    DrawRectangle(rightX, topY + 34, rightW - 40, 1, BORDER_NORMAL);

    DrawTextEx(font, "ADDRESS", { (float)rightX, (float)(topY + 46) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, c.address, { (float)rightX, (float)(topY + 62) }, 13, 0.5f, TEXT_PRIMARY);

    // Screens header
    int tblY = topY + 100;
    DrawTextEx(font, "SCREENS & CAPACITY", { (float)rightX, (float)tblY }, 11, 1, TEXT_SECONDARY);
    DrawRectangle(rightX, tblY + 16, rightW - 40, 1, BORDER_NORMAL);
    tblY += 24;

    // Column headers
    DrawTextEx(font, "SCREEN", { (float)rightX,          (float)tblY }, 10, 1, TEXT_MUTED);
    DrawTextEx(font, "TYPE", { (float)(rightX + 120),  (float)tblY }, 10, 1, TEXT_MUTED);
    DrawTextEx(font, "SEATS", { (float)(rightX + 240),  (float)tblY }, 10, 1, TEXT_MUTED);
    DrawTextEx(font, "STATUS", { (float)(rightX + 320),  (float)tblY }, 10, 1, TEXT_MUTED);
    tblY += 16;
    DrawRectangle(rightX, tblY, rightW - 40, 1, BORDER_NORMAL);
    tblY += 6;

    // Scroll wheel for screens list
    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        cin_detailScrollTarget -= wheel * 40.0f;
        float maxScroll = std::max(0.0f, (float)c.screens.size() * 44 - (panelH - (tblY - panelY) - 60));
        if (cin_detailScrollTarget < 0) cin_detailScrollTarget = 0;
        if (cin_detailScrollTarget > maxScroll) cin_detailScrollTarget = maxScroll;
    }
    cin_detailScrollY += (cin_detailScrollTarget - cin_detailScrollY) * dt * 12.0f;

    BeginScissorMode(rightX, tblY, rightW - 40, panelH - (tblY - panelY) - 60);

    for (int si = 0; si < (int)c.screens.size(); si++)
    {
        const CinScreen& sc = c.screens[si];
        float rowY = (float)tblY + si * 44 - cin_detailScrollY;

        Rectangle rowRect = { (float)rightX, rowY, (float)(rightW - 40), 38 };
        bool rowHov = CheckCollisionPointRec(mouse, rowRect);
        if (rowHov)
            DrawRectangleRounded(rowRect, 0.1f, 4,
                { 30, 35, 60, (unsigned char)(100 * panelAlpha) });

        Color typeCol = Cin_ScreenTypeColor(sc.type);

        DrawTextEx(font, sc.name,
            { (float)rightX, rowY + 12 }, 12, 1, TEXT_PRIMARY);

        // Type badge
        Vector2 tpSz = MeasureTextEx(font, sc.type, 10, 1);
        DrawRectangleRounded({ (float)(rightX + 120), rowY + 8, tpSz.x + 12, 20 },
            0.4f, 4, { typeCol.r, typeCol.g, typeCol.b, 40 });
        DrawTextEx(font, sc.type, { (float)(rightX + 126), rowY + 11 }, 10, 1, typeCol);

        char seatBuf[16]; snprintf(seatBuf, 16, "%d", sc.seats);
        DrawTextEx(font, seatBuf,
            { (float)(rightX + 240), rowY + 12 }, 12, 1, TEXT_SECONDARY);

        // Status dot (always "OPEN" for static data)
        DrawCircle(rightX + 330, (int)(rowY + 18), 5, { 80, 220, 120, PA });
        DrawTextEx(font, "OPEN", { (float)(rightX + 340), rowY + 12 }, 11, 1,
            { 80, 220, 120, PA });

        DrawRectangle(rightX, (int)(rowY + 40), rightW - 40, 1, BORDER_NORMAL);
    }

    EndScissorMode();

    // Get Directions button
    Rectangle dirBtn = { (float)rightX, (float)(panelY + panelH - 58), (float)(rightW - 40), 44 };
    bool hoverDir = CheckCollisionPointRec(mouse, dirBtn);
    Color dirBg = hoverDir
        ? Color{ c.accent.r, c.accent.g, c.accent.b, 220 }
    : Color{ c.accent.r, c.accent.g, c.accent.b, 140 };
    DrawRectangleRounded(dirBtn, 0.25f, 8, dirBg);
    Vector2 dts = MeasureTextEx(font, "GET DIRECTIONS", 13, 1);
    DrawTextEx(font, "GET DIRECTIONS",
        { dirBtn.x + dirBtn.width / 2 - dts.x / 2,
          dirBtn.y + dirBtn.height / 2 - dts.y / 2 },
        13, 1, WHITE);

    if (clicked && hoverDir)
        Cin_ShowToast(std::string("DIRECTIONS TO ") + c.name + " COPIED");

    // Back button
    Rectangle backBtn = { (float)(panelX + 12), (float)(panelY + 12), 110, 34 };
    bool hoverBack = CheckCollisionPointRec(mouse, backBtn);
    DrawRectangleRounded(backBtn, 0.3f, 6,
        hoverBack ? Color{ 72, 130, 255, 200 } : Color{ 30, 35, 60, 220 });
    DrawRectangleRoundedLines(backBtn, 0.3f, 6,
        hoverBack ? ACCENT : BORDER_NORMAL);
    Vector2 bkSz = MeasureTextEx(font, "< BACK", 13, 1);
    DrawTextEx(font, "< BACK",
        { backBtn.x + backBtn.width / 2 - bkSz.x / 2,
          backBtn.y + backBtn.height / 2 - bkSz.y / 2 },
        13, 1, hoverBack ? WHITE : TEXT_PRIMARY);

    if (clicked && hoverBack)
    {
        cin_selectedCinema = -1;
        cin_detailScrollY = 0;
        cin_detailScrollTarget = 0;
    }
}

AppState cinemasScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    if (!cin_particlesInit) Cin_InitParticles(screenW, screenH);

    if (cin_entranceTimer < CIN_ENTER_DURATION) cin_entranceTimer += dt;
    float enterT = Cin_EaseOutCubic(cin_entranceTimer / CIN_ENTER_DURATION);
    float panelAlpha = enterT;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    Cin_UpdateParticles(dt, screenW, screenH);

    if (cin_toastTimer > 0) cin_toastTimer -= dt;

    // Detail slide animation
    float targetSlide = (cin_selectedCinema >= 0) ? 0.0f : (float)screenW;
    cin_detailSlide += (targetSlide - cin_detailSlide) * dt * 16.0f;

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Nav
    int navH = 64;
    cin_pendingNav = CINEMAS;
    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        Rectangle navRect = { navX, 0, 130, (float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
        {
            cin_activeNav = i;
            cin_entranceTimer = 0.0f;
            AppState targets[] = { MAIN, CINEMAS, TICKETS, PROFILE };
            cin_pendingNav = targets[i];
        }
    }

    // Search input
    Rectangle searchBox = { (float)(screenW - 560), (float)(navH / 2 - 16), 220, 32 };
    if (clicked) cin_searchActive = CheckCollisionPointRec(mouse, searchBox);
    if (cin_searchActive && cin_selectedCinema < 0)
    {
        if (IsKeyPressed(KEY_BACKSPACE) && !cin_searchQuery.empty())
            cin_searchQuery.pop_back();
        int k = GetCharPressed();
        while (k > 0)
        {
            if (cin_searchQuery.size() < 32) cin_searchQuery += (char)k;
            k = GetCharPressed();
        }
    }

    // Card hover detection
    int   navH2 = navH;
    int   filterBarH = 36;
    int   listY = navH2 + filterBarH + 14 + 12;
    int   cardW = 210;
    int   cardH = 250;
    int   cardSpacing = 20;
    int   cardsStartX = 36;

    std::vector<int> filtered = Cin_FilteredIndices();

    cin_hoveredCinema = -1;
    if (cin_selectedCinema < 0)
    {
        for (int fi = 0; fi < (int)filtered.size(); fi++)
        {
            float cx = (float)(cardsStartX + fi * (cardW + cardSpacing));
            float cy = (float)listY;
            Rectangle cr = { cx, cy, (float)cardW, (float)cardH };
            if (CheckCollisionPointRec(mouse, cr))
            {
                cin_hoveredCinema = fi;
                if (clicked)
                {
                    cin_selectedCinema = filtered[fi];
                    cin_detailScrollY = 0;
                    cin_detailScrollTarget = 0;
                    cin_entranceTimer = 0.0f;
                }
            }
        }
    }

    // Logout
    Rectangle logoutBtn = { (float)(screenW - 105), (float)(navH / 2 - 14), 88, 28 };
    bool      hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);
    if (clicked && hoverLogout)
    {
        cin_selectedCinema = -1;
        cin_searchQuery = "";
        cin_searchActive = false;
        cin_entranceTimer = 0.0f;
        cin_particlesInit = false;
        sessionUser.username = "";
        sessionUser.email = "";
        EndDrawing();
        return AUTH;
    }

    BeginDrawing();
    ClearBackground(BG_DARK);

    // Particles
    for (int i = 0; i < CIN_PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(cin_particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)cin_particles[i].x, (int)cin_particles[i].y,
            cin_particles[i].r, { 100, 140, 255, pa });
    }

    // Ambient blobs
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

    // Nav bar
    DrawRectangle(0, 0, screenW, navH, { 10, 12, 28, 220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "Gekoya", { 32, (float)(navH / 2) - 11 }, 22, 1.5f,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        float navY = (float)(navH / 2) - 7;
        bool  isA = (cin_activeNav == i);
        DrawTextEx(font, cin_navItems[i], { navX, navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,   TEXT_PRIMARY.g,   TEXT_PRIMARY.b,   PA }
        : Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
        if (isA) DrawRectangle((int)navX, navH - 2,
            (int)MeasureTextEx(font, cin_navItems[i], 12, 1).x, 2, ACCENT);
    }

    // Search box
    DrawRectangleRounded(searchBox, 0.3f, 6, BG_INPUT);
    DrawRectangleRoundedLines(searchBox, 0.3f, 6, cin_searchActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (cin_searchQuery.empty() && !cin_searchActive)
        DrawTextEx(font, "SEARCH CINEMAS...", { searchBox.x + 10, searchBox.y + 9 }, 11, 1, TEXT_MUTED);
    else
        DrawTextEx(font, cin_searchQuery.c_str(), { searchBox.x + 10, searchBox.y + 9 }, 11, 1, TEXT_PRIMARY);
    if (cin_searchActive && ((int)(time * 2)) % 2 == 0)
    {
        float curX = searchBox.x + 10 + MeasureTextEx(font, cin_searchQuery.c_str(), 11, 1).x + 2;
        DrawRectangle((int)curX, (int)searchBox.y + 6, 2, 20, BORDER_FOCUS);
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

    // Filter / count bar
    int filterBarY = navH + 12;
    char countBuf[32];
    snprintf(countBuf, 32, "%d CINEMAS", (int)filtered.size());
    Vector2 cbSz = MeasureTextEx(font, countBuf, 11, 1);
    DrawTextEx(font, countBuf,
        { (float)(screenW - cbSz.x - 32), (float)(filterBarY + 8) }, 11, 1, TEXT_SECONDARY);

    // Nearby label
    DrawTextEx(font, "CINEMAS NEAR YOU",
        { 36, (float)(filterBarY + 8) }, 11, 1, TEXT_SECONDARY);

    // Card list
    if (cin_selectedCinema < 0 || cin_detailSlide > screenW * 0.05f)
    {
        BeginScissorMode(0, listY - 4, screenW, screenH - listY - 34);

        if (filtered.empty())
        {
            Vector2 noSz = MeasureTextEx(font, "NO CINEMAS FOUND", 18, 1);
            DrawTextEx(font, "NO CINEMAS FOUND",
                { (float)(screenW / 2) - noSz.x / 2, (float)(screenH / 2) - 40 },
                18, 1, TEXT_MUTED);
        }
        else
        {
            for (int fi = 0; fi < (int)filtered.size(); fi++)
            {
                int   idx = filtered[fi];
                float cx = (float)(cardsStartX + fi * (cardW + cardSpacing));
                float cy = (float)listY;
                if (cx + cardW < 0 || cx > screenW) continue;
                Cin_DrawCard(font, idx, cx, cy, cardW, cardH,
                    (cin_hoveredCinema == fi), (cin_selectedCinema == idx),
                    mouse, clicked, dt, panelAlpha);
            }
        }

        EndScissorMode();
    }

    // Detail view
    if (cin_selectedCinema >= 0 || cin_detailSlide < screenW * 0.95f)
    {
        Cin_DrawDetail(font, cin_selectedCinema >= 0 ? cin_selectedCinema : 0,
            screenW, screenH, cin_detailSlide, time, mouse, clicked, dt, panelAlpha);
    }

    // Status bar
    int barY = screenH - 34;
    DrawRectangle(0, barY, screenW, 34, { 10, 12, 28, 210 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);

    char cinBuf[32];  snprintf(cinBuf, 32, "%d CINEMAS NEARBY", (int)cin_cinemas.size());
    char scrBuf[32];  snprintf(scrBuf, 32, "%d TOTAL SCREENS", 14);
    char seaBuf[32];  snprintf(seaBuf, 32, "1,020 SEATS TOTAL");
    DrawTextEx(font, cinBuf, { 32,  (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, scrBuf, { 220, (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, seaBuf, { 380, (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    float dotP = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotP * 75);
    DrawCircle(screenW - 120, barY + 17, 5, { 80, 220, 120, dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110), (float)(barY + 10) }, 11, 1,
        { 80, 220, 120, 255 });

    // Toast
    if (cin_toastTimer > 0.0f)
    {
        float fadeIn = std::min(1.0f, (CIN_TOAST_DURATION - cin_toastTimer) / 0.2f);
        float fadeOut = std::min(1.0f, cin_toastTimer / 0.3f);
        float alpha = std::min(fadeIn, fadeOut);
        unsigned char ta = (unsigned char)(alpha * 240.0f);

        Vector2 tSz = MeasureTextEx(font, cin_toastMsg.c_str(), 12, 1);
        float tw = tSz.x + 32, th = 42;
        float tx = (float)(screenW / 2) - tw / 2, ty = (float)(barY - th - 12);

        DrawRectangleRounded({ tx, ty, tw, th }, 0.3f, 8, { 20, 120, 60, ta });
        DrawRectangleRoundedLines({ tx, ty, tw, th }, 0.3f, 8, { 60, 200, 100, ta });
        DrawTextEx(font, cin_toastMsg.c_str(), { tx + 16, ty + 13 }, 12, 1,
            { 200, 255, 220, ta });
    }

    EndDrawing();

    AppState ret = cin_pendingNav;
    cin_pendingNav = CINEMAS;
    return ret;
}