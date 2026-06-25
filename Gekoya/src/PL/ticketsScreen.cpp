#include "ticketsScreen.h"
#include "../colors.h"
#include "../BLL/BookingService.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// Nav
static const char* tkt_navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int         tkt_activeNav = 2;
static AppState    tkt_pendingNav = TICKETS;

// Entrance
static float       tkt_entranceTimer = 0.0f;
static const float TKT_ENTER_DURATION = 0.55f;
static float Tkt_EaseOutCubic(float t) { float inv = 1.0f - t; return 1.0f - inv * inv * inv; }

// Particles
static const int TKT_PARTICLE_COUNT = 55;
struct TktParticle { float x, y, vx, vy, r, alpha; };
static TktParticle tkt_particles[TKT_PARTICLE_COUNT];
static bool        tkt_particlesInit = false;

static void Tkt_InitParticles(int w, int h)
{
    for (int i = 0; i < TKT_PARTICLE_COUNT; i++)
    {
        tkt_particles[i].x = (float)GetRandomValue(0, w);
        tkt_particles[i].y = (float)GetRandomValue(0, h);
        tkt_particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        tkt_particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f;
        tkt_particles[i].r = (float)GetRandomValue(1, 3);
        tkt_particles[i].alpha = (float)GetRandomValue(20, 70) / 255.0f;
    }
    tkt_particlesInit = true;
}

static void Tkt_UpdateParticles(float dt, int w, int h)
{
    for (int i = 0; i < TKT_PARTICLE_COUNT; i++)
    {
        tkt_particles[i].x += tkt_particles[i].vx * dt * 60.0f;
        tkt_particles[i].y += tkt_particles[i].vy * dt * 60.0f;
        if (tkt_particles[i].y < -4)    tkt_particles[i].y = (float)h + 4;
        if (tkt_particles[i].x < -4)    tkt_particles[i].x = (float)w + 4;
        if (tkt_particles[i].x > w + 4) tkt_particles[i].x = -4.0f;
    }
}

// Toast
static std::string tkt_toastMsg = "";
static float       tkt_toastTimer = 0.0f;
static const float TKT_TOAST_DURATION = 2.8f;
static void Tkt_ShowToast(const std::string& msg) { tkt_toastMsg = msg; tkt_toastTimer = TKT_TOAST_DURATION; }

// Ripple
static bool  tkt_rippleActive = false;
static float tkt_rippleTimer = 0.0f;
static float tkt_rippleX = 0, tkt_rippleY = 0;
static const float TKT_RIPPLE_DURATION = 0.5f;
static const float TKT_RIPPLE_MAX_R = 120.0f;

// Color lerp
static Color Tkt_LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

// Booking data state
static std::vector<Booking> tkt_bookings;
static bool                 tkt_loaded = false;
static std::string          tkt_loadedFor = "";  // username we loaded for

// UI state
static int   tkt_selectedTicket = -1;   // index into tkt_bookings
static float tkt_scrollY = 0.0f;
static float tkt_scrollTarget = 0.0f;

// Confirm cancel dialog
static bool  tkt_confirmCancel = false;
static int   tkt_cancelIndex = -1;
static float tkt_confirmAlpha = 0.0f;

// Hover tracking per row (up to 64 tickets)
static float tkt_rowHoverLerp[64] = {};

// Seat type colour
static Color Tkt_SeatColor(const std::string& seatType)
{
    if (seatType == "PLATINUM") return { 180,  80, 255, 255 };
    if (seatType == "GOLD")     return { 220, 180,  50, 255 };
    return                             { 160, 160, 180, 255 }; // SILVER
}

// Seat type price (matches mainScreen)
static int Tkt_SeatPrice(const std::string& seatType)
{
    if (seatType == "PLATINUM") return 22;
    if (seatType == "GOLD")     return 14;
    return 8; // SILVER
}

