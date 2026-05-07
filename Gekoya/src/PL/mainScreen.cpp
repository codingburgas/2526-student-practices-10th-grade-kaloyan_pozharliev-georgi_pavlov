#include "mainScreen.h"
#include "../colors.h"
#include <string>
#include <cmath>
#include <vector>

struct MovieCard
{
    const char* title;
    const char* genre;
    const char* rating;
    const char* duration;
    Color accent;
};

static std::vector<MovieCard> movies = {
    { "DUNE: PART TWO",    "SCI-FI",   "8.5", "166 MIN", { 80,  130, 255, 255 } },
    { "OPPENHEIMER",       "DRAMA",    "8.9", "180 MIN", { 255, 140,  60, 255 } },
    { "THE BATMAN",        "ACTION",   "7.8", "176 MIN", { 60,  180, 255, 255 } },
    { "POOR THINGS",       "FANTASY",  "7.9", "141 MIN", { 180,  80, 255, 255 } },
    { "PAST LIVES",        "ROMANCE",  "7.8", "106 MIN", { 80,  220, 160, 255 } },
};

static int hoveredCard = -1;
static int selectedCard = -1;

static const char* navItems[] = { "MOVIES", "CINEMAS", "MY TICKETS", "PROFILE" };
static int activeNav = 0;

AppState mainScreen(Font font, SessionUser& sessionUser)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float time = (float)GetTime();
    float pulse = (sinf(time * 0.8f) + 1.0f) / 2.0f;

    Vector2 mouse = GetMousePosition();
    bool    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int navH = 64;
    int heroH = 200;
    int sectionY = navH + heroH + 24;
    int cardW = 180;
    int cardH = 260;
    int cardSpacing = 20;
    int cardsStartX = 40;

    // --- Nav bar hit detection ---
    for (int i = 0; i < 4; i++)
    {
        float navItemW = MeasureTextEx(font, navItems[i], 12, 1).x + 32;
        float navX = 180.0f + i * 140.0f;
        Rectangle navRect = { navX, 0, navItemW, (float)navH };
        if (clicked && CheckCollisionPointRec(mouse, navRect))
            activeNav = i;
    }

    // --- Card hover ---
    hoveredCard = -1;
    for (int i = 0; i < (int)movies.size(); i++)
    {
        float cx = (float)(cardsStartX + i * (cardW + cardSpacing));
        float cy = (float)sectionY + 40;
        Rectangle cardRect = { cx, cy, (float)cardW, (float)cardH };
        if (CheckCollisionPointRec(mouse, cardRect))
        {
            hoveredCard = i;
            if (clicked) selectedCard = i;
        }
    }

    BeginDrawing();
    ClearBackground(BG_DARK);

    // --- Ambient glow circles (matching auth screens) ---
    for (int r = 280; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 280.0f;
        unsigned char a = (unsigned char)(t * t * (18.0f + pulse * 8.0f));
        DrawCircle((int)(screenW * 0.08f), (int)(screenH * 0.25f), (float)r, Color{ 40, 90, 255, a });
    }
    for (int r = 300; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 300.0f;
        unsigned char a = (unsigned char)(t * t * (16.0f + pulse * 6.0f));
        DrawCircle((int)(screenW * 0.92f), (int)(screenH * 0.75f), (float)r, Color{ 50, 80, 220, a });
    }
    for (int r = 220; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 220.0f;
        unsigned char a = (unsigned char)(t * t * (10.0f + pulse * 4.0f));
        DrawCircle((int)(screenW * 0.88f), (int)(screenH * 0.12f), (float)r, Color{ 80, 50, 200, a });
    }
    for (int r = 180; r >= 0; r -= 14)
    {
        float t = 1.0f - (float)r / 180.0f;
        unsigned char a = (unsigned char)(t * t * (8.0f + pulse * 4.0f));
        DrawCircle((int)(screenW * 0.5f), (int)(screenH * 0.5f), (float)r, Color{ 55, 95, 210, a });
    }

    // =========================================================
    // NAV BAR
    // =========================================================
    DrawRectangle(0, 0, screenW, navH, Color{ 10, 12, 28, 220 });
    DrawRectangle(0, navH - 1, screenW, 1, BORDER_NORMAL);

    // Logo
    DrawTextEx(font, "Gekoya", { 32, (float)(navH / 2) - 11 }, 22, 1.5f, TEXT_PRIMARY);

    // Nav items
    for (int i = 0; i < 4; i++)
    {
        float navX = 180.0f + i * 140.0f;
        float navY = (float)(navH / 2) - 7;
        bool  isActive = (activeNav == i);
        Color col = isActive ? TEXT_PRIMARY : TEXT_SECONDARY;
        DrawTextEx(font, navItems[i], { navX, navY }, 12, 1, col);
        if (isActive)
            DrawRectangle((int)navX, navH - 2, (int)MeasureTextEx(font, navItems[i], 12, 1).x, 2, ACCENT);
    }

    // User greeting (top right)
    std::string greeting = "HI, " + sessionUser.username;
    Vector2 greetSize = MeasureTextEx(font, greeting.c_str(), 12, 1);
    DrawTextEx(font, greeting.c_str(), { (float)(screenW - 120), (float)(navH / 2) - 7 }, 12, 1, TEXT_SECONDARY);

    // =========================================================
    // HERO BANNER
    // =========================================================
    int heroY = navH + 20;
    DrawRectangleRounded({ 20, (float)heroY, (float)(screenW - 40), (float)(heroH - 10) }, 0.06f, 10, BG_CARD);
    DrawRectangleRoundedLines({ 20, (float)heroY, (float)(screenW - 40), (float)(heroH - 10) }, 0.06f, 10, BORDER_NORMAL);

    // Hero glow accent
    for (int r = 120; r >= 0; r -= 10)
    {
        float t = 1.0f - (float)r / 120.0f;
        unsigned char a = (unsigned char)(t * t * (30.0f + pulse * 10.0f));
        DrawCircle((int)(screenW * 0.75f), heroY + heroH / 2, (float)r, Color{ 72, 120, 255, a });
    }

    DrawTextEx(font, "NOW SHOWING", { 48, (float)(heroY + 28) }, 11, 1, ACCENT);
    DrawTextEx(font, "Book Your Next", { 48, (float)(heroY + 48) }, 26, 1.5f, TEXT_PRIMARY);
    DrawTextEx(font, "Cinema Experience", { 48, (float)(heroY + 82) }, 26, 1.5f, TEXT_PRIMARY);
    DrawTextEx(font, "Find movies, pick your seats, and enjoy the show.", { 48, (float)(heroY + 122) }, 13, 0.5f, TEXT_SECONDARY);

    // Hero CTA button
    Rectangle heroCta = { 48, (float)(heroY + 148), 160, 38 };
    bool hoverCta = CheckCollisionPointRec(mouse, heroCta);
    DrawRectangleRounded(heroCta, 0.35f, 8, hoverCta ? ACCENT_HOVER : ACCENT);
    Vector2 ctaTextSize = MeasureTextEx(font, "BROWSE MOVIES", 12, 1);
    DrawTextEx(font, "BROWSE MOVIES",
        { heroCta.x + heroCta.width / 2 - ctaTextSize.x / 2,
          heroCta.y + heroCta.height / 2 - ctaTextSize.y / 2 },
        12, 1, WHITE);

    DrawTextEx(font, "FEATURED MOVIES", { (float)cardsStartX, (float)(sectionY + 8) }, 11, 1, TEXT_SECONDARY);

    for (int i = 0; i < (int)movies.size(); i++)
    {
        float cx = (float)(cardsStartX + i * (cardW + cardSpacing));
        float cy = (float)(sectionY + 40);
        bool  hovered = (hoveredCard == i);
        bool  selected = (selectedCard == i);

        // Card shadow
        DrawRectangleRounded({ cx + 4, cy + 6, (float)cardW, (float)cardH }, 0.08f, 8, Color{ 0, 0, 0, 60 });

        // Card body
        DrawRectangleRounded({ cx, cy, (float)cardW, (float)cardH }, 0.08f, 8, BG_CARD);
        DrawRectangleRoundedLines({ cx, cy, (float)cardW, (float)cardH }, 0.08f, 8,
            selected ? movies[i].accent : (hovered ? BORDER_FOCUS : BORDER_NORMAL));

        // Accent color bar at top
        DrawRectangleRounded({ cx, cy, (float)cardW, 5 }, 0.5f, 4, movies[i].accent);

        // Fake poster area
        Color posterBg = { (unsigned char)(movies[i].accent.r / 4),
                           (unsigned char)(movies[i].accent.g / 4),
                           (unsigned char)(movies[i].accent.b / 4), 255 };
        DrawRectangle((int)cx + 12, (int)cy + 18, cardW - 24, 120, posterBg);
        DrawRectangleLines((int)cx + 12, (int)cy + 18, cardW - 24, 120, movies[i].accent);

        // Movie icon placeholder
        float iconX = cx + cardW / 2 - 16;
        float iconY = cy + 18 + 40;
        DrawRectangle((int)iconX, (int)iconY, 32, 28, movies[i].accent);
        DrawTriangle(
            { iconX + 10, iconY + 6 },
            { iconX + 10, iconY + 22 },
            { iconX + 26, iconY + 14 },
            posterBg);

        // Genre badge
        Vector2 genreSize = MeasureTextEx(font, movies[i].genre, 10, 1);
        float badgeX = cx + 12;
        float badgeY = cy + 148;
        DrawRectangleRounded({ badgeX, badgeY, genreSize.x + 12, 18 }, 0.4f, 4,
            Color{ movies[i].accent.r, movies[i].accent.g, movies[i].accent.b, 40 });
        DrawTextEx(font, movies[i].genre, { badgeX + 6, badgeY + 4 }, 10, 1, movies[i].accent);

        // Title
        DrawTextEx(font, movies[i].title, { cx + 12, cy + 175 }, 11, 1, TEXT_PRIMARY);

        // Rating & duration
        std::string ratingStr = std::string("★ ") + movies[i].rating;
        DrawTextEx(font, ratingStr.c_str(), { cx + 12, cy + 196 }, 11, 1, Color{ 255, 200, 50, 255 });
        DrawTextEx(font, movies[i].duration, { cx + 12, cy + 212 }, 10, 1, TEXT_SECONDARY);

        // Book button
        Rectangle bookBtn = { cx + 12, cy + (float)cardH - 36, (float)cardW - 24, 26 };
        bool hoverBook = CheckCollisionPointRec(mouse, bookBtn);
        DrawRectangleRounded(bookBtn, 0.35f, 6,
            hoverBook ? ACCENT_HOVER : Color{ movies[i].accent.r, movies[i].accent.g, movies[i].accent.b, 180 });
        Vector2 bookTextSize = MeasureTextEx(font, "BOOK NOW", 10, 1);
        DrawTextEx(font, "BOOK NOW",
            { bookBtn.x + bookBtn.width / 2 - bookTextSize.x / 2,
              bookBtn.y + bookBtn.height / 2 - bookTextSize.y / 2 },
            10, 1, WHITE);
    }

    int barY = screenH - 36;
    DrawRectangle(0, barY, screenW, 36, Color{ 10, 12, 28, 200 });
    DrawRectangle(0, barY, screenW, 1, BORDER_NORMAL);
    DrawTextEx(font, "3 CINEMAS NEARBY", { 32, (float)(barY + 11) }, 11, 1, TEXT_SECONDARY);
    DrawTextEx(font, "12 SHOWS TODAY", { 200, (float)(barY + 11) }, 11, 1, TEXT_SECONDARY);

    // Live dot
    float dotPulse = (sinf(time * 3.0f) + 1.0f) / 2.0f;
    unsigned char dotA = (unsigned char)(180 + dotPulse * 75);
    DrawCircle((int)(screenW - 120), barY + 18, 5, Color{ 80, 220, 120, dotA });
    DrawTextEx(font, "LIVE", { (float)(screenW - 110), (float)(barY + 11) }, 11, 1, Color{ 80, 220, 120, 255 });

    EndDrawing();

    return MAIN;
}