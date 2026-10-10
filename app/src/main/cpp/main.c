#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CAP 4
#define MAXT 16

static const Color ANSI[14] = { RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, SKYBLUE, PINK, BROWN, LIME, DARKBLUE, MAGENTA, GRAY, WHITE };

typedef struct { uint8_t t[MAXT][CAP]; uint8_t len[MAXT]; uint8_t n; } State;

static double mb32(uint32_t *a) {
    uint32_t t; *a += 0x6D2B79F5u; t = *a;
    t = (t ^ (t >> 15)) * (t | 1u); t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return (double)(t ^ (t >> 14)) / 4294967296.0;
}
static int top(const State *s, int i) { return s->len[i] ? s->t[i][s->len[i] - 1] : -1; }
static int blk(const State *s, int i) {
    int n = 0, c = top(s, i);
    while (n < s->len[i] && s->t[i][s->len[i] - 1 - n] == c) n++;
    return n;
}
static int uniform(const State *s, int i) {
    for (int k = 1; k < s->len[i]; k++) if (s->t[i][k] != s->t[i][0]) return 0;
    return 1;
}
static const char *why(const State *s, int a, int b) {
    if (!s->len[a]) return "Empty tube.";
    if (a == b) return "Same tube.";
    if (s->len[b] >= CAP) return "Full.";
    if (s->len[b] && top(s, b) != top(s, a)) return "No match.";
    return NULL;
}
static State pour(State s, int a, int b) {
    int n = blk(&s, a), c = top(&s, a), room = CAP - s.len[b];
    if (room < n) n = room;
    s.len[a] -= n;
    while (n--) s.t[b][s.len[b]++] = (uint8_t)c;
    return s;
}
static int won(const State *s) {
    for (int i = 0; i < s->n; i++) if (s->len[i] && !(s->len[i] == CAP && uniform(s, i))) return 0;
    return 1;
}

static State generate_level(int colors, int depth, int level) {
    State s; memset(&s, 0, sizeof s); s.n = colors + 2; 
    for (int c = 0; c < colors; c++) { for (int k = 0; k < CAP; k++) s.t[c][k] = (uint8_t)c; s.len[c] = CAP; }
    uint32_t rng = 12345 + level * 7919; 
    
    for (int i = 0; i < depth; i++) {
        int src[MAXT], ns = 0, dst[MAXT], nd = 0;
        for (int j = 0; j < s.n; j++) if (s.len[j]) src[ns++] = j;
        int a = src[(int)(mb32(&rng) * ns)];
        for (int j = 0; j < s.n; j++) if (j != a && s.len[j] < CAP) dst[nd++] = j;
        if (!nd) continue;
        int b = dst[(int)(mb32(&rng) * nd)], mx = blk(&s, a), room = CAP - s.len[b];
        if (room < mx) mx = room;
        int n = 1 + (int)(mb32(&rng) * mx), c = top(&s, a);
        s.len[a] -= n; while (n--) s.t[b][s.len[b]++] = (uint8_t)c;
    }
    return s;
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1080, 1920, "Water Sort Mobile");
    SetTargetFPS(60);

    int level = 1;
    State s = generate_level(4, 30, level);
    int selected = -1;
    char msg[100] = "Level 1 - Tap a tube!";

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || (GetTouchPointCount() > 0 && IsGestureDetected(GESTURE_TAP))) {
            Vector2 touch = GetTouchPosition(0);
            
            if (won(&s)) { 
                level++;
                int cols = 4 + (level / 5);
                if (cols > 12) cols = 12;
                s = generate_level(cols, 30 + level * 5, level);
                sprintf(msg, "Level %d", level);
                selected = -1;
            } else {
                for(int i = 0; i < s.n; i++) {
                    int col = i % 4;
                    int row = i / 4;
                    Rectangle rec = { 100 + col * 220, 400 + row * 400, 150, 300 };
                    
                    if (CheckCollisionPointRec(touch, rec)) {
                        if (selected < 0) {
                            if (s.len[i] > 0) selected = i;
                        } else {
                            if (selected == i) { selected = -1; }
                            else if (!why(&s, selected, i)) {
                                s = pour(s, selected, i);
                                if(won(&s)) strcpy(msg, "COMPLETED! Tap for next.");
                                selected = -1;
                            }
                        }
                    }
                }
            }
        }
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(msg, 100, 200, 60, DARKGRAY);
        
        for(int i = 0; i < s.n; i++) {
            int col = i % 4;
            int row = i / 4;
            int x = 100 + col * 220;
            int y = 400 + row * 400;
            
            if (selected == i) y -= 50; 
            
            for (int j = 0; j < s.len[i]; j++) {
                DrawRectangle(x, y + 300 - (j+1)*75, 150, 75, ANSI[s.t[i][j]]);
            }
            DrawRectangleLinesEx((Rectangle){x, y, 150, 300}, 8, BLACK);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
