#include "mainScreen.h"
#include "../colors.h"
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

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
};

static std::vector<Movie> allMovies = {
    { "DUNE: PART TWO",  "SCI-FI",   "English", "2024-03-01", 8.5f, 166,
      {80,130,255,255}, {{"10:30",true},{"13:45",true},{"17:00",false},{"20:15",true}} },
    { "OPPENHEIMER",     "DRAMA",    "English", "2023-07-21", 8.9f, 180,
      {255,140,60,255}, {{"11:00",true},{"14:30",false},{"18:00",true},{"21:30",true}} },
    { "THE BATMAN",      "ACTION",   "English", "2022-03-04", 7.8f, 176,
      {60,180,255,255}, {{"10:00",true},{"13:00",true},{"16:30",true},{"20:00",false}} },
    { "POOR THINGS",     "FANTASY",  "English", "2023-12-08", 7.9f, 141,
      {180,80,255,255}, {{"12:00",false},{"15:15",true},{"18:30",true},{"21:45",true}} },
    { "PAST LIVES",      "ROMANCE",  "Korean",  "2023-06-02", 7.8f, 106,
      {80,220,160,255}, {{"11:30",true},{"14:00",true},{"16:45",false},{"19:30",true}} },
    { "KILLERS OF THE FLOWER MOON","DRAMA","English","2023-10-20",7.7f,206,
      {220,100,80,255}, {{"10:15",true},{"14:00",false},{"18:15",true},{"22:00",true}} },
    { "POOR THINGS 2",   "FANTASY",  "English", "2024-06-01", 7.5f, 130,
      {200,60,180,255}, {{"13:00",false},{"16:00",false},{"19:00",false},{"22:00",false}} },
    { "INTERSTELLAR",    "SCI-FI",   "English", "2014-11-07", 8.7f, 169,
      {100,200,255,255},{{"11:00",true},{"14:30",true},{"18:00",true},{"21:30",false}} },
};

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

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t) };
}

static void ShowToast(const std::string& msg)
{
    toastMsg = msg;
    toastTimer = TOAST_DURATION;
}

static bool IsSoldOut(const Movie& m)
{
    for (auto& s : m.shows)
        if (s.available) return false;
    return true;
}

static std::vector<int> FilteredIndices()
{
    std::vector<int> result;
    for (int i = 0; i < (int)allMovies.size(); i++)
    {
        const Movie& m = allMovies[i];
        if (filterGenre > 0 && std::string(m.genre) != genreFilters[filterGenre])
            continue;
        if (!searchQuery.empty())
        {
            std::string q = searchQuery;
            std::transform(q.begin(), q.end(), q.begin(), ::toupper);
            std::string t = std::string(m.title);    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
            std::string g = std::string(m.genre);    std::transform(g.begin(), g.end(), g.begin(), ::toupper);
            std::string l = std::string(m.language); std::transform(l.begin(), l.end(), l.begin(), ::toupper);
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
    bool         soldOut = IsSoldOut(m);

    float targetLift = hovered ? -7.0f : 0.0f;
    cardHoverY[idx] += (targetLift - cardHoverY[idx]) * dt * 14.0f;
    float drawY = cy + cardHoverY[idx];

    DrawRectangleRounded({ cx + 4, drawY + 8, (float)cardW, (float)cardH }, 0.08f, 8,
        { 0, 0, 0, (unsigned char)(hovered ? 90 : 50) });

    Color border = selected ? m.accent : (hovered ? BORDER_FOCUS : BORDER_NORMAL);
    DrawRectangleRounded({ cx, drawY, (float)cardW, (float)cardH }, 0.08f, 8, BG_CARD);
    DrawRectangleRoundedLines({ cx, drawY, (float)cardW, (float)cardH }, 0.08f, 8, border);
    DrawRectangleRounded({ cx, drawY, (float)cardW, 5 }, 0.5f, 4, m.accent);

    Color posterBg = {
        (unsigned char)(m.accent.r / 5),
        (unsigned char)(m.accent.g / 5),
        (unsigned char)(m.accent.b / 5), 255 };
    DrawRectangle((int)cx + 10, (int)drawY + 14, cardW - 20, 108, posterBg);
    DrawRectangleLines((int)cx + 10, (int)drawY + 14, cardW - 20, 108, m.accent);

    float ix = cx + cardW / 2 - 12, iy = drawY + 14 + 30;
    DrawRectangle((int)ix, (int)iy, 24, 24, m.accent);
    DrawTriangle({ ix + 7, iy + 5 }, { ix + 7, iy + 19 }, { ix + 21, iy + 12 }, posterBg);

    std::string posterTitle = m.title;
    if (posterTitle.size() > 12) posterTitle = posterTitle.substr(0, 11) + "..";
    Vector2 ptSz = MeasureTextEx(font, posterTitle.c_str(), 9, 1);
    DrawTextEx(font, posterTitle.c_str(),
        { cx + 10 + (cardW - 20 - ptSz.x) / 2, drawY + 14 + 82 }, 9, 1,
        { m.accent.r, m.accent.g, m.accent.b, 200 });

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
    Color bookBg = soldOut ? Color{ 40, 40, 55, 255 } : (hoverBook ? ACCENT_HOVER : Color{ m.accent.r, m.accent.g, m.accent.b, 200 });
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
            { cx + cardW / 2 - soSz.x / 2, drawY + cardH / 2 - 7 },
            13, 1, WHITE);
    }
}

static void DrawDetailView(Font font, int idx, int screenW, int screenH,
    float slideOffset, float time, float pulse,
    Vector2 mouse, bool clicked, SessionUser& sessionUser, float dt)
{
    const Movie& m = allMovies[idx];
    float        ox = slideOffset;

    int panelX = (int)(40 + ox);
    int panelY = 84;
    int panelW = screenW - 80;
    int panelH = screenH - 130;

    DrawRectangle(panelX + 5, panelY + 8, panelW, panelH, { 0, 0, 0, 60 });
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW,(float)panelH }, 0.04f, 10, BG_CARD);
    DrawRectangleRoundedLines({ (float)panelX,(float)panelY,(float)panelW,(float)panelH }, 0.04f, 10, BORDER_NORMAL);
    DrawRectangleRounded({ (float)panelX,(float)panelY,(float)panelW,6 }, 0.04f, 4, m.accent);

    int leftX = panelX + 32;
    int topY = panelY + 28;

    Color posterBg = {
        (unsigned char)(m.accent.r / 5),
        (unsigned char)(m.accent.g / 5),
        (unsigned char)(m.accent.b / 5), 255 };
    DrawRectangle(leftX, topY, 200, 280, posterBg);
    DrawRectangleLines(leftX, topY, 200, 280, m.accent);

    DrawRectangle(leftX + 80, topY + 90, 40, 40, m.accent);
    DrawTriangle(
        { (float)leftX + 90, (float)topY + 98 },
        { (float)leftX + 90, (float)topY + 122 },
        { (float)leftX + 118, (float)topY + 110 }, posterBg);

    std::string posterTitle = m.title;
    Vector2 ptSz = MeasureTextEx(font, posterTitle.c_str(), 10, 1);
    while (ptSz.x > 180 && posterTitle.size() > 4)
    {
        posterTitle = posterTitle.substr(0, posterTitle.size() - 1);
        ptSz = MeasureTextEx(font, (posterTitle + "..").c_str(), 10, 1);
    }
    DrawTextEx(font, posterTitle.c_str(),
        { (float)(leftX + 100 - (int)ptSz.x / 2), (float)(topY + 145) }, 10, 1,
        { m.accent.r, m.accent.g, m.accent.b, 200 });

    Vector2 rdSz = MeasureTextEx(font, m.releaseDate, 9, 1);
    DrawTextEx(font, m.releaseDate,
        { (float)(leftX + 100 - (int)rdSz.x / 2), (float)(topY + 162) }, 9, 1,
        { 180, 180, 200, 160 });

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
        ? LerpColor(TEXT_SECONDARY, { 255, 200, 80, 255 }, showtimeGlow)
        : TEXT_SECONDARY;
    DrawTextEx(font, "AVAILABLE SHOWTIMES", { (float)rightX,(float)showY }, 11, 1, showtimeLabel);
    DrawRectangle(rightX, showY + 16, rightW - 40, 1, BORDER_NORMAL);
    showY += 26;

    static int selectedShow = -1;

    for (int i = 0; i < (int)m.shows.size(); i++)
    {
        const Showtime& sh = m.shows[i];
        int sx = rightX + i * (90 + 10);
        bool avail = sh.available;
        bool sel = (selectedShow == i);
        bool hov = CheckCollisionPointRec(mouse, { (float)sx,(float)showY,86,38 });

        Color showBg = sel ? m.accent : (hov && avail) ? LerpColor(BG_INPUT, m.accent, 0.3f) : avail ? BG_INPUT : Color{ 25,25,35,255 };
        Color showBdr = sel ? m.accent : avail ? (hov ? BORDER_FOCUS : BORDER_NORMAL) : Color{ 50,50,60,255 };
        Color showTxt = avail ? (sel ? WHITE : TEXT_PRIMARY) : TEXT_MUTED;

        if (jumpToShowtime && avail)
        {
            unsigned char glowA = (unsigned char)(showtimeGlow * 60.0f);
            DrawRectangleRounded({ (float)sx - 3,(float)showY - 3,92,44 }, 0.2f, 6,
                { 255, 200, 80, glowA });
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

        if (clicked && hov && avail) selectedShow = i;
    }

    int seatY = showY + 60;
    DrawTextEx(font, "SELECT SEAT TYPE", { (float)rightX,(float)seatY }, 11, 1, TEXT_SECONDARY);
    DrawRectangle(rightX, seatY + 16, rightW - 40, 1, BORDER_NORMAL);
    seatY += 26;

    struct SeatType { const char* name; int price; Color col; };
    static SeatType seatTypes[] = {
        {"SILVER",  8,  {160,160,180,255}},
        {"GOLD",    14, {220,180,50,255}},
        {"PLATINUM",22, {180,80,255,255}},
    };
    static int selectedSeat = -1;

    for (int i = 0; i < 3; i++)
    {
        int stx = rightX + i * (110 + 12);
        bool sel = (selectedSeat == i);
        bool hov = CheckCollisionPointRec(mouse, { (float)stx,(float)seatY,108,52 });

        Color stBg = sel ? Color{ seatTypes[i].col.r,seatTypes[i].col.g,seatTypes[i].col.b,40 }
        : hov ? Color{ seatTypes[i].col.r,seatTypes[i].col.g,seatTypes[i].col.b,20 } : BG_INPUT;
        Color stBdr = sel ? seatTypes[i].col : (hov ? BORDER_FOCUS : BORDER_NORMAL);

        DrawRectangleRounded({ (float)stx,(float)seatY,108,52 }, 0.15f, 6, stBg);
        DrawRectangleRoundedLines({ (float)stx,(float)seatY,108,52 }, 0.15f, 6, stBdr);

        Vector2 ns = MeasureTextEx(font, seatTypes[i].name, 11, 1);
        DrawTextEx(font, seatTypes[i].name,
            { (float)stx + 54 - ns.x / 2,(float)seatY + 8 }, 11, 1,
            sel ? seatTypes[i].col : TEXT_PRIMARY);

        char priceBuf[16]; snprintf(priceBuf, 16, "$%d", seatTypes[i].price);
        Vector2 ps = MeasureTextEx(font, priceBuf, 13, 1);
        DrawTextEx(font, priceBuf,
            { (float)stx + 54 - ps.x / 2,(float)seatY + 26 }, 13, 1,
            sel ? seatTypes[i].col : TEXT_SECONDARY);

        if (clicked && hov) selectedSeat = i;
    }

    int btnY = seatY + 72;
    Rectangle confirmBtn = { (float)rightX,(float)btnY,(float)(rightW - 40),48 };
    bool hoverConfirm = CheckCollisionPointRec(mouse, confirmBtn);
    bool canConfirm = (selectedShow >= 0 && selectedSeat >= 0);

    if (confirmPulseActive)
    {
        confirmPulseTimer += dt * 5.0f;
        if (confirmPulseTimer > 3.14159f * 3) { confirmPulseActive = false; confirmPulseTimer = 0; }
        float p = (sinf(confirmPulseTimer) + 1.0f) / 2.0f;
        DrawRectangleRounded(
            { confirmBtn.x - 4, confirmBtn.y - 4, confirmBtn.width + 8, confirmBtn.height + 8 },
            0.25f, 8, { 200, 80, 80, (unsigned char)(p * 80.0f) });
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

    const char* confirmLabel = canConfirm ? "CONFIRM BOOKING" : "SELECT SHOWTIME AND SEAT TYPE";
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
            note += m.title;
            note += "  ";
            note += m.shows[selectedShow].time;
            note += "  ";
            note += seatTypes[selectedSeat].name;
            ShowToast(note);
            selectedShow = -1; selectedSeat = -1;
        }
        else
        {
            confirmPulseActive = true;
            confirmPulseTimer = 0.0f;
            jumpToShowtime = true;
            showtimePulse = 0.0f;
        }
    }

    Rectangle backBtn = { (float)(panelX + 12),(float)(panelY + 12),110,34 };
    bool hoverBack = CheckCollisionPointRec(mouse, backBtn);
    DrawRectangleRounded(backBtn, 0.3f, 6,
        hoverBack ? Color{ 72, 130, 255, 200 } : Color{ 30, 35, 60, 220 });
    DrawRectangleRoundedLines(backBtn, 0.3f, 6,
        hoverBack ? ACCENT : BORDER_NORMAL);
    Vector2 bkSz = MeasureTextEx(font, "< BACK", 13, 1);
    DrawTextEx(font, "< BACK",
        { backBtn.x + backBtn.width / 2 - bkSz.x / 2, backBtn.y + backBtn.height / 2 - bkSz.y / 2 },
        13, 1, hoverBack ? WHITE : TEXT_PRIMARY);

    if (clicked && hoverBack)
    {
        selectedMovie = -1;
        selectedShow = -1;
        selectedSeat = -1;
        jumpToShowtime = false;
        confirmPulseActive = false;
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
        Rectangle navRect = { navX,0,130,(float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
        {
            activeNav = i;
            entranceTimer = 0.0f;
            AppState targets[] = { MAIN, CINEMAS, TICKETS, PROFILE };
            prof_pendingNav = targets[i];
        }
    }

    int filterBarY = navH + 12;
    int filterBarH = 36;
    int cardW = 180, cardH = 230, cardSpacing = 16, cardsStartX = 32;
    int listY = filterBarY + filterBarH + 14;

    std::vector<int> filtered = FilteredIndices();

    int totalCardsW = (int)filtered.size() * (cardW + cardSpacing);
    float maxScroll = (float)std::max(0, totalCardsW - (screenW - cardsStartX * 2));

    float wheel = GetMouseWheelMove();
    if (selectedMovie < 0 && wheel != 0)
    {
        cardScrollTarget -= wheel * 60.0f;
        if (cardScrollTarget < 0) cardScrollTarget = 0;
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
                    selectedMovie = filtered[fi];
                    entranceTimer = 0;
                    jumpToShowtime = false;
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

    for (int sy = 0; sy < screenH; sy += 4)
        DrawRectangle(0, sy, screenW, 1, { 0, 0, 0, 12 });

    DrawRectangle(0, 0, screenW, navH, { 10,12,28,220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "Gekoya", { 32,(float)(navH / 2) - 11 }, 22, 1.5f,
        { TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA });

    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        float navY = (float)(navH / 2) - 7;
        bool  isA = (activeNav == i);
        DrawTextEx(font, navItems[i], { navX,navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA }
        : Color{ TEXT_SECONDARY.r,TEXT_SECONDARY.g,TEXT_SECONDARY.b,PA });
        if (isA) DrawRectangle((int)navX, navH - 2,
            (int)MeasureTextEx(font, navItems[i], 12, 1).x, 2, ACCENT);
    }

    Rectangle searchBox = { (float)(screenW - 560),(float)(navH / 2 - 16),220,32 };
    if (clicked) searchActive = CheckCollisionPointRec(mouse, searchBox);
    if (searchActive)
    {
        if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery.empty()) searchQuery.pop_back();
        int k = GetCharPressed();
        while (k > 0) { if (searchQuery.size() < 32) searchQuery += (char)k; k = GetCharPressed(); }
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
    DrawRectangleRounded(
        { (float)(screenW - 220), (float)(navH / 2 - 14), greetSz.x + 20, 28 },
        0.3f, 6, { 30, 40, 70, 180 });
    DrawTextEx(font, greeting.c_str(),
        { (float)(screenW - 210),(float)(navH / 2 - 7) }, 13, 1,
        { TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA });

    Rectangle logoutBtn = { (float)(screenW - 105),(float)(navH / 2 - 14),88,28 };

    DrawRectangle(0, 0, screenW, navH, { 10,12,28,220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "Gekoya", { 32,(float)(navH / 2) - 11 }, 22, 1.5f,
        { TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA });

    for (int i = 0; i < 4; i++)
    {
        float navX = 200.0f + i * 150.0f;
        float navY = (float)(navH / 2) - 7;
        bool  isA = (activeNav == i);
        DrawTextEx(font, navItems[i], { navX,navY }, 12, 1,
            isA ? Color{ TEXT_PRIMARY.r,TEXT_PRIMARY.g,TEXT_PRIMARY.b,PA }
        : Color{ TEXT_SECONDARY.r,TEXT_SECONDARY.g,TEXT_SECONDARY.b,PA });
        if (isA) DrawRectangle((int)navX, navH - 2,
            (int)MeasureTextEx(font, navItems[i], 12, 1).x, 2, ACCENT);
    }

    logoutBtn = { (float)(screenW - 105),(float)(navH / 2 - 14),88,28 };
    bool hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);
    DrawRectangleRounded(logoutBtn, 0.3f, 6, hoverLogout ? Color{ 180,50,50,220 } : Color{ 80,30,30,180 });
    DrawRectangleRoundedLines(logoutBtn, 0.3f, 6, hoverLogout ? Color{ 220,80,80,255 } : Color{ 140,50,50,200 });
    Vector2 loSz = MeasureTextEx(font, "LOG OUT", 11, 1);
    DrawTextEx(font, "LOG OUT",
        { logoutBtn.x + logoutBtn.width / 2 - loSz.x / 2,
          logoutBtn.y + logoutBtn.height / 2 - loSz.y / 2 },
        11, 1, hoverLogout ? WHITE : Color{ 200,100,100,255 });

    if (clicked && hoverLogout)
    {
        selectedMovie = -1;
        searchActive = false;
        filterGenre = 0;
        entranceTimer = 0.0f;
        particlesInit = false;
        cardScrollX = 0;
        cardScrollTarget = 0;
        jumpToShowtime = false;
        confirmPulseActive = false;
        sessionUser.username = "";
        sessionUser.email = "";
        EndDrawing();
        return AUTH;
    }

    for (int i = 0; i < GENRE_COUNT; i++)
    {
        float fx = 32.0f + i * 110.0f;
        float fw = MeasureTextEx(font, genreFilters[i], 11, 1).x + 20;
        Rectangle fr = { fx,(float)filterBarY,fw,(float)filterBarH - 6 };
        bool sel = (filterGenre == i);
        bool hov = CheckCollisionPointRec(mouse, fr);
        DrawRectangleRounded(fr, 0.3f, 6, sel ? ACCENT : (hov ? Color{ 40,50,80,200 } : Color{ 20,22,40,180 }));
        DrawRectangleRoundedLines(fr, 0.3f, 6, sel ? ACCENT : (hov ? BORDER_FOCUS : BORDER_NORMAL));
        DrawTextEx(font, genreFilters[i], { fx + 10,(float)filterBarY + 8 }, 11, 1,
            sel ? WHITE : (hov ? TEXT_PRIMARY : TEXT_SECONDARY));
        if (clicked && hov) { filterGenre = i; cardScrollTarget = 0; }
    }

    char countBuf[32];
    snprintf(countBuf, 32, "%d FILMS", (int)filtered.size());
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
                int   idx = filtered[fi];
                float cx = (float)(cardsStartX + fi * (cardW + cardSpacing)) - cardScrollX;
                float cy = (float)listY;
                if (cx + cardW < 0 || cx > screenW) continue;
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
            int   trackY = screenH - 46;
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
    for (auto& mv : allMovies)
        for (auto& sh : mv.shows)
            if (sh.available) availableShows++;

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