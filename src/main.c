#include "raylib.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

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
} Dir;

#define MOVES ((void (*[])(void)){move_left, move_right, move_down, move_up})

typedef struct {
    Vector2u from, to;
    u8       k;
    u8       sequence;
} Transition;

typedef struct {
    Vector2u position;
    u8       k;
    u8       sequence;
} Block;

#define BACKGROUND BLACK

#define SCREEN_X 480
#define SCREEN_Y SCREEN_X

#define ROWS 4
#define COLS ROWS

#define RECT_X (SCREEN_X / ROWS)
#define RECT_Y RECT_X

#define RECT_BORDER 0.05f

#define TEXT_Y       40
#define TEXT_SPACING (TEXT_Y / 10)

#define FPS_X 10.0f
#define FPS_Y FPS_X

#define ANIMATION_STEP (1.0f / 4.0f)

static u8   BOARD[ROWS][COLS] = {0};
static Font FONT;
static bool CAN_INJECT = false;
static u8   NO_MOVE = 0;

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

#define CAP_BLOCKS (1 << 6)
static Block BLOCKS[CAP_BLOCKS];
static u32   LEN_BLOCKS = 0;

#define CAP_DIRS (1 << 6)
static Dir DIRS[CAP_DIRS];
static u32 LEN_DIRS;

static void push_transition(const Vector2u from, const Vector2u to, const u8 k, const u8 sequence) {
    assert(LEN_TRANSITIONS < CAP_TRANSITIONS);
    TRANSITIONS[LEN_TRANSITIONS++] = (Transition){from, to, k, sequence};
}

static void push_block(const Vector2u position, const u8 k, const u8 sequence) {
    assert(LEN_BLOCKS < CAP_BLOCKS);
    BLOCKS[LEN_BLOCKS++] = (Block){position, k, sequence};
}

static void push_dir(const Dir dir) {
    assert(LEN_DIRS < CAP_DIRS);
    DIRS[LEN_DIRS++] = dir;
}

static Dir pop_dir(void) {
    assert(0 < LEN_DIRS);
    return DIRS[--LEN_DIRS];
}

static void push_keys(void) {
    for (;;) {
        const i32 key = GetKeyPressed();
        if (key == 0) {
            break;
        }

        switch (key) {
        case KEY_A: {
            push_dir(DIR_LEFT);
            break;
        }
        case KEY_D: {
            push_dir(DIR_RIGHT);
            break;
        }
        case KEY_S: {
            push_dir(DIR_DOWN);
            break;
        }
        case KEY_W: {
            push_dir(DIR_UP);
            break;
        }
        default: {
        }
        }
    }
}

static void slide_row(u8 row[COLS], u8 sequence) {
    u8 j = 0;

    for (u8 i = 0; i < COLS; ++i) {
        if (row[i] == 0) {
            continue;
        }

        while ((row[j] != 0) && (j < i)) {
            ++j;
        }

        if (i == j) {
            push_block((Vector2u){i, 0}, row[i], sequence);
            continue;
        }

        assert(row[j] == 0);

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
            push_block((Vector2u){i, 0}, row[i], sequence);
            continue;
        }

        push_transition((Vector2u){i, 0}, (Vector2u){i - 1, 0}, row[i], sequence);

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

        const u32 prev_transition = LEN_TRANSITIONS;
        const u32 prev_block = LEN_BLOCKS;
        move_row(row);
        for (u32 k = prev_transition; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.x = COLS - (TRANSITIONS[k].from.x + 1);
            TRANSITIONS[k].from.y = i;
            TRANSITIONS[k].to.x = COLS - (TRANSITIONS[k].to.x + 1);
            TRANSITIONS[k].to.y = i;
        }
        for (u32 k = prev_block; k < LEN_BLOCKS; ++k) {
            BLOCKS[k].position.x = COLS - (BLOCKS[k].position.x + 1);
            BLOCKS[k].position.y = i;
        }

        for (u8 j = 0; j < COLS; ++j) {
            BOARD[i][j] = row[COLS - (j + 1)];
        }
    }
}

static void move_left(void) {
    for (u8 i = 0; i < ROWS; ++i) {
        const u32 prev_transition = LEN_TRANSITIONS;
        const u32 prev_block = LEN_BLOCKS;
        move_row(BOARD[i]);
        for (u32 k = prev_transition; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = i;
            TRANSITIONS[k].to.y = i;
        }
        for (u32 k = prev_block; k < LEN_BLOCKS; ++k) {
            BLOCKS[k].position.y = i;
        }
    }
}

static void move_down(void) {
    for (u8 j = 0; j < COLS; ++j) {
        u8 col[ROWS];

        for (u8 i = 0; i < ROWS; ++i) {
            col[ROWS - (i + 1)] = BOARD[i][j];
        }

        const u32 prev_transition = LEN_TRANSITIONS;
        const u32 prev_block = LEN_BLOCKS;
        move_row(col);
        for (u32 k = prev_transition; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = ROWS - (TRANSITIONS[k].from.x + 1);
            TRANSITIONS[k].from.x = j;
            TRANSITIONS[k].to.y = ROWS - (TRANSITIONS[k].to.x + 1);
            TRANSITIONS[k].to.x = j;
        }
        for (u32 k = prev_block; k < LEN_BLOCKS; ++k) {
            BLOCKS[k].position.y = ROWS - (BLOCKS[k].position.x + 1);
            BLOCKS[k].position.x = j;
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

        const u32 prev_transition = LEN_TRANSITIONS;
        const u32 prev_block = LEN_BLOCKS;
        move_row(col);
        for (u32 k = prev_transition; k < LEN_TRANSITIONS; ++k) {
            TRANSITIONS[k].from.y = TRANSITIONS[k].from.x;
            TRANSITIONS[k].from.x = j;
            TRANSITIONS[k].to.y = TRANSITIONS[k].to.x;
            TRANSITIONS[k].to.x = j;
        }
        for (u32 k = prev_block; k < LEN_BLOCKS; ++k) {
            BLOCKS[k].position.y = BLOCKS[k].position.x;
            BLOCKS[k].position.x = j;
        }

        for (u8 i = 0; i < ROWS; ++i) {
            BOARD[i][j] = col[i];
        }
    }
}

static void pop_move(void) {
    if (LEN_DIRS == 0) {
        return;
    }

    Dir dir = pop_dir();
    MOVES[dir]();

    if (LEN_TRANSITIONS == 0) {
        NO_MOVE |= (1u << dir);
    } else {
        NO_MOVE = 0;
    }
}

static void draw_block(const Vector2 position, const u8 k) {
    DrawRectangleV(
        (Vector2){
            position.x + (RECT_X * (RECT_BORDER / 2.0f)),
            position.y + (RECT_Y * (RECT_BORDER / 2.0f)),
        },
        (Vector2){(RECT_X * (1.0f - RECT_BORDER)), (RECT_Y * (1.0f - RECT_BORDER))},
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
    draw_block(
        (Vector2){
            lerp((f32)transition.from.x * RECT_X, (f32)transition.to.x * RECT_X, t * t),
            lerp((f32)transition.from.y * RECT_X, (f32)transition.to.y * RECT_X, t * t),
        },
        transition.k);
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
        push_keys();

        if (LEN_TRANSITIONS == 0) {
            LEN_BLOCKS = 0;

            if (CAN_INJECT) {
                inject_block();
                CAN_INJECT = false;
            }

            pop_move();

            if (LEN_TRANSITIONS != 0) {
                continue;
            }

            if (NO_MOVE ==
                ((1u << DIR_LEFT) | (1u << DIR_RIGHT) | (1u << DIR_DOWN) | (1u << DIR_UP)))
            {
                break;
            }

            BeginDrawing();
            ClearBackground(BACKGROUND);

            for (u32 i = 0; i < ROWS; ++i) {
                for (u32 j = 0; j < COLS; ++j) {
                    if (BOARD[i][j] == 0) {
                        continue;
                    }
                    draw_block((Vector2){(f32)j * RECT_X, (f32)i * RECT_Y}, BOARD[i][j]);
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
        for (u32 i = 0; i < LEN_BLOCKS; ++i) {
            if (BLOCKS[i].sequence != sequence) {
                continue;
            }
            draw_block(
                (Vector2){(f32)BLOCKS[i].position.x * RECT_X, (f32)BLOCKS[i].position.y * RECT_Y},
                BLOCKS[i].k);
        }

        DrawFPS(FPS_X, FPS_Y);
        EndDrawing();

        t -= ANIMATION_STEP;

        if (t <= 0.0f) {
            if (++sequence < 3) {
                t += 1.0f;
            } else {
                LEN_TRANSITIONS = 0;
                LEN_BLOCKS = 0;
            }
        }
    }

    CloseWindow();

    return 0;
}
