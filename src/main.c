#include "raylib.h"

#include <stdint.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;

typedef float f32;

typedef struct {
    Vector2 position;
    u8      value;
    bool    alive;
} Cell;

#define RECT_X 120
#define RECT_Y RECT_X

#define ROWS 4
#define COLS ROWS

#define SCREEN_X (RECT_X * COLS)
#define SCREEN_Y (RECT_Y * ROWS)

#define RECT_BORDER 0.05f

#define BACKGROUND BLACK

#define MOVE_FRACTION 0.5f

#define TEXT_Y       40
#define TEXT_SPACING (TEXT_Y / 10)

#define FPS_X 2
#define FPS_Y 0

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

static const Color COLORS[LEN_TEXTS] = {
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

static void random_block(Cell cells[][COLS]) {
    u32 available = 0;
    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 0; x < COLS; ++x) {
            if (!cells[y][x].alive) {
                ++available;
            }
        }
    }

    if (available == 0) {
        return;
    }

    u32 index = (u32)GetRandomValue(0, (i32)(available - 1));

    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 0; x < COLS; ++x) {
            if (cells[y][x].alive) {
                continue;
            }
            if (index == 0) {
                cells[y][x] = (Cell){
                    .position =
                        (Vector2){
                            .x = (f32)x,
                            .y = (f32)y,
                        },
                    .value = (u8)GetRandomValue(0, 1),
                    .alive = true,
                };
                return;
            }
            --index;
        }
    }
}

static bool up(Cell cells[][COLS]) {
    bool change = false;

    for (u32 x = 0; x < COLS; ++x) {
        u32 end = 0;

        for (u32 y = end; y < ROWS; ++y) {
            if (!cells[y][x].alive) {
                continue;
            }
            if ((end != 0) && (cells[end - 1][x].value == cells[y][x].value)) {
                cells[end - 1][x].position = cells[y][x].position;
                ++cells[end - 1][x].value;
                cells[y][x].alive = false;
                change = true;
            } else if (y != end) {
                cells[end++][x] = cells[y][x];
                cells[y][x].alive = false;
                change = true;
            } else {
                ++end;
            }
        }
    }

    return change;
}

static bool down(Cell cells[][COLS]) {
    bool change = false;

    for (u32 x = 0; x < COLS; ++x) {
        u32 end = ROWS;

        for (u32 y = end; 0 < y; --y) {
            if (!cells[y - 1][x].alive) {
                continue;
            }
            if ((end != ROWS) && (cells[end][x].value == cells[y - 1][x].value)) {
                cells[end][x].position = cells[y - 1][x].position;
                ++cells[end][x].value;
                cells[y - 1][x].alive = false;
                change = true;
            } else if (y != end) {
                cells[--end][x] = cells[y - 1][x];
                cells[y - 1][x].alive = false;
                change = true;
            } else {
                --end;
            }
        }
    }

    return change;
}

static bool left(Cell cells[][COLS]) {
    bool change = false;

    for (u32 y = 0; y < ROWS; ++y) {
        u32 end = 0;

        for (u32 x = end; x < COLS; ++x) {
            if (!cells[y][x].alive) {
                continue;
            }
            if ((end != 0) && (cells[y][end - 1].value == cells[y][x].value)) {
                cells[y][end - 1].position = cells[y][x].position;
                ++cells[y][end - 1].value;
                cells[y][x].alive = false;
                change = true;
            } else if (x != end) {
                cells[y][end++] = cells[y][x];
                cells[y][x].alive = false;
                change = true;
            } else {
                ++end;
            }
        }
    }

    return change;
}

static bool right(Cell cells[][COLS]) {
    bool change = false;

    for (u32 y = 0; y < ROWS; ++y) {
        u32 end = COLS;

        for (u32 x = end; 0 < x; --x) {
            if (!cells[y][x - 1].alive) {
                continue;
            }
            if ((end != COLS) && (cells[y][end].value == cells[y][x - 1].value)) {
                cells[y][end].position = cells[y][x - 1].position;
                ++cells[y][end].value;
                cells[y][x - 1].alive = false;
                change = true;
            } else if (x != end) {
                cells[y][--end] = cells[y][x - 1];
                cells[y][x - 1].alive = false;
                change = true;
            } else {
                --end;
            }
        }
    }

    return change;
}