// Draw a single ticket row
static void Tkt_DrawTicketRow(Font font, int idx, const Booking& b,
    float rx, float ry, float rw, float rh,
    bool hovered, bool selected, float hoverLerp,
    Vector2 mouse, bool clicked, float dt,
    unsigned char PA)
{
    Color seatCol = Tkt_SeatColor(b.seatType);

    // Animated hover lift
    float lift = hoverLerp * -4.0f;

    // Shadow
    DrawRectangleRounded({ rx + 3, ry + lift + 6, rw, rh }, 0.08f, 8,
        { 0, 0, 0, (unsigned char)(hoverLerp * 60.0f) });

    // Card background
    Color cardBg = selected
        ? Color{ (unsigned char)(seatCol.r / 6 + 10),
                 (unsigned char)(seatCol.g / 6 + 10),
                 (unsigned char)(seatCol.b / 6 + 10), PA }
    : BG_CARD;
    Color cardBdr = selected ? seatCol : (hovered ? BORDER_FOCUS : BORDER_NORMAL);

    DrawRectangleRounded({ rx, ry + lift, rw, rh }, 0.08f, 8, cardBg);
    DrawRectangleRoundedLines({ rx, ry + lift, rw, rh }, 0.08f, 8, cardBdr);

    // Left accent bar (seat type colour)
    DrawRectangleRounded({ rx, ry + lift, 5, rh }, 0.5f, 4, seatCol);

    // Ticket stub perforation line
    float perfX = rx + rw - 120.0f;
    for (float dy = ry + lift + 8; dy < ry + lift + rh - 8; dy += 8)
        DrawRectangle((int)perfX, (int)dy, 1, 4,
            { BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, (unsigned char)(80 * PA / 255) });

    // Left section: movie title + date
    int lx = (int)rx + 20;
    int ly = (int)(ry + lift) + 14;

    // Movie title
    std::string title = b.movieTitle;
    if (title.size() > 28) title = title.substr(0, 26) + "..";
    DrawTextEx(font, title.c_str(), { (float)lx, (float)ly }, 14, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    // Showtime
    char showBuf[64];
    snprintf(showBuf, 64, "%s", b.showtime.c_str());
    DrawTextEx(font, showBuf, { (float)lx, (float)(ly + 22) }, 11, 1,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    // Booking date
    DrawTextEx(font, b.bookingDate.c_str(), { (float)lx, (float)(ly + 38) }, 10, 1,
        { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, PA });

    // Middle section: seat type badge
    float midX = rx + rw * 0.5f - 50;
    float midY = ry + lift + rh / 2.0f - 18;

    Vector2 stSz = MeasureTextEx(font, b.seatType.c_str(), 12, 1);
    DrawRectangleRounded({ midX, midY, stSz.x + 18, 22 }, 0.4f, 6,
        { seatCol.r, seatCol.g, seatCol.b, (unsigned char)(50 * PA / 255) });
    DrawRectangleRoundedLines({ midX, midY, stSz.x + 18, 22 }, 0.4f, 6,
        { seatCol.r, seatCol.g, seatCol.b, PA });
    DrawTextEx(font, b.seatType.c_str(), { midX + 9, midY + 4 }, 12, 1,
        { seatCol.r, seatCol.g, seatCol.b, PA });

    // Price below badge
    char priceBuf[16]; snprintf(priceBuf, 16, "$%d", b.price);
    Vector2 prSz = MeasureTextEx(font, priceBuf, 13, 1);
    DrawTextEx(font, priceBuf,
        { midX + (stSz.x + 18) / 2 - prSz.x / 2, midY + 28 }, 13, 1,
        { seatCol.r, seatCol.g, seatCol.b, PA });

    // Right stub: barcode + cancel button
    float stubX = perfX + 10;
    float stubW = rw - (stubX - rx) - 10;
    float stubCX = stubX + stubW / 2;

    // Barcode visual (decorative vertical lines)
    int   barCount = 18;
    float barStart = stubX + 4;
    float barH = rh - 36.0f;
    float barY2 = ry + lift + 8;
    for (int bi = 0; bi < barCount; bi++)
    {
        float bx = barStart + bi * (stubW - 8) / (float)barCount;
        int   bw = (bi % 3 == 0) ? 3 : 1;
        DrawRectangle((int)bx, (int)barY2, bw, (int)barH,
            { seatCol.r, seatCol.g, seatCol.b, (unsigned char)(60 * PA / 255) });
    }

    // Booking ID (truncated)
    std::string shortId = b.id.size() > 8 ? b.id.substr(b.id.size() - 8) : b.id;
    std::string idLabel = "#" + shortId;
    Vector2 idSz = MeasureTextEx(font, idLabel.c_str(), 9, 1);
    DrawTextEx(font, idLabel.c_str(),
        { stubCX - idSz.x / 2, ry + lift + rh - 20 }, 9, 1,
        { TEXT_MUTED.r, TEXT_MUTED.g, TEXT_MUTED.b, (unsigned char)(120 * PA / 255) });

    // Cancel button (small, red, bottom-right of stub)
    Rectangle cancelBtn = { stubCX - 28, ry + lift + rh - 36, 56, 20 };
    bool hoverCancel = CheckCollisionPointRec(mouse, cancelBtn);
    DrawRectangleRounded(cancelBtn, 0.3f, 4,
        hoverCancel ? Color{ 200, 60, 60, PA } : Color{ 80, 30, 30, (unsigned char)(180 * PA / 255) });
    DrawRectangleRoundedLines(cancelBtn, 0.3f, 4,
        hoverCancel ? Color{ 240, 90, 90, PA } : Color{ 140, 50, 50, PA });
    Vector2 cSz = MeasureTextEx(font, "CANCEL", 9, 1);
    DrawTextEx(font, "CANCEL",
        { cancelBtn.x + cancelBtn.width / 2 - cSz.x / 2,
          cancelBtn.y + cancelBtn.height / 2 - cSz.y / 2 },
        9, 1, { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });
}

// Draw confirm cancel overlay
static bool Tkt_DrawConfirmDialog(Font font, int screenW, int screenH,
    float alpha, Vector2 mouse, bool clicked)
{
    if (alpha < 0.01f) return false;
    unsigned char a = (unsigned char)(alpha * 200.0f);
    unsigned char ta = (unsigned char)(alpha * 255.0f);

    // Dim background
    DrawRectangle(0, 0, screenW, screenH, { 0, 0, 0, a });

    float dw = 420, dh = 160;
    float dx = screenW / 2.0f - dw / 2;
    float dy = screenH / 2.0f - dh / 2;

    DrawRectangle((int)dx + 4, (int)dy + 6, (int)dw, (int)dh, { 0, 0, 0, ta });
    DrawRectangleRounded({ dx, dy, dw, dh }, 0.08f, 8,
        { BG_CARD.r, BG_CARD.g, BG_CARD.b, ta });
    DrawRectangleRoundedLines({ dx, dy, dw, dh }, 0.08f, 8,
        { 200, 60, 60, ta });
    DrawRectangleRounded({ dx, dy, dw, 5 }, 0.5f, 4, { 200, 60, 60, ta });

    Vector2 hSz = MeasureTextEx(font, "CANCEL BOOKING?", 15, 1);
    DrawTextEx(font, "CANCEL BOOKING?",
        { dx + dw / 2 - hSz.x / 2, dy + 18 }, 15, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, ta });

    Vector2 sSz = MeasureTextEx(font, "This will permanently remove your booking.", 11, 0.5f);
    DrawTextEx(font, "This will permanently remove your booking.",
        { dx + dw / 2 - sSz.x / 2, dy + 44 }, 11, 0.5f,
        { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, ta });

    // Buttons
    Rectangle yesBtn = { dx + 30,        dy + dh - 54, 160, 38 };
    Rectangle noBtn = { dx + dw - 190,  dy + dh - 54, 160, 38 };

    bool hoverYes = CheckCollisionPointRec(mouse, yesBtn);
    bool hoverNo = CheckCollisionPointRec(mouse, noBtn);

    DrawRectangleRounded(yesBtn, 0.3f, 6,
        hoverYes ? Color{ 200, 60, 60, ta } : Color{ 120, 40, 40, ta });
    DrawRectangleRoundedLines(yesBtn, 0.3f, 6, { 200, 80, 80, ta });
    Vector2 ySz = MeasureTextEx(font, "YES, CANCEL", 12, 1);
    DrawTextEx(font, "YES, CANCEL",
        { yesBtn.x + yesBtn.width / 2 - ySz.x / 2,
          yesBtn.y + yesBtn.height / 2 - ySz.y / 2 }, 12, 1, { 255, 255, 255, ta });

    DrawRectangleRounded(noBtn, 0.3f, 6,
        hoverNo ? Color{ 50, 60, 90, ta } : Color{ 30, 35, 60, ta });
    DrawRectangleRoundedLines(noBtn, 0.3f, 6, { BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, ta });
    Vector2 nSz = MeasureTextEx(font, "KEEP TICKET", 12, 1);
    DrawTextEx(font, "KEEP TICKET",
        { noBtn.x + noBtn.width / 2 - nSz.x / 2,
          noBtn.y + noBtn.height / 2 - nSz.y / 2 }, 12, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, ta });

    if (clicked)
    {
        if (hoverYes) return true;   // confirmed
        if (hoverNo) { tkt_confirmCancel = false; tkt_cancelIndex = -1; }
    }
    return false;
}

