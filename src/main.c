#include "raylib.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT SCREEN_WIDTH

#define RECT_WIDTH  (SCREEN_WIDTH / 4)
#define RECT_HEIGHT RECT_WIDTH

#define TEXT_HEIGHT  48
#define TEXT_SPACING 4

static const char* TEXTS[] = {
    "2",
    "4",
    "8",
    "16",
    "32",
    "64",
    "128",
    "256",
    "512",
    "1024",
    "2048",
    "4096",
    "8192",
    "16384",
    "32768",
    "65536",
    "131072",
};

#define LEN_TEXTS (sizeof(TEXTS) / sizeof(TEXTS[0]))

static Color COLORS[LEN_TEXTS] = {
    BEIGE,
    LIGHTGRAY,
    GRAY,
    DARKGRAY,
    // YELLOW,
    // GOLD,
    ORANGE,
    PINK,
    RED,
    MAROON,
    GREEN,
    LIME,
    DARKGREEN,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    PURPLE,
    VIOLET,
    DARKPURPLE,
    // BROWN,
    // DARKBROWN,
};

static unsigned int into_index(unsigned int k) {
    unsigned int i = 0u;
    while (2u < k) {
        k >>= 1u;
        ++i;
    }
    return i;
}

static void draw_cell(float x, float y, unsigned int k) {
    const unsigned int i = into_index(k);
    DrawRectangleV((Vector2){x, y}, (Vector2){RECT_WIDTH, RECT_HEIGHT}, COLORS[i]);
    const Vector2 text_size = MeasureTextEx(GetFontDefault(), TEXTS[i], TEXT_HEIGHT, TEXT_SPACING);
    DrawTextEx(GetFontDefault(),
               TEXTS[i],
               (Vector2){
                   x + ((RECT_WIDTH / 2) - (text_size.x / 2)),
                   y + ((RECT_HEIGHT / 2) - (text_size.y / 2)),
               },
               TEXT_HEIGHT,
               TEXT_SPACING,
               WHITE);
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ray2s");
    SetTargetFPS(60);

    unsigned int k = 0;
    int          frames = 1;

    while (!WindowShouldClose()) {
        if ((frames % 20) == 0) {
            frames = 0;
            k = (k + 1) % LEN_TEXTS;
        }

        BeginDrawing();
        ClearBackground(WHITE);

        draw_cell(RECT_WIDTH, RECT_HEIGHT, 2u << k);

        DrawFPS(10.0f, 10.0f);
        EndDrawing();

        ++frames;
    }

    CloseWindow();

    return 0;
}