static bool is_done(Cell cells[][COLS]) {
    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 0; x < COLS; ++x) {
            if (!cells[y][x].alive) {
                return false;
            }
        }
    }

    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 1; x < COLS; ++x) {
            if (cells[y][x - 1].value == cells[y][x].value) {
                return false;
            }
        }
    }

    for (u32 x = 0; x < COLS; ++x) {
        for (u32 y = 1; y < ROWS; ++y) {
            if (cells[y - 1][x].value == cells[y][x].value) {
                return false;
            }
        }
    }

    return true;
}

static bool update(Cell cells[][COLS]) {
    bool pressed = false;
    bool change = false;

    for (;;) {
        const i32 key = GetKeyPressed();
        if (key == 0) {
            break;
        }

        pressed = true;

        if (key == KEY_W) {
            change |= up(cells);
        } else if (key == KEY_S) {
            change |= down(cells);
        } else if (key == KEY_A) {
            change |= left(cells);
        } else if (key == KEY_D) {
            change |= right(cells);
        }
    }

    if (pressed) {
        if (change) {
            random_block(cells);
        } else if (is_done(cells)) {
            return false;
        }
    }

    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 0; x < COLS; ++x) {
            if (!cells[y][x].alive) {
                continue;
            }
            cells[y][x].position.x += (((f32)x) - cells[y][x].position.x) * MOVE_FRACTION;
            cells[y][x].position.y += (((f32)y) - cells[y][x].position.y) * MOVE_FRACTION;
        }
    }

    return true;
}

static void draw(const Font* font, const Vector2* text_sizes, const Cell cells[][COLS]) {
    BeginDrawing();

    ClearBackground(BACKGROUND);

    for (u32 y = 0; y < ROWS; ++y) {
        for (u32 x = 0; x < COLS; ++x) {
            if (!cells[y][x].alive) {
                continue;
            }

            const Vector2 position = (Vector2){
                .x = cells[y][x].position.x * RECT_X,
                .y = cells[y][x].position.y * RECT_Y,
            };
            const u8 value = cells[y][x].value;

            DrawRectangleV(
                (Vector2){
                    .x = position.x + (RECT_X * (RECT_BORDER / 2.0f)),
                    .y = position.y + (RECT_Y * (RECT_BORDER / 2.0f)),
                },
                (Vector2){
                    .x = RECT_X * (1.0f - RECT_BORDER),
                    .y = RECT_Y * (1.0f - RECT_BORDER),
                },
                COLORS[value]);
            DrawTextEx(*font,
                       TEXTS[value],
                       (Vector2){
                           .x = position.x + ((RECT_X / 2) - (text_sizes[value].x / 2)),
                           .y = position.y + ((RECT_Y / 2) - (text_sizes[value].y / 2)),
                       },
                       TEXT_Y,
                       TEXT_SPACING,
                       BACKGROUND);
        }
    }

    DrawFPS(FPS_X, FPS_Y);

    EndDrawing();
}

i32 main(void) {
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(SCREEN_X, SCREEN_Y, "ray2s");
    SetTargetFPS(60);

    const Font font = GetFontDefault();

    static Vector2 text_sizes[LEN_TEXTS];

    for (u32 i = 0; i < LEN_TEXTS; ++i) {
        text_sizes[i] = MeasureTextEx(font, TEXTS[i], TEXT_Y, TEXT_SPACING);
    }

    static Cell cells[ROWS][COLS] = {0};

    random_block(cells);
    random_block(cells);

    while (!WindowShouldClose()) {
        if (!update(cells)) {
            break;
        }
        draw(&font, text_sizes, cells);
    }

    CloseWindow();

    return 0;
}
