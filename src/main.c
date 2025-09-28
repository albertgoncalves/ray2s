#include "raylib.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STATIC_ASSERT(condition) _Static_assert(condition, "!(" #condition ")")

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;

typedef struct {
    u8 x, y;
} Vector2u;

typedef enum {
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_UP,
    DIR_NONE,
    DIR_COUNT,
} Dir;

typedef struct {
    Vector2u from, to;
    u8       k;
    u8       sequence;
} Transition;

#define BACKGROUND RAYWHITE

#define SCREEN_X 480
#define SCREEN_Y SCREEN_X

#define ROWS 4
#define COLS ROWS
STATIC_ASSERT(ROWS == COLS);

#define RECT_X (SCREEN_X / ROWS)
#define RECT_Y RECT_X

#define RECT_BORDER 0.05f

#define TEXT_Y       40
#define TEXT_SPACING (TEXT_Y / 10)

#define FPS_X 10.0f
#define FPS_Y FPS_X

#define ANIMATION_STEP (1.0f / 3.0f)

static u8   BOARD[ROWS][COLS] = {0};
static Font FONT;
static bool CAN_INJECT = false;

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

static Vector2 TEXT_SIZES[LEN_TEXTS];

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

#define CAP_TRANSITIONS (1 << 6)
static Transition TRANSITIONS[CAP_TRANSITIONS];
static u32        LEN_TRANSITIONS = 0;

static Dir input_to_move(void) {
    Dir dir = DIR_NONE;

    u32 count = 0;
    if (IsKeyPressed(KEY_A)) {
        dir = DIR_LEFT;
        ++count;
    }
    if (IsKeyPressed(KEY_D)) {
        dir = DIR_RIGHT;
        ++count;
    }
    if (IsKeyPressed(KEY_S)) {
        dir = DIR_DOWN;
        ++count;
    }
    if (IsKeyPressed(KEY_W)) {
        dir = DIR_UP;
        ++count;
    }

    if (count != 1) {
        dir = DIR_NONE;
    }

    return dir;
}

static void push_transition(const Vector2u from, const Vector2u to, const u8 k, const u8 sequence) {
    assert(LEN_TRANSITIONS < CAP_TRANSITIONS);
    TRANSITIONS[LEN_TRANSITIONS++] = (Transition){from, to, k, sequence};
}

static void slide_row(u8 row[COLS], u8 sequence) {
    u8 j = 0;

    for (; j < COLS; ++j) {
        if (row[j] == 0) {
            break;
        }
        push_transition((Vector2u){j, 0}, (Vector2u){j, 0}, row[j], sequence);
    }

    for (u8 i = j + 1; i < COLS; ++i) {
        assert(i != j);

        if (row[i] == 0) {
            continue;
        }

        if (row[j] != 0) {
            push_transition((Vector2u){i, 0}, (Vector2u){i, 0}, row[i], sequence);
            continue;
        }

        push_transition((Vector2u){i, 0}, (Vector2u){j, 0}, row[i], sequence);

        row[j++] = row[i];
        row[i] = 0;

        CAN_INJECT = true;
    }
}

static void promote_row(u8 row[COLS], u8 sequence) {
    for (u8 i = 0; i < COLS; ++i) {
        if (row[i] == 0) {
            break;
        }

        if ((i == 0) || (row[i - 1] != row[i])) {
            push_transition((Vector2u){i, 0}, (Vector2u){i, 0}, row[i], sequence);
            continue;
        }

        push_transition((Vector2u){i, 0}, (Vector2u){i - 1, 0}, row[i - 1], sequence);

        ++row[i - 1];
        row[i] = 0;

        CAN_INJECT = true;
    }
}

static void move_row(u8 row[COLS]) {
    slide_row(row, 0);
    promote_row(row, 1);
    slide_row(row, 2);
}

static void move_right(void) {
    for (u8 i = 0; i < ROWS; ++i) {
        u8 row[COLS];

        for (u8 j = 0; j < COLS; ++j) {
            row[COLS - (j + 1)] = BOARD[i][j];
        }

        u32 prev = LEN_TRANSITIONS;
        move_row(row);
        for (u32 k = prev; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.x = COLS - (TRANSITIONS[k].from.x + 1);
            TRANSITIONS[k].from.y = i;
            TRANSITIONS[k].to.x = COLS - (TRANSITIONS[k].to.x + 1);
            TRANSITIONS[k].to.y = i;
        }

        for (u8 j = 0; j < COLS; ++j) {
            BOARD[i][j] = row[COLS - (j + 1)];
        }
    }
}

static void move_left(void) {
    for (u8 i = 0; i < ROWS; ++i) {
        u32 prev = LEN_TRANSITIONS;
        move_row(BOARD[i]);
        for (u32 k = prev; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = i;
            TRANSITIONS[k].to.y = i;
        }
    }
}