AppState ticketsScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    if (!tkt_particlesInit) Tkt_InitParticles(screenW, screenH);

    if (tkt_entranceTimer < TKT_ENTER_DURATION) tkt_entranceTimer += dt;
    float enterT = Tkt_EaseOutCubic(tkt_entranceTimer / TKT_ENTER_DURATION);
    float panelAlpha = enterT;
    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    Tkt_UpdateParticles(dt, screenW, screenH);

    if (tkt_toastTimer > 0) tkt_toastTimer -= dt;
    if (tkt_rippleActive)
    {
        tkt_rippleTimer += dt;
        if (tkt_rippleTimer >= TKT_RIPPLE_DURATION) tkt_rippleActive = false;
    }

    // Confirm dialog fade
    float confirmTarget = tkt_confirmCancel ? 1.0f : 0.0f;
    tkt_confirmAlpha += (confirmTarget - tkt_confirmAlpha) * dt * 14.0f;

    // Load bookings from DB when user changes or first load
    if (!tkt_loaded || tkt_loadedFor != sessionUser.username)
    {
        tkt_bookings = BookingService::GetUserBookings(sessionUser.username);
        tkt_loaded = true;
        tkt_loadedFor = sessionUser.username;
    }

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Nav
    int navH = 64;
    tkt_pendingNav = TICKETS;
    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        Rectangle navRect = { navX, 0, 130, (float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
        {
            tkt_activeNav = i;
            tkt_entranceTimer = 0.0f;
            AppState targets[] = { MAIN, CINEMAS, TICKETS, PROFILE };
            tkt_pendingNav = targets[i];
        }
    }

    // Layout
    int   listStartY = navH + 56;         // below nav + header row
    int   listEndY = screenH - 34 - 8; // above status bar
    int   listH = listEndY - listStartY;
    int   rowH = 100;
    int   rowSpacing = 10;
    int   rowStride = rowH + rowSpacing;
    int   rowX = 36;
    int   rowW = screenW - 72;

    // Total scrollable content height
    float totalH = (float)(tkt_bookings.size() * rowStride);
    float maxScroll = std::max(0.0f, totalH - (float)listH);

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && !tkt_confirmCancel)
    {
        tkt_scrollTarget -= wheel * 60.0f;
        if (tkt_scrollTarget < 0)          tkt_scrollTarget = 0;
        if (tkt_scrollTarget > maxScroll)  tkt_scrollTarget = maxScroll;
    }
    tkt_scrollY += (tkt_scrollTarget - tkt_scrollY) * dt * 12.0f;

    // Logout
    Rectangle logoutBtn = { (float)(screenW - 105), (float)(navH / 2 - 14), 88, 28 };
    bool      hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);
    if (clicked && hoverLogout && !tkt_confirmCancel)
    {
        tkt_loaded = false;
        tkt_loadedFor = "";
        tkt_bookings.clear();
        tkt_scrollY = 0;
        tkt_scrollTarget = 0;
        tkt_entranceTimer = 0.0f;
        tkt_particlesInit = false;
        tkt_confirmCancel = false;
        sessionUser.username = "";
        sessionUser.email = "";
        EndDrawing();
        return AUTH;
    }

    // Refresh button
    int   headerY = navH + 14;
    Rectangle refreshBtn = { (float)(screenW - 160), (float)headerY, 120, 30 };
    bool hoverRefresh = CheckCollisionPointRec(mouse, refreshBtn);
    if (clicked && hoverRefresh && !tkt_confirmCancel)
    {
        tkt_bookings = BookingService::GetUserBookings(sessionUser.username);
        tkt_loadedFor = sessionUser.username;
        Tkt_ShowToast("TICKETS REFRESHED");
    }

    // Per-row hover lerp + cancel click detection
    for (int i = 0; i < (int)tkt_bookings.size() && i < 64; i++)
    {
        float ry = (float)(listStartY + i * rowStride) - tkt_scrollY;
        if (ry + rowH < listStartY || ry > listEndY) continue;

        Rectangle rowRect = { (float)rowX, ry, (float)rowW, (float)rowH };
        bool rowHov = CheckCollisionPointRec(mouse, rowRect) && !tkt_confirmCancel;
        tkt_rowHoverLerp[i] += ((rowHov ? 1.0f : 0.0f) - tkt_rowHoverLerp[i]) * dt * 14.0f;

        if (!tkt_confirmCancel && clicked && rowHov)
        {
            // Check cancel sub-button
            // Cancel button inside stub (right ~120px)
            float perfX = (float)rowX + rowW - 120.0f;
            float stubX = perfX + 10;
            float stubW = rowW - (stubX - rowX) - 10;
            float stubCX = stubX + stubW / 2;
            Rectangle cancelBtn = { stubCX - 28, ry + rowH - 36, 56, 20 };
            if (CheckCollisionPointRec(mouse, cancelBtn))
            {
                tkt_confirmCancel = true;
                tkt_cancelIndex = i;
            }
            else
            {
                tkt_selectedTicket = (tkt_selectedTicket == i) ? -1 : i;
            }
        }
    }

    // Confirm cancel dialog result
    if (tkt_confirmAlpha > 0.05f)
    {
        bool confirmed = Tkt_DrawConfirmDialog(font, screenW, screenH,
            tkt_confirmAlpha, mouse, clicked);
        // (We draw it again after BeginDrawing, this is just for click logic)
        if (confirmed && tkt_cancelIndex >= 0 && tkt_cancelIndex < (int)tkt_bookings.size())
        {
            // will be acted on after drawing
        }
    }

    BeginDrawing();
    ClearBackground(BG_DARK);

    // Particles
    for (int i = 0; i < TKT_PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(tkt_particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)tkt_particles[i].x, (int)tkt_particles[i].y,
            tkt_particles[i].r, { 100, 140, 255, pa });
    }

    // Ambient blobs (identical to all other screens)
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
        bool  isA = (tkt_activeNav == i);
        DrawTextEx(font, tkt_navItems[i], { navX, navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,   TEXT_PRIMARY.g,   TEXT_PRIMARY.b,   PA }
        : Color{ TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });
        if (isA) DrawRectangle((int)navX, navH - 2,
            (int)MeasureTextEx(font, tkt_navItems[i], 12, 1).x, 2, ACCENT);
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

    // Page header row
    DrawTextEx(font, "MY TICKETS",
        { 36, (float)headerY + 6 }, 16, 1,
        { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, PA });

    char countBuf[32];
    snprintf(countBuf, 32, "%d BOOKING%s",
        (int)tkt_bookings.size(), tkt_bookings.size() == 1 ? "" : "S");
    Vector2 cntSz = MeasureTextEx(font, countBuf, 11, 1);
    DrawRectangleRounded({ 36 + MeasureTextEx(font, "MY TICKETS", 16, 1).x + 14,
        (float)headerY + 8, cntSz.x + 14, 20 }, 0.4f, 4,
        { 80, 130, 255, (unsigned char)(50 * panelAlpha) });
    DrawTextEx(font, countBuf,
        { 36 + MeasureTextEx(font, "MY TICKETS", 16, 1).x + 21,
          (float)headerY + 12 }, 11, 1, { 80, 130, 255, PA });

    // Refresh button
    DrawRectangleRounded(refreshBtn, 0.3f, 6,
        hoverRefresh ? Color{ 50, 60, 90, (unsigned char)(220 * panelAlpha) }
    : Color{ 30, 35, 60, (unsigned char)(180 * panelAlpha) });
    DrawRectangleRoundedLines(refreshBtn, 0.3f, 6,
        hoverRefresh ? Color{ BORDER_FOCUS.r, BORDER_FOCUS.g, BORDER_FOCUS.b, PA }
    : Color{ BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });
    Vector2 rfSz = MeasureTextEx(font, "REFRESH", 11, 1);
    DrawTextEx(font, "REFRESH",
        { refreshBtn.x + refreshBtn.width / 2 - rfSz.x / 2,
          refreshBtn.y + refreshBtn.height / 2 - rfSz.y / 2 },
        11, 1, { TEXT_SECONDARY.r, TEXT_SECONDARY.g, TEXT_SECONDARY.b, PA });

    DrawRectangle(0, listStartY - 4, screenW, 1, BORDER_NORMAL);

    // Ticket list
    BeginScissorMode(0, listStartY, screenW, listH);

    if (tkt_bookings.empty())
    {
        // Empty state
        float emCX = screenW / 2.0f;
        float emCY = listStartY + listH / 2.0f - 30;

        // Big ticket icon placeholder
        DrawRectangleRounded({ emCX - 50, emCY - 40, 100, 70 }, 0.1f, 6,
            { 25, 30, 55, PA });
        DrawRectangleRoundedLines({ emCX - 50, emCY - 40, 100, 70 }, 0.1f, 6,
            { BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, PA });
        // Stub perf line on icon
        for (float dy = emCY - 32; dy < emCY + 28; dy += 6)
            DrawRectangle((int)(emCX + 18), (int)dy, 1, 3,
                { BORDER_NORMAL.r, BORDER_NORMAL.g, BORDER_NORMAL.b, (unsigned char)(80 * PA / 255) });

        Vector2 noSz = MeasureTextEx(font, "NO TICKETS YET", 18, 1);
        DrawTextEx(font, "NO TICKETS YET",
            { emCX - noSz.x / 2, emCY + 40 }, 18, 1, TEXT_MUTED);

        Vector2 subSz = MeasureTextEx(font, "Book a movie to see your tickets here", 12, 0.5f);
        DrawTextEx(font, "Book a movie to see your tickets here",
            { emCX - subSz.x / 2, emCY + 66 }, 12, 0.5f, TEXT_MUTED);

        // Go to movies button
        Rectangle goBtn = { emCX - 80, emCY + 90, 160, 38 };
        bool hoverGo = CheckCollisionPointRec(mouse, goBtn);
        DrawRectangleRounded(goBtn, 0.3f, 8,
            hoverGo ? ACCENT_HOVER : ACCENT);
        Vector2 goSz = MeasureTextEx(font, "BROWSE MOVIES", 12, 1);
        DrawTextEx(font, "BROWSE MOVIES",
            { goBtn.x + goBtn.width / 2 - goSz.x / 2,
              goBtn.y + goBtn.height / 2 - goSz.y / 2 },
            12, 1, WHITE);
        if (clicked && hoverGo)
        {
            EndScissorMode();
            EndDrawing();
            tkt_pendingNav = MAIN;
        }
    }
    else
    {
        for (int i = 0; i < (int)tkt_bookings.size() && i < 64; i++)
        {
            float ry = (float)(listStartY + i * rowStride) - tkt_scrollY;
            if (ry + rowH < listStartY || ry > listEndY) continue;

            bool hov = tkt_rowHoverLerp[i] > 0.05f;
            bool sel = (tkt_selectedTicket == i);

            Tkt_DrawTicketRow(font, i, tkt_bookings[i],
                (float)rowX, ry, (float)rowW, (float)rowH,
                hov, sel, tkt_rowHoverLerp[i],
                mouse, clicked, dt, PA);
        }

        // Scrollbar
        if (maxScroll > 0)
        {
            float trackH = (float)listH;
            float thumbH = trackH * ((float)listH / totalH);
            float thumbY = (float)listStartY + (tkt_scrollY / maxScroll) * (trackH - thumbH);
            DrawRectangle(screenW - 6, listStartY, 4, listH, { 40, 45, 70, 120 });
            DrawRectangleRounded({ (float)(screenW - 6), thumbY, 4, thumbH },
                0.5f, 4, ACCENT);
        }
    }

    EndScissorMode();

    // Status bar
    int barY = screenH - 34;
    DrawRectangle(0, barY, screenW, 34, { 10, 12, 28, 210 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);

    // Calculate total spend
    int totalSpend = 0;
    for (auto& b : tkt_bookings) totalSpend += b.price;

    char bkBuf[32];  snprintf(bkBuf, 32, "%d BOOKINGS", (int)tkt_bookings.size());
    char spBuf[32];  snprintf(spBuf, 32, "$%d TOTAL SPENT", totalSpend);
    DrawTextEx(font, bkBuf, { 32,  (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, spBuf, { 200, (float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    float dotP = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotP * 75);
    DrawCircle(screenW - 120, barY + 17, 5, { 80, 220, 120, dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110), (float)(barY + 10) }, 11, 1,
        { 80, 220, 120, 255 });

    // Confirm cancel dialog (drawn on top of everything)
    if (tkt_confirmAlpha > 0.01f)
    {
        bool confirmed = Tkt_DrawConfirmDialog(font, screenW, screenH,
            tkt_confirmAlpha, mouse, clicked);

        if (confirmed && tkt_cancelIndex >= 0 && tkt_cancelIndex < (int)tkt_bookings.size())
        {
            std::string cancelId = tkt_bookings[tkt_cancelIndex].id;
            bool ok = BookingService::CancelBooking(cancelId);
            if (ok)
            {
                tkt_bookings.erase(tkt_bookings.begin() + tkt_cancelIndex);
                if (tkt_selectedTicket == tkt_cancelIndex) tkt_selectedTicket = -1;
                Tkt_ShowToast("BOOKING CANCELLED SUCCESSFULLY");
            }
            else
            {
                Tkt_ShowToast("ERROR: COULD NOT CANCEL BOOKING");
            }
            tkt_confirmCancel = false;
            tkt_cancelIndex = -1;
        }
    }

    // Toast
    if (tkt_toastTimer > 0.0f)
    {
        float fadeIn = std::min(1.0f, (TKT_TOAST_DURATION - tkt_toastTimer) / 0.2f);
        float fadeOut = std::min(1.0f, tkt_toastTimer / 0.3f);
        float alpha = std::min(fadeIn, fadeOut);
        unsigned char ta = (unsigned char)(alpha * 240.0f);

        Vector2 tSz = MeasureTextEx(font, tkt_toastMsg.c_str(), 12, 1);
        float tw = tSz.x + 32, th = 42;
        float tx = (float)(screenW / 2) - tw / 2, ty = (float)(barY - th - 12);

        DrawRectangleRounded({ tx, ty, tw, th }, 0.3f, 8, { 20, 120, 60, ta });
        DrawRectangleRoundedLines({ tx, ty, tw, th }, 0.3f, 8, { 60, 200, 100, ta });
        DrawTextEx(font, tkt_toastMsg.c_str(), { tx + 16, ty + 13 }, 12, 1,
            { 200, 255, 220, ta });
    }

    EndDrawing();

    AppState ret = tkt_pendingNav;
    tkt_pendingNav = TICKETS;
    return ret;
}