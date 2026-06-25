#include "mainScreen.h"
#include "../colors.h"
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <set>
#include <map>

struct Showtime { const char* time; bool available; };

struct Movie
{
    const char* title;
    const char* genre;
    const char* language;
    const char* releaseDate;
    float       rating;
    int         durationMin;
    Color       accent;
    std::vector<Showtime> shows;
    const char* posterPath;  
};

static std::vector<Movie> allMovies = {
    { "DUNE: PART TWO",  "SCI-FI",   "English", "2024-03-01", 8.5f, 166,
      {80,130,255,255}, {{"10:30",true},{"13:45",true},{"17:00",false},{"20:15",true}},
      "assets/dune2.png" },
    { "OPPENHEIMER",     "DRAMA",    "English", "2023-07-21", 8.9f, 180,
      {255,140,60,255}, {{"11:00",true},{"14:30",false},{"18:00",true},{"21:30",true}},
      "assets/oppenheimer.png" },
    { "THE BATMAN",      "ACTION",   "English", "2022-03-04", 7.8f, 176,
      {60,180,255,255}, {{"10:00",true},{"13:00",true},{"16:30",true},{"20:00",false}},
      "assets/thebatman.png" },
    { "POOR THINGS",     "FANTASY",  "English", "2023-12-08", 7.9f, 141,
      {180,80,255,255}, {{"12:00",false},{"15:15",true},{"18:30",true},{"21:45",true}},
      "assets/poorthings.png" },
    { "PAST LIVES",      "ROMANCE",  "Korean",  "2023-06-02", 7.8f, 106,
      {80,220,160,255}, {{"11:30",true},{"14:00",true},{"16:45",false},{"19:30",true}},
      "assets/pastlives.png" },
    { "KILLERS OF THE FLOWER MOON","DRAMA","English","2023-10-20",7.7f,206,
      {220,100,80,255}, {{"10:15",true},{"14:00",false},{"18:15",true},{"22:00",true}},
      "assets/killersoftheflowermoon.png" },
    { "FIGHT CLUB",   "CRIME",  "English", "2024-06-01", 7.5f, 130,
      {200,60,180,255}, {{"13:00",false},{"16:00",false},{"19:00",false},{"22:00",false}},
      "assets/poorthings2.png" },
    { "INTERSTELLAR",    "SCI-FI",   "English", "2014-11-07", 8.7f, 169,
      {100,200,255,255},{{"11:00",true},{"14:30",true},{"18:00",true},{"21:30",false}},
      "assets/interstellar.png" },
};

// ── Texture cache ────────────────────────────────────────────────────────────
static std::map<std::string, Texture2D> posterCache;

static Texture2D GetPoster(const char* path)
{
    std::string key(path);
    auto it = posterCache.find(key);
    if (it != posterCache.end()) return it->second;

    Texture2D tex = LoadTexture(path);

    if (tex.id == 0 || tex.width == 0) {
        TraceLog(LOG_WARNING, "FAILED TO LOAD TEXTURE: %s", path);
    }
    else {
        TraceLog(LOG_INFO, "SUCCESSFULLY LOADED TEXTURE: %s (%dx%d)", path, tex.width, tex.height);
    }

    posterCache[key] = tex;
    return tex;
}

static void UnloadMovieTextures()
{
    for (auto& kv : posterCache) UnloadTexture(kv.second);
    posterCache.clear();
}

static void DrawPosterFitted(Texture2D tex, Rectangle dest, Color bgColor, Color accent)
{
    DrawRectangleRec(dest, bgColor);                  

    if (tex.width <= 1 || tex.height <= 1)   
    {
        DrawRectangleLinesEx(dest, 1, accent);
        // small play icon
        float ix = dest.x + dest.width / 2 - 12;
        float iy = dest.y + dest.height / 2 - 12;
        DrawRectangle((int)ix, (int)iy, 24, 24, accent);
        DrawTriangle({ ix + 7, iy + 5 }, { ix + 7, iy + 19 }, { ix + 21, iy + 12 }, bgColor);
        return;
    }

    // Compute scale to fit while preserving aspect ratio
    float scaleX = dest.width / (float)tex.width;
    float scaleY = dest.height / (float)tex.height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float dw = tex.width * scale;
    float dh = tex.height * scale;
    float dx = dest.x + (dest.width - dw) / 2.0f;
    float dy = dest.y + (dest.height - dh) / 2.0f;

    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = { dx, dy, dw, dh };
    DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
}
// ────────────────────────────────────────────────────────────────────────────

static const int PARTICLE_COUNT = 55;
struct MainParticle { float x, y, vx, vy, r, alpha; };
static MainParticle particles[PARTICLE_COUNT];
static bool particlesInit = false;

static AppState prof_pendingNav = MAIN;

static void InitParticles(int w, int h)
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x = (float)GetRandomValue(0, w);
        particles[i].y = (float)GetRandomValue(0, h);
        particles[i].vx = (float)GetRandomValue(-30, 30) / 100.0f;
        particles[i].vy = (float)GetRandomValue(-18, -6) / 100.0f;
        particles[i].r = (float)GetRandomValue(1, 3);
        particles[i].alpha = (float)GetRandomValue(20, 70) / 255.0f;
    }
    particlesInit = true;
}

static void UpdateParticles(float dt, int w, int h)
{
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        particles[i].x += particles[i].vx * dt * 60.0f;
        particles[i].y += particles[i].vy * dt * 60.0f;
        if (particles[i].y < -4)    particles[i].y = (float)h + 4;
        if (particles[i].x < -4)    particles[i].x = (float)w + 4;
        if (particles[i].x > w + 4) particles[i].x = -4.0f;
    }
}