static void move_down(void) {
    for (u8 j = 0; j < COLS; ++j) {
        u8 col[ROWS];

        for (u8 i = 0; i < ROWS; ++i) {
            col[ROWS - (i + 1)] = BOARD[i][j];
        }

        u32 prev = LEN_TRANSITIONS;
        move_row(col);
        for (u32 k = prev; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = ROWS - (TRANSITIONS[k].from.x + 1);
            TRANSITIONS[k].from.x = j;
            TRANSITIONS[k].to.y = ROWS - (TRANSITIONS[k].to.x + 1);
            TRANSITIONS[k].to.x = j;
        }

        for (u8 i = 0; i < ROWS; ++i) {
            BOARD[i][j] = col[ROWS - (i + 1)];
        }
    }
}

static void move_up(void) {
    for (u8 j = 0; j < COLS; ++j) {
        u8 col[ROWS];

        for (u8 i = 0; i < ROWS; ++i) {
            col[i] = BOARD[i][j];
        }

        u32 prev = LEN_TRANSITIONS;
        move_row(col);
        for (u32 k = prev; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = TRANSITIONS[k].from.x;
            TRANSITIONS[k].from.x = j;
            TRANSITIONS[k].to.y = TRANSITIONS[k].to.x;
            TRANSITIONS[k].to.x = j;
        }

        for (u8 i = 0; i < ROWS; ++i) {
            BOARD[i][j] = col[i];
        }
    }
}

static void move_none(void) {
}

static void (*MOVES[DIR_COUNT])(void) = {move_left, move_right, move_down, move_up, move_none};

static void draw_block(const Vector2 position, const u8 k) {
    DrawRectangleV(
        (Vector2){
            position.x + (RECT_X * (RECT_BORDER / 2.0f)),
            position.y + (RECT_Y * (RECT_BORDER / 2.0f)),
        },
        (Vector2){
            (RECT_X * (1.0f - RECT_BORDER)),
            (RECT_Y * (1.0f - RECT_BORDER)),
        },
        COLORS[k - 1]);
    DrawTextEx(FONT,
               TEXTS[k - 1],
               (Vector2){
                   position.x + ((RECT_X / 2) - (TEXT_SIZES[k - 1].x / 2)),
                   position.y + ((RECT_Y / 2) - (TEXT_SIZES[k - 1].y / 2)),
               },
               TEXT_Y,
               TEXT_SPACING,
               BACKGROUND);
}

static f32 lerp(const f32 l, const f32 r, const f32 t) {
    return l + (t * (r - l));
}

static void draw_transition(const Transition transition, const f32 t) {
    // NOTE: See `https://blog.bruce-hill.com/6-useful-snippets`.
    const Vector2 position = {
        lerp((f32)transition.from.x * RECT_X, (f32)transition.to.x * RECT_X, t * t),
        lerp((f32)transition.from.y * RECT_X, (f32)transition.to.y * RECT_X, t * t),
    };
    draw_block(position, transition.k);
}

static void inject_block(void) {
    Vector2u available[ROWS * COLS];

    u32 count = 0;
    for (u8 i = 0; i < ROWS; ++i) {
        for (u8 j = 0; j < COLS; ++j) {
            if (BOARD[i][j] == 0) {
                available[count++] = (Vector2u){j, i};
            }
        }
    }

    if (count == 0) {
        return;
    }

    const Vector2u position = available[GetRandomValue(0, (i32)(count - 1))];
    BOARD[position.y][position.x] = (u8)GetRandomValue(1, 2);
}

i32 main(void) {
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(SCREEN_X, SCREEN_Y, "ray2s");
    SetTargetFPS(60);

    FONT = GetFontDefault();

    for (u32 i = 0; i < LEN_TEXTS; ++i) {
        TEXT_SIZES[i] = MeasureTextEx(FONT, TEXTS[i], TEXT_Y, TEXT_SPACING);
    }

    inject_block();
    inject_block();

    f32 t = 0.0f;
    u8  sequence = 0;

    while (!WindowShouldClose()) {
        if (LEN_TRANSITIONS == 0) {
            if (CAN_INJECT) {
                inject_block();
                CAN_INJECT = false;
            }

            const Dir dir = input_to_move();

            MOVES[dir]();

            if (LEN_TRANSITIONS != 0) {
                continue;
            }

            BeginDrawing();
            ClearBackground(BACKGROUND);

            for (u32 i = 0; i < ROWS; ++i) {
                for (u32 j = 0; j < COLS; ++j) {
                    if (BOARD[i][j] == 0) {
                        continue;
                    }

                    const Vector2 position = {(f32)j * RECT_X, (f32)i * RECT_Y};
                    draw_block(position, BOARD[i][j]);
                }
            }

            DrawFPS(FPS_X, FPS_Y);
            EndDrawing();

            continue;
        }

        if (t <= 0.0f) {
            sequence = 0;
            t += 1.0f;
        }

        BeginDrawing();
        ClearBackground(BACKGROUND);

        for (u32 i = 0; i < LEN_TRANSITIONS; ++i) {
            if (TRANSITIONS[i].sequence != sequence) {
                continue;
            }
            draw_transition(TRANSITIONS[i], 1.0f - t);
        }

        DrawFPS(FPS_X, FPS_Y);
        EndDrawing();

        t -= ANIMATION_STEP;

        if (t <= 0.0f) {
            if (++sequence < 3) {
                t += 1.0f;
            } else {
                LEN_TRANSITIONS = 0;
            }
        }
    }

    CloseWindow();

    return 0;
}