static float entranceTimer = 0.0f;
static const float ENTER_DURATION = 0.55f;
static float EaseOutCubic(float t) { float inv = 1.0f - t; return 1.0f - inv * inv * inv; }

static const char* navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int         activeNav = 0;
static int         hoveredCard = -1;
static int         selectedMovie = -1;

static std::string searchQuery = "";
static bool        searchActive = false;
static int         filterGenre = 0;
static const char* genreFilters[] = { "ALL","SCI-FI","DRAMA","ACTION","FANTASY","ROMANCE" };
static const int   GENRE_COUNT = 6;

static std::string toastMsg = "";
static float       toastTimer = 0.0f;
static const float TOAST_DURATION = 2.8f;

static float detailSlide = 0.0f;
static float cardScrollX = 0.0f;
static float cardScrollTarget = 0.0f;

static bool  rippleActive = false;
static float rippleTimer = 0.0f;
static float rippleX = 0, rippleY = 0;
static const float RIPPLE_DURATION = 0.5f;
static const float RIPPLE_MAX_R = 140.0f;

static bool  jumpToShowtime = false;
static float showtimePulse = 0.0f;
static bool  confirmPulseActive = false;
static float confirmPulseTimer = 0.0f;

static float cardHoverY[16] = {};

// ── Seat map state ──────────────────────────────────────────────────────────
static const int SEAT_ROWS = 8;
static const int SEAT_COLS = 12;
static const int MAX_SELECTED = 10;

struct SeatState { bool occupied; bool selected; };
static SeatState seatMap[SEAT_ROWS][2][SEAT_COLS];
static bool      seatMapInit = false;

static void InitSeatMap()
{
    for (int r = 0; r < SEAT_ROWS; r++)
        for (int s = 0; s < 2; s++)
            for (int c = 0; c < SEAT_COLS; c++)
                seatMap[r][s][c] = { false, false };

    auto occ = [](int r, int s, int c) { seatMap[r][s][c].occupied = true; };
    occ(0, 0, 9); occ(0, 0, 10); occ(0, 0, 11);
    occ(0, 1, 0); occ(0, 1, 1);
    occ(1, 0, 10); occ(1, 0, 11);
    occ(2, 1, 9); occ(2, 1, 10);
    occ(3, 0, 8); occ(3, 0, 9);
    occ(4, 0, 11); occ(4, 1, 11);
    occ(5, 0, 6); occ(5, 0, 7); occ(5, 0, 8); occ(5, 1, 6); occ(5, 1, 7);
    occ(6, 0, 9); occ(6, 0, 10); occ(6, 1, 8); occ(6, 1, 9);
    occ(7, 0, 10); occ(7, 0, 11); occ(7, 1, 10); occ(7, 1, 11);
    seatMapInit = true;
}

static void ClearSeatSelections()
{
    for (int r = 0; r < SEAT_ROWS; r++)
        for (int s = 0; s < 2; s++)
            for (int c = 0; c < SEAT_COLS; c++)
                seatMap[r][s][c].selected = false;
}

static int CountSelected()
{
    int n = 0;
    for (int r = 0; r < SEAT_ROWS; r++)
        for (int s = 0; s < 2; s++)
            for (int c = 0; c < SEAT_COLS; c++)
                if (seatMap[r][s][c].selected) n++;
    return n;
}

static int SeatPrice(int col)
{
    if (col >= 8) return 22;
    if (col >= 3) return 14;
    return 8;
}

static const char* SeatTypeName(int col)
{
    if (col >= 8) return "PLATINUM";
    if (col >= 3) return "GOLD";
    return "SILVER";
}

static int TotalPrice()
{
    int total = 0;
    for (int r = 0; r < SEAT_ROWS; r++)
        for (int s = 0; s < 2; s++)
            for (int c = 0; c < SEAT_COLS; c++)
                if (seatMap[r][s][c].selected)
                    total += SeatPrice(c);
    return total;
}

static Color SeatZoneColor(int col, bool selected, bool hovered, Color accent)
{
    if (selected) return accent;
    if (col >= 8)
        return hovered ? Color{ 160,80,230,220 } : Color{ 120,50,180,160 };
    if (col >= 3)
        return hovered ? Color{ 230,190,60,220 } : Color{ 180,140,40,160 };
    return hovered ? Color{ 170,175,200,220 } : Color{ 110,115,140,160 };
}
// ────────────────────────────────────────────────────────────────────────────

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

static void ShowToast(const std::string& msg) { toastMsg = msg; toastTimer = TOAST_DURATION; }

static bool IsSoldOut(const Movie& m)
{
    for (auto& s : m.shows) if (s.available) return false;
    return true;
}

static std::vector<int> FilteredIndices()
{
    std::vector<int> result;
    for (int i = 0; i < (int)allMovies.size(); i++)
    {
        const Movie& m = allMovies[i];
        if (filterGenre > 0 && std::string(m.genre) != genreFilters[filterGenre]) continue;
        if (!searchQuery.empty())
        {
            std::string q = searchQuery;
            std::transform(q.begin(), q.end(), q.begin(), ::toupper);
            std::string t = m.title;   std::transform(t.begin(), t.end(), t.begin(), ::toupper);
            std::string g = m.genre;   std::transform(g.begin(), g.end(), g.begin(), ::toupper);
            std::string l = m.language; std::transform(l.begin(), l.end(), l.begin(), ::toupper);
            if (t.find(q) == std::string::npos && g.find(q) == std::string::npos && l.find(q) == std::string::npos)
                continue;
        }
        result.push_back(i);
    }
    return result;
}

static void DrawMovieCard(Font font, int idx, float cx, float cy,
    int cardW, int cardH, bool hovered, bool selected,
    Vector2 mouse, bool clicked, float dt)
{
    const Movie& m = allMovies[idx];
    bool soldOut = IsSoldOut(m);

    float targetLift = hovered ? -7.0f : 0.0f;
    cardHoverY[idx] += (targetLift - cardHoverY[idx]) * dt * 14.0f;
    float drawY = cy + cardHoverY[idx];

    DrawRectangleRounded({ cx + 4, drawY + 8, (float)cardW, (float)cardH }, 0.08f, 8,
        { 0, 0, 0, (unsigned char)(hovered ? 90 : 50) });

    Color border = selected ? m.accent : (hovered ? BORDER_FOCUS : BORDER_NORMAL);
    DrawRectangleRounded({ cx, drawY, (float)cardW, (float)cardH }, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines({ cx, drawY, (float)cardW, (float)cardH }, 0.08f, 8, border);
    DrawRectangleRounded({ cx, drawY, (float)cardW, 5 }, 0.5f, 4, m.accent);

    // ── Poster image (replaces the old placeholder block) ──
    Color posterBg = {
        (unsigned char)(m.accent.r / 5),
        (unsigned char)(m.accent.g / 5),
        (unsigned char)(m.accent.b / 5), 255 };
    Rectangle posterRect = { cx + 10, drawY + 14, (float)(cardW - 20), 108.0f };
    Texture2D poster = GetPoster(m.posterPath);
    DrawPosterFitted(poster, posterRect, posterBg, m.accent);

    // Genre badge
    Vector2 gs = MeasureTextEx(font, m.genre, 9, 1);
    DrawRectangleRounded({ cx + 10, drawY + 128, gs.x + 10, 16 }, 0.4f, 4,
        { m.accent.r, m.accent.g, m.accent.b, 40 });
    DrawTextEx(font, m.genre, { cx + 15, drawY + 132 }, 9, 1, m.accent);

    std::string titleStr = m.title;
    if (titleStr.size() > 16) titleStr = titleStr.substr(0, 15) + "..";
    DrawTextEx(font, titleStr.c_str(), { cx + 10, drawY + 150 }, 10, 1, TEXT_PRIMARY);

    char ratingBuf[16]; snprintf(ratingBuf, 16, "* %.1f", m.rating);
    DrawTextEx(font, ratingBuf, { cx + 10, drawY + 166 }, 10, 1, { 255, 200, 50, 255 });

    char durBuf[16]; snprintf(durBuf, 16, "%d MIN", m.durationMin);
    DrawTextEx(font, durBuf, { cx + 10, drawY + 180 }, 9, 1, TEXT_SECONDARY);

    Rectangle bookBtn = { cx + 10, drawY + (float)cardH - 34, (float)cardW - 20, 26 };
    bool hoverBook = CheckCollisionPointRec(mouse, bookBtn) && !soldOut;
    Color bookBg = soldOut ? Color{ 40,40,55,255 } : (hoverBook ? ACCENT_HOVER : Color{ m.accent.r,m.accent.g,m.accent.b,200 });
    DrawRectangleRounded(bookBtn, 0.35f, 6, bookBg);
    const char* bookLabel = soldOut ? "SOLD OUT" : "BOOK NOW";
    Vector2 bts = MeasureTextEx(font, bookLabel, 10, 1);
    DrawTextEx(font, bookLabel,
        { bookBtn.x + bookBtn.width / 2 - bts.x / 2, bookBtn.y + bookBtn.height / 2 - bts.y / 2 },
        10, 1, soldOut ? TEXT_MUTED : WHITE);

    if (soldOut)
    {
        DrawRectangleRounded({ cx, drawY, (float)cardW, (float)cardH }, 0.08f, 8, { 0, 0, 0, 100 });
        Vector2 soSz = MeasureTextEx(font, "SOLD OUT", 13, 1);
        DrawRectangleRounded(
            { cx + cardW / 2 - soSz.x / 2 - 10, drawY + cardH / 2 - 14, soSz.x + 20, 28 },
            0.3f, 6, { 180, 50, 50, 220 });
        DrawTextEx(font, "SOLD OUT",
            { cx + cardW / 2 - soSz.x / 2, drawY + cardH / 2 - 7 }, 13, 1, WHITE);
    }
}

static void DrawSeatMap(Font font, int originX, int originY, int availableW,
    Color accent, Vector2 mouse, bool clicked)
{
    if (!seatMapInit) InitSeatMap();

    const int SW = 22, SH = 18, SGAP = 4, RGAP = 6, AISLE = 20;
    int sideW = SEAT_COLS * (SW + SGAP) - SGAP;
    int totalW = sideW * 2 + AISLE;
    int mapX = originX + (availableW - totalW) / 2;
    int mapY = originY;

    DrawRectangleRounded({ (float)mapX,(float)mapY,(float)totalW,14 }, 0.4f, 6,
        { accent.r,accent.g,accent.b,60 });
    Vector2 scSz = MeasureTextEx(font, "S C R E E N", 9, 1);
    DrawTextEx(font, "S C R E E N",
        { (float)(mapX + totalW / 2 - scSz.x / 2),(float)(mapY + 2) }, 9, 1,
        { accent.r,accent.g,accent.b,200 });
    mapY += 22;

    for (int r = 0; r < SEAT_ROWS; r++)
    {
        int rowY = mapY + r * (SH + RGAP);
        char rowLabel[4]; snprintf(rowLabel, 4, "%d", r + 1);
        DrawTextEx(font, rowLabel, { (float)(mapX - 16),(float)(rowY + 3) }, 9, 1, TEXT_MUTED);

        for (int side = 0; side < 2; side++)
        {
            int blockX = (side == 0) ? mapX : (mapX + sideW + AISLE);
            for (int c = 0; c < SEAT_COLS; c++)
            {
                int priceCol = (side == 0) ? (SEAT_COLS - 1 - c) : c;
                int sx = blockX + c * (SW + SGAP);
                int sy = rowY;
                Rectangle seatRect = { (float)sx,(float)sy,(float)SW,(float)SH };
                SeatState& seat = seatMap[r][side][c];
                bool hov = CheckCollisionPointRec(mouse, seatRect) && !seat.occupied;

                if (clicked && hov)
                {
                    if (seat.selected) seat.selected = false;
                    else if (CountSelected() < MAX_SELECTED) seat.selected = true;
                }

                Color seatCol = seat.occupied
                    ? Color{ 55,55,70,255 }
                : SeatZoneColor(priceCol, seat.selected, hov, accent);

                DrawRectangleRounded(seatRect, 0.25f, 4, seatCol);
                if (seat.selected) DrawRectangleRoundedLines(seatRect, 0.25f, 4, WHITE);
                else if (hov)      DrawRectangleRoundedLines(seatRect, 0.25f, 4, { 255,255,255,120 });
            }
        }
        char rowLabel2[4]; snprintf(rowLabel2, 4, "%d", r + 1);
        DrawTextEx(font, rowLabel2, { (float)(mapX + totalW + 4),(float)(rowY + 3) }, 9, 1, TEXT_MUTED);
    }

    int legY = mapY + SEAT_ROWS * (SH + RGAP) + 10;
    struct LegItem { const char* label; Color col; int price; };
    LegItem legend[] = {
        {"SILVER",  {110,115,140,220}, 8 },
        {"GOLD",    {180,140,40,220},  14},
        {"PLATINUM",{120,50,180,220},  22},
        {"TAKEN",   {55,55,70,255},    -1},
        {"SELECTED",accent,            -1},
    };
    int legX = mapX;
    for (auto& li : legend)
    {
        DrawRectangleRounded({ (float)legX,(float)legY,14,12 }, 0.3f, 4, li.col);
        char lbuf[32];
        if (li.price >= 0) snprintf(lbuf, 32, "%s $%d", li.label, li.price);
        else               snprintf(lbuf, 32, "%s", li.label);
        DrawTextEx(font, lbuf, { (float)(legX + 18),(float)(legY + 1) }, 9, 1, TEXT_SECONDARY);
        legX += (int)MeasureTextEx(font, lbuf, 9, 1).x + 28;
    }
}

static void DrawDetailView(Font font, int idx, int screenW, int screenH,
    float slideOffset, float time, float pulse,
    Vector2 mouse, bool clicked, SessionUser& sessionUser, float dt)
{
    const Movie& m = allMovies[idx];
    float ox = slideOffset;

    int panelX = (int)(40 + ox);
    int panelY = 84;
    int panelW = screenW - 80;
    int panelH = screenH - 130;

    DrawRectangle(panelX + 5, panelY + 8, panelW, panelH, { 0,0,0,60 });
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW,(float)panelH }, 0.04f, 10, BG_CARD);
    DrawRectangleRoundedLines({ (float)panelX,(float)panelY,(float)panelW,(float)panelH }, 0.04f, 10, BORDER_NORMAL);
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW,6 }, 0.04f, 4, m.accent);

    int leftX = panelX + 32;
    int topY = panelY + 28;

    // ── Poster image (replaces the old placeholder block) ──
    Color posterBg = {
        (unsigned char)(m.accent.r / 5),
        (unsigned char)(m.accent.g / 5),
        (unsigned char)(m.accent.b / 5), 255 };
    Rectangle posterRect = { (float)leftX, (float)topY, 200.0f, 280.0f };
    Texture2D poster = GetPoster(m.posterPath);
    DrawPosterFitted(poster, posterRect, posterBg, m.accent);

    // Genre / language badges
    Vector2 gs = MeasureTextEx(font, m.genre, 11, 1);
    DrawRectangleRounded({ (float)leftX,(float)(topY + 290),gs.x + 14,20 }, 0.4f, 4,
        { m.accent.r,m.accent.g,m.accent.b,50 });
    DrawTextEx(font, m.genre, { (float)(leftX + 7),(float)(topY + 294) }, 11, 1, m.accent);

    Vector2 ls = MeasureTextEx(font, m.language, 11, 1);
    DrawRectangleRounded({ (float)(leftX + (int)gs.x + 20),(float)(topY + 290),ls.x + 14,20 }, 0.4f, 4,
        { 60,60,80,180 });
    DrawTextEx(font, m.language, { (float)(leftX + (int)gs.x + 27),(float)(topY + 294) }, 11, 1, TEXT_SECONDARY);

    char ratingBuf[32]; snprintf(ratingBuf, 32, "* %.1f / 10", m.rating);
    DrawTextEx(font, ratingBuf, { (float)leftX,(float)(topY + 322) }, 13, 1, { 255,200,50,255 });

    char durBuf[32]; snprintf(durBuf, 32, "%d MIN", m.durationMin);
    DrawTextEx(font, durBuf, { (float)leftX,(float)(topY + 344) }, 12, 1, TEXT_SECONDARY);
    DrawTextEx(font, m.releaseDate, { (float)leftX,(float)(topY + 364) }, 11, 1, TEXT_SECONDARY);

    // ── Right panel ──
    int rightX = leftX + 240;
    int rightW = panelW - 280;

    DrawTextEx(font, m.title, { (float)rightX,(float)topY }, 24, 1.5f, TEXT_PRIMARY);
    DrawRectangle(rightX, topY + 36, rightW - 40, 1, BORDER_NORMAL);
    DrawTextEx(font, "SYNOPSIS", { (float)rightX,(float)(topY + 48) }, 11, 1, TEXT_SECONDARY);

    const char* synopsis = "An epic tale of adventure and wonder brought to life on the big screen. Not to be missed.";
    std::string syn = synopsis;
    int lineY = topY + 66, lineChars = 55;
    for (int s = 0; s < (int)syn.size(); s += lineChars)
    {
        DrawTextEx(font, syn.substr(s, lineChars).c_str(), { (float)rightX,(float)lineY }, 12, 0.5f, TEXT_PRIMARY);
        lineY += 18;
    }

    int showY = topY + 140;

    if (jumpToShowtime)
    {
        showtimePulse += dt * 6.0f;
        if (showtimePulse > 3.14159f * 4) { showtimePulse = 0; jumpToShowtime = false; }
    }
    float showtimeGlow = jumpToShowtime ? (sinf(showtimePulse) + 1.0f) / 2.0f : 0.0f;
    Color showtimeLabel = jumpToShowtime
        ? LerpColor(TEXT_SECONDARY, { 255,200,80,255 }, showtimeGlow) : TEXT_SECONDARY;

    DrawTextEx(font, "AVAILABLE SHOWTIMES", { (float)rightX,(float)showY }, 11, 1, showtimeLabel);
    DrawRectangle(rightX, showY + 16, rightW - 40, 1, BORDER_NORMAL);
    showY += 26;

    static int selectedShow = -1;

    for (int i = 0; i < (int)m.shows.size(); i++)
    {
        const Showtime& sh = m.shows[i];
        int sx = rightX + i * (90 + 10);
        bool avail = sh.available, sel = (selectedShow == i);
        bool hov = CheckCollisionPointRec(mouse, { (float)sx,(float)showY,86,38 });

        Color showBg = sel ? m.accent : (hov && avail) ? LerpColor(BG_INPUT, m.accent, 0.3f) : avail ? BG_INPUT : Color{ 25,25,35,255 };
        Color showBdr = sel ? m.accent : avail ? (hov ? BORDER_FOCUS : BORDER_NORMAL) : Color{ 50,50,60,255 };
        Color showTxt = avail ? (sel ? WHITE : TEXT_PRIMARY) : TEXT_MUTED;

        if (jumpToShowtime && avail)
        {
            unsigned char glowA = (unsigned char)(showtimeGlow * 60.0f);
            DrawRectangleRounded({ (float)sx - 3,(float)showY - 3,92,44 }, 0.2f, 6, { 255,200,80,glowA });
        }

        DrawRectangleRounded({ (float)sx,(float)showY,86,38 }, 0.2f, 6, showBg);
        DrawRectangleRoundedLines({ (float)sx,(float)showY,86,38 }, 0.2f, 6, showBdr);
        Vector2 ts = MeasureTextEx(font, sh.time, 13, 1);
        DrawTextEx(font, sh.time, { (float)sx + 43 - ts.x / 2,(float)showY + 11 }, 13, 1, showTxt);

        if (!avail)
        {
            Vector2 fs = MeasureTextEx(font, "FULL", 9, 1);
            DrawTextEx(font, "FULL", { (float)sx + 43 - fs.x / 2,(float)showY + 26 }, 9, 1, TEXT_MUTED);
        }

        if (clicked && hov && avail) { selectedShow = i; ClearSeatSelections(); }
    }

    int seatMapSectionY = showY + 60;
    DrawTextEx(font, "CHOOSE YOUR SEAT", { (float)rightX,(float)seatMapSectionY }, 11, 1, TEXT_SECONDARY);
    int selCount = CountSelected();
    char selCountBuf[48]; snprintf(selCountBuf, 48, "%d / %d SEATS", selCount, MAX_SELECTED);
    Vector2 scb = MeasureTextEx(font, selCountBuf, 10, 1);
    Color selCountColor = (selCount == MAX_SELECTED) ? Color{ 255,160,60,255 } : TEXT_SECONDARY;
    DrawTextEx(font, selCountBuf, { (float)(rightX + rightW - 40 - scb.x),(float)seatMapSectionY }, 10, 1, selCountColor);
    DrawRectangle(rightX, seatMapSectionY + 16, rightW - 40, 1, BORDER_NORMAL);
    seatMapSectionY += 24;

    DrawSeatMap(font, rightX, seatMapSectionY, rightW - 40, m.accent, mouse, clicked);

    int priceSummaryY = seatMapSectionY + SEAT_ROWS * (18 + 6) + 22 + 32;
    int totalPrice = TotalPrice();
    bool hasSeatSelected = (selCount > 0);

    if (hasSeatSelected)
    {
        char priceBuf[64];
        snprintf(priceBuf, 64, "TOTAL:  $%d  (%d seat%s)", totalPrice, selCount, selCount > 1 ? "s" : "");
        Vector2 pbSz = MeasureTextEx(font, priceBuf, 13, 1);
        float priceBoxX = (float)rightX, priceBoxW = (float)(rightW - 40);
        DrawRectangleRounded({ priceBoxX,(float)priceSummaryY,priceBoxW,36 }, 0.2f, 6,
            { m.accent.r,m.accent.g,m.accent.b,30 });
        DrawRectangleRoundedLines({ priceBoxX,(float)priceSummaryY,priceBoxW,36 }, 0.2f, 6,
            { m.accent.r,m.accent.g,m.accent.b,100 });
        DrawTextEx(font, priceBuf,
            { priceBoxX + priceBoxW / 2 - pbSz.x / 2,(float)priceSummaryY + 10 }, 13, 1, { 255,220,100,255 });
        priceSummaryY += 44;
    }

    int btnY = priceSummaryY + 4;
    Rectangle confirmBtn = { (float)rightX,(float)btnY,(float)(rightW - 40),48 };
    bool hoverConfirm = CheckCollisionPointRec(mouse, confirmBtn);
    bool canConfirm = (selectedShow >= 0 && hasSeatSelected);

    if (confirmPulseActive)
    {
        confirmPulseTimer += dt * 5.0f;
        if (confirmPulseTimer > 3.14159f * 3) { confirmPulseActive = false; confirmPulseTimer = 0; }
        float p = (sinf(confirmPulseTimer) + 1.0f) / 2.0f;
        DrawRectangleRounded(
            { confirmBtn.x - 4,confirmBtn.y - 4,confirmBtn.width + 8,confirmBtn.height + 8 },
            0.25f, 8, { 200,80,80,(unsigned char)(p * 80.0f) });
    }

    Color confirmBg = canConfirm ? (hoverConfirm ? ACCENT_HOVER : ACCENT) : Color{ 40,40,55,255 };
    Color confirmTxt = canConfirm ? WHITE : TEXT_MUTED;
    DrawRectangleRounded(confirmBtn, 0.25f, 8, confirmBg);

    if (rippleActive)
    {
        float rp = rippleTimer / RIPPLE_DURATION;
        DrawCircle((int)rippleX, (int)rippleY, rp * RIPPLE_MAX_R,
            { 255,255,255,(unsigned char)((1.0f - rp) * 50.0f) });
    }

    const char* confirmLabel = canConfirm ? "CONFIRM BOOKING" : "SELECT SHOWTIME AND SEATS";
    Vector2 cts = MeasureTextEx(font, confirmLabel, 13, 1);
    DrawTextEx(font, confirmLabel,
        { confirmBtn.x + confirmBtn.width / 2 - cts.x / 2, confirmBtn.y + confirmBtn.height / 2 - cts.y / 2 },
        13, 1, confirmTxt);

    if (clicked && hoverConfirm)
    {
        if (canConfirm)
        {
            rippleActive = true; rippleTimer = 0; rippleX = mouse.x; rippleY = mouse.y;
            std::string note = "BOOKING CONFIRMED: ";
            note += m.title; note += "  "; note += m.shows[selectedShow].time;
            note += "  " + std::to_string(selCount) + " SEAT(S)  $" + std::to_string(totalPrice);
            ShowToast(note);
            ClearSeatSelections(); selectedShow = -1;
        }
        else { confirmPulseActive = true; confirmPulseTimer = 0.0f; jumpToShowtime = true; showtimePulse = 0.0f; }
    }

    Rectangle backBtn = { (float)(panelX + 12),(float)(panelY + 12),110,34 };
    bool hoverBack = CheckCollisionPointRec(mouse, backBtn);
    DrawRectangleRounded(backBtn, 0.3f, 6, hoverBack ? Color{ 72,130,255,200 } : Color{ 30,35,60,220 });
    DrawRectangleRoundedLines(backBtn, 0.3f, 6, hoverBack ? ACCENT : BORDER_NORMAL);
    Vector2 bkSz = MeasureTextEx(font, "< BACK", 13, 1);
    DrawTextEx(font, "< BACK",
        { backBtn.x + backBtn.width / 2 - bkSz.x / 2, backBtn.y + backBtn.height / 2 - bkSz.y / 2 },
        13, 1, hoverBack ? WHITE : TEXT_PRIMARY);

    if (clicked && hoverBack)
    {
        selectedMovie = -1; selectedShow = -1;
        jumpToShowtime = false; confirmPulseActive = false;
        ClearSeatSelections();
    }
}

AppState mainScreen(Font font, SessionUser& sessionUser)
{
    float dt = GetFrameTime();
    int   screenW = GetScreenWidth();
    int   screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    if (!particlesInit) InitParticles(screenW, screenH);
    if (entranceTimer < ENTER_DURATION) entranceTimer += dt;
    float enterT = EaseOutCubic(entranceTimer / ENTER_DURATION);
    float panelAlpha = enterT;

    UpdateParticles(dt, screenW, screenH);
    if (toastTimer > 0) toastTimer -= dt;
    if (rippleActive) { rippleTimer += dt; if (rippleTimer >= RIPPLE_DURATION) rippleActive = false; }

    float targetSlide = (selectedMovie >= 0) ? 0.0f : (float)screenW;
    detailSlide += (targetSlide - detailSlide) * dt * 16.0f;
    cardScrollX += (cardScrollTarget - cardScrollX) * dt * 12.0f;

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int navH = 64;
    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        Rectangle navRect = { navX, 0, 130, (float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
        {
            activeNav = i; entranceTimer = 0.0f;
            AppState targets[] = { MAIN, CINEMAS, TICKETS, PROFILE };
            prof_pendingNav = targets[i];
        }
    }

    int filterBarY = navH + 12;
    int filterBarH = 36;
    int cardW = 180, cardH = 230, cardSpacing = 16, cardsStartX = 32;
    int listY = filterBarY + filterBarH + 14;

    std::vector<int> filtered = FilteredIndices();
    int   totalCardsW = (int)filtered.size() * (cardW + cardSpacing);
    float maxScroll = (float)std::max(0, totalCardsW - (screenW - cardsStartX * 2));

    float wheel = GetMouseWheelMove();
    if (selectedMovie < 0 && wheel != 0)
    {
        cardScrollTarget -= wheel * 60.0f;
        if (cardScrollTarget < 0)         cardScrollTarget = 0;
        if (cardScrollTarget > maxScroll) cardScrollTarget = maxScroll;
    }

    hoveredCard = -1;
    if (selectedMovie < 0)
    {
        for (int fi = 0; fi < (int)filtered.size(); fi++)
        {
            float cx = (float)(cardsStartX + fi * (cardW + cardSpacing)) - cardScrollX;
            float cy = (float)listY;
            Rectangle cr = { cx, cy, (float)cardW, (float)cardH };
            if (cx + cardW < 0 || cx > screenW) continue;
            if (CheckCollisionPointRec(mouse, cr))
            {
                hoveredCard = fi;
                if (clicked && !IsSoldOut(allMovies[filtered[fi]]))
                {
                    selectedMovie = filtered[fi]; entranceTimer = 0;
                    jumpToShowtime = false; InitSeatMap();
                }
            }
        }
    }

    BeginDrawing();
    ClearBackground(BG_DARK);

    unsigned char PA = (unsigned char)(panelAlpha * 255.0f);

    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        unsigned char pa = (unsigned char)(particles[i].alpha * panelAlpha * 255.0f);
        DrawCircle((int)particles[i].x, (int)particles[i].y, particles[i].r, { 100,140,255,pa });
    }

    for (int r = 280; r >= 0; r -= 14) { float t = 1.0f - (float)r / 280.0f; unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f) * panelAlpha); DrawCircle((int)(screenW * 0.12f), (int)(screenH * 0.18f), (float)r, { 40,90,255,a }); }
    for (int r = 260; r >= 0; r -= 14) { float t = 1.0f - (float)r / 260.0f; unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f) * panelAlpha); DrawCircle((int)(screenW * 0.88f), (int)(screenH * 0.82f), (float)r, { 50,80,220,a }); }
    for (int r = 200; r >= 0; r -= 14) { float t = 1.0f - (float)r / 200.0f; unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f) * panelAlpha); DrawCircle((int)(screenW * 0.85f), (int)(screenH * 0.15f), (float)r, { 80,50,200,a }); }
    for (int r = 180; r >= 0; r -= 14) { float t = 1.0f - (float)r / 180.0f; unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f) * panelAlpha);  DrawCircle((int)(screenW * 0.14f), (int)(screenH * 0.80f), (float)r, { 30,70,200,a }); }
    for (int r = 340; r >= 0; r -= 14) { float t = 1.0f - (float)r / 340.0f; unsigned char a = (unsigned char)(t * t * 22.0f * panelAlpha);               DrawCircle(screenW / 2, screenH / 2, (float)r, { 55,95,210,a }); }

    for (int sy = 0; sy < screenH; sy += 4) DrawRectangle(0, sy, screenW, 1, { 0,0,0,12 });

    Rectangle searchBox = { (float)(screenW - 560),(float)(navH / 2 - 16),220,32 };
    if (clicked) searchActive = CheckCollisionPointRec(mouse, searchBox);
    if (searchActive)
    {
        if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery.empty()) searchQuery.pop_back();
        int k = GetCharPressed();
        while (k > 0) { if (searchQuery.size() < 32) searchQuery += (char)k; k = GetCharPressed(); }
    }

    DrawRectangle(0, 0, screenW, navH, { 10,12,28,220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "Gekoya", { 32,(float)(navH / 2) - 11 }, 22, 1.5f, { TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA });

    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f, navY = (float)(navH / 2) - 7;
        bool isA = (activeNav == i);
        DrawTextEx(font, navItems[i], { navX,navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA }
        : Color{ TEXT_SECONDARY.r,TEXT_SECONDARY.g,TEXT_SECONDARY.b,PA });
        if (isA) DrawRectangle((int)navX, navH - 2, (int)MeasureTextEx(font, navItems[i], 12, 1).x, 2, ACCENT);
    }

    DrawRectangleRounded(searchBox, 0.3f, 6, BG_INPUT);
    DrawRectangleRoundedLines(searchBox, 0.3f, 6, searchActive ? BORDER_FOCUS : BORDER_NORMAL);
    if (searchQuery.empty() && !searchActive)
        DrawTextEx(font, "SEARCH MOVIES...", { searchBox.x + 10,searchBox.y + 9 }, 11, 1, TEXT_MUTED);
    else
        DrawTextEx(font, searchQuery.c_str(), { searchBox.x + 10,searchBox.y + 9 }, 11, 1, TEXT_PRIMARY);
    if (searchActive && ((int)(time * 2)) % 2 == 0)
    {
        float cx2 = searchBox.x + 10 + MeasureTextEx(font, searchQuery.c_str(), 11, 1).x + 2;
        DrawRectangle((int)cx2, (int)searchBox.y + 6, 2, 20, BORDER_FOCUS);
    }

    std::string greeting = "HI, " + sessionUser.username;
    Vector2 greetSz = MeasureTextEx(font, greeting.c_str(), 13, 1);
    DrawRectangleRounded({ (float)(screenW - 220),(float)(navH / 2 - 14),greetSz.x + 20,28 }, 0.3f, 6, { 30,40,70,180 });
    DrawTextEx(font, greeting.c_str(), { (float)(screenW - 210),(float)(navH / 2 - 7) }, 13, 1,
        { TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA });

    Rectangle logoutBtn = { (float)(screenW - 105),(float)(navH / 2 - 14),88,28 };
    bool hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);
    DrawRectangleRounded(logoutBtn, 0.3f, 6, hoverLogout ? Color{ 180,50,50,220 } : Color{ 80,30,30,180 });
    DrawRectangleRoundedLines(logoutBtn, 0.3f, 6, hoverLogout ? Color{ 220,80,80,255 } : Color{ 140,50,50,200 });
    Vector2 loSz = MeasureTextEx(font, "LOG OUT", 11, 1);
    DrawTextEx(font, "LOG OUT",
        { logoutBtn.x + logoutBtn.width / 2 - loSz.x / 2, logoutBtn.y + logoutBtn.height / 2 - loSz.y / 2 },
        11, 1, hoverLogout ? WHITE : Color{ 200,100,100,255 });

    if (clicked && hoverLogout)
    {
        selectedMovie = -1; searchActive = false; filterGenre = 0; entranceTimer = 0.0f;
        particlesInit = false; cardScrollX = 0; cardScrollTarget = 0;
        jumpToShowtime = false; confirmPulseActive = false; seatMapInit = false;
        ClearSeatSelections();
        UnloadMovieTextures();   // ← free GPU memory on logout
        sessionUser.username = ""; sessionUser.email = "";
        EndDrawing();
        return AUTH;
    }

    for (int i = 0; i < GENRE_COUNT; i++)
    {
        float fx = 32.0f + i * 110.0f;
        float fw = MeasureTextEx(font, genreFilters[i], 11, 1).x + 20;
        Rectangle fr = { fx,(float)filterBarY,fw,(float)filterBarH - 6 };
        bool sel = (filterGenre == i), hov = CheckCollisionPointRec(mouse, fr);
        DrawRectangleRounded(fr, 0.3f, 6, sel ? ACCENT : (hov ? Color{ 40,50,80,200 } : Color{ 20,22,40,180 }));
        DrawRectangleRoundedLines(fr, 0.3f, 6, sel ? ACCENT : (hov ? BORDER_FOCUS : BORDER_NORMAL));
        DrawTextEx(font, genreFilters[i], { fx + 10,(float)filterBarY + 8 }, 11, 1,
            sel ? WHITE : (hov ? TEXT_PRIMARY : TEXT_SECONDARY));
        if (clicked && hov) { filterGenre = i; cardScrollTarget = 0; }
    }

    char countBuf[32]; snprintf(countBuf, 32, "%d FILMS", (int)filtered.size());
    Vector2 cbSz = MeasureTextEx(font, countBuf, 11, 1);
    DrawTextEx(font, countBuf, { (float)(screenW - cbSz.x - 32),(float)(filterBarY + 8) }, 11, 1, TEXT_SECONDARY);

    if (selectedMovie < 0 || detailSlide > screenW * 0.05f)
    {
        BeginScissorMode(0, listY - 4, screenW, screenH - listY - 34);
        if (filtered.empty())
        {
            Vector2 noSz = MeasureTextEx(font, "NO MOVIES FOUND", 18, 1);
            DrawTextEx(font, "NO MOVIES FOUND",
                { (float)(screenW / 2) - noSz.x / 2,(float)(screenH / 2) - 40 }, 18, 1, TEXT_MUTED);
        }
        else
        {
            for (int fi = 0; fi < (int)filtered.size(); fi++)
            {
                int idx = filtered[fi];
                float cx = (float)(cardsStartX + fi * (cardW + cardSpacing)) - cardScrollX;
                float cy = (float)listY;
                if (cx + cardW<0 || cx>screenW) continue;
                DrawMovieCard(font, idx, cx, cy, cardW, cardH,
                    (hoveredCard == fi), (selectedMovie == idx), mouse, clicked, dt);
            }
        }
        EndScissorMode();

        if (maxScroll > 0)
        {
            float trackW = (float)(screenW - cardsStartX * 2);
            float thumbW = trackW * ((float)(screenW - cardsStartX * 2) / (float)totalCardsW);
            float thumbX = (float)cardsStartX + (cardScrollX / maxScroll) * (trackW - thumbW);
            int trackY = screenH - 46;
            DrawRectangle(cardsStartX, trackY, (int)trackW, 3, { 40,45,70,180 });
            DrawRectangleRounded({ thumbX,(float)trackY - 1,thumbW,5 }, 0.5f, 4, ACCENT);
        }
    }

    if (selectedMovie >= 0 || detailSlide < screenW * 0.95f)
    {
        DrawDetailView(font, selectedMovie >= 0 ? selectedMovie : 0,
            screenW, screenH, detailSlide, time, pulse, mouse, clicked, sessionUser, dt);
    }

    int barY = screenH - 34;
    DrawRectangle(0, barY, screenW, 34, { 10,12,28,210 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);

    int availableShows = 0;
    for (auto& mv : allMovies) for (auto& sh : mv.shows) if (sh.available) availableShows++;

    char cinBuf[32];  snprintf(cinBuf, 32, "%d CINEMAS NEARBY", 3);
    char showBuf[32]; snprintf(showBuf, 32, "%d SHOWS TODAY", availableShows);
    char filBuf[32];  snprintf(filBuf, 32, "%d FILMS SHOWING", (int)allMovies.size());
    DrawTextEx(font, cinBuf, { 32,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, showBuf, { 220,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, filBuf, { 420,(float)(barY + 10) }, 11, 1, TEXT_SECONDARY);

    float dotP = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotP * 75);
    DrawCircle(screenW - 120, barY + 17, 5, { 80,220,120,dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110),(float)(barY + 10) }, 11, 1, { 80,220,120,255 });

    if (toastTimer > 0.0f)
    {
        float fadeIn = std::min(1.0f, (TOAST_DURATION - toastTimer) / 0.2f);
        float fadeOut = std::min(1.0f, toastTimer / 0.3f);
        float alpha = std::min(fadeIn, fadeOut);
        unsigned char ta = (unsigned char)(alpha * 240.0f);
        Vector2 tSz = MeasureTextEx(font, toastMsg.c_str(), 12, 1);
        float tw = tSz.x + 32, th = 42;
        float tx = (float)(screenW / 2) - tw / 2, ty = (float)(barY - th - 12);
        DrawRectangleRounded({ tx,ty,tw,th }, 0.3f, 8, { 20,120,60,ta });
        DrawRectangleRoundedLines({ tx,ty,tw,th }, 0.3f, 8, { 60,200,100,ta });
        DrawTextEx(font, toastMsg.c_str(), { tx + 16,ty + 13 }, 12, 1, { 200,255,220,ta });
    }

    EndDrawing();
    AppState ret = prof_pendingNav;
    prof_pendingNav = MAIN;
    return ret;
}