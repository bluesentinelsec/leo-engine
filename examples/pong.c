// examples/pong.c
#include "leo/color.h"
#include "leo/engine.h"
#include "leo/graphics.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>

/* ---- Config ---- */
#define WIN_W 800
#define WIN_H 480
#define NET_SEG_H 16
#define NET_GAP 10

/* Paddle */
#define PAD_W 12
#define PAD_H 80
#define PAD_MARGIN 32
#define PAD_SPEED 420.0f /* px/s */

/* Ball */
#define BALL_R 8.0f
#define BALL_SPEED 310.0f /* starting speed px/s */
#define BALL_MAXSPD 720.0f
#define BALL_ACCEL 1.015f /* speed multiplier every paddle hit */
#define BALL_SPIN 6.0f    /* how strongly impact offset changes vy */

/* Colors */
static const leo_Color COL_BG = {30, 30, 38, 255};
static const leo_Color COL_NET = {200, 200, 200, 255};
static const leo_Color COL_P1 = {30, 200, 120, 255};
static const leo_Color COL_P2 = {200, 80, 30, 255};
static const leo_Color COL_BALL = {240, 240, 240, 255};

/* ---- Game state ---- */
typedef struct
{
    float x, y;
    float vy; /* Only AI needs vy target; motion uses vx,vy below */
} Paddle;

typedef struct
{
    float x, y;
    float vx, vy;
    float speed;
} Ball;

static Paddle p1, p2;
static Ball ball;
static int scoreL = 0, scoreR = 0;

/* ---- Utils ---- */
static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
static float sgnf(float v)
{
    return (v < 0.0f) ? -1.0f : 1.0f;
}

static void draw_net(void)
{
    int x = WIN_W / 2;
    for (int y = NET_GAP; y < WIN_H - NET_GAP; y += NET_SEG_H + NET_GAP)
    {
        leo_DrawLine(x, y, x, y + NET_SEG_H, COL_NET);
    }
}

static void reset_ball(int dir /* -1 = left serve, +1 = right serve */)
{
    ball.x = WIN_W * 0.5f;
    ball.y = WIN_H * 0.5f;
    ball.speed = BALL_SPEED;

    /* Launch with a small random vertical angle */
    float angles[] = {-0.35f, -0.25f, -0.15f, 0.15f, 0.25f, 0.35f};
    int idx = rand() % (int)(sizeof(angles) / sizeof(angles[0]));
    float a = angles[idx];

    ball.vx = dir * ball.speed * cosf(a);
    ball.vy = ball.speed * sinf(a);
}

static void reset_game(void)
{
    p1.x = PAD_MARGIN;
    p1.y = (WIN_H - PAD_H) * 0.5f;
    p2.x = WIN_W - PAD_MARGIN - PAD_W;
    p2.y = (WIN_H - PAD_H) * 0.5f;

    scoreL = scoreR = 0;
    reset_ball((rand() & 1) ? -1 : +1);
}

/* Simple AI: move paddle center toward ball.y with capped speed */
static void ai_update(Paddle *p, float dt)
{
    float target = ball.y - PAD_H * 0.5f;
    float dy = target - p->y;
    float maxstep = PAD_SPEED * dt;

    if (dy > maxstep)
        dy = maxstep;
    else if (dy < -maxstep)
        dy = -maxstep;

    p->y += dy;
    p->y = clampf(p->y, 0.0f, (float)(WIN_H - PAD_H));
}

/* Circle vs AABB collision (returns surface normal in *nx if hit) */
static int circle_vs_aabb(float cx, float cy, float r, float rx, float ry, float rw, float rh, float *nx)
{
    float nearestX = clampf(cx, rx, rx + rw);
    float nearestY = clampf(cy, ry, ry + rh);

    float dx = cx - nearestX;
    float dy = cy - nearestY;
    float d2 = dx * dx + dy * dy;
    if (d2 <= r * r)
    {
        /* Determine whether we hit left/right face (we only care about x normal for Pong) */
        float center = ry + rh * 0.5f;
        (void)center; /* not needed right now */
        if (cx < rx)
            *nx = -1.0f; /* hit left face of box */
        else if (cx > rx + rw)
            *nx = 1.0f; /* hit right face */
        else
            *nx = (dx >= 0) ? 1.0f : -1.0f; /* inside vertically; choose by dx */
        return 1;
    }
    return 0;
}

/* Handle ball/paddle/world collisions and scoring */
static void physics_update(float dt)
{
    /* Integrate */
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    /* Top/bottom bounce */
    if (ball.y - BALL_R < 0.0f)
    {
        ball.y = BALL_R;
        ball.vy = fabsf(ball.vy);
    }
    else if (ball.y + BALL_R > WIN_H)
    {
        ball.y = WIN_H - BALL_R;
        ball.vy = -fabsf(ball.vy);
    }

    /* Left/right score */
    if (ball.x + BALL_R < 0)
    {
        scoreR++;
        reset_ball(+1);
        return;
    }
    if (ball.x - BALL_R > WIN_W)
    {
        scoreL++;
        reset_ball(-1);
        return;
    }

    /* Paddles */
    float nx;

    /* Left paddle AABB */
    if (circle_vs_aabb(ball.x, ball.y, BALL_R, p1.x, p1.y, PAD_W, PAD_H, &nx) && ball.vx < 0.0f)
    {
        /* Reflect horizontally */
        ball.vx = fabsf(ball.vx);

        /* Add "spin": based on where we hit relative to paddle center */
        float rel = (ball.y - (p1.y + PAD_H * 0.5f)) / (PAD_H * 0.5f); /* -1..1 */
        ball.vy += rel * BALL_SPIN * ball.speed;

        /* Nudge out to avoid sticking */
        ball.x = p1.x + PAD_W + BALL_R + 1.0f;

        /* Speed up a touch */
        ball.speed = fminf(ball.speed * BALL_ACCEL, BALL_MAXSPD);
        float len = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
        if (len > 0.0f)
        {
            ball.vx = (ball.vx / len) * ball.speed;
            ball.vy = (ball.vy / len) * ball.speed;
        }
    }

    /* Right paddle AABB */
    if (circle_vs_aabb(ball.x, ball.y, BALL_R, p2.x, p2.y, PAD_W, PAD_H, &nx) && ball.vx > 0.0f)
    {
        ball.vx = -fabsf(ball.vx);

        float rel = (ball.y - (p2.y + PAD_H * 0.5f)) / (PAD_H * 0.5f);
        ball.vy += rel * BALL_SPIN * ball.speed;

        ball.x = p2.x - BALL_R - 1.0f;

        ball.speed = fminf(ball.speed * BALL_ACCEL, BALL_MAXSPD);
        float len = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
        if (len > 0.0f)
        {
            ball.vx = (ball.vx / len) * ball.speed;
            ball.vy = (ball.vy / len) * ball.speed;
        }
    }
}

/* Minimal HUD: 2x2 pips per score (no font API yet) */
static void draw_score_pips(void)
{
    const int pipR = 4;
    const int gap = 16;

    for (int i = 0; i < scoreL && i < 10; ++i)
    {
        int row = i / 5;
        int col = i % 5;
        int cx = 40 + col * gap;
        int cy = 40 + row * gap;
        leo_DrawCircle(cx, cy, (float)pipR, COL_P1);
    }
    for (int i = 0; i < scoreR && i < 10; ++i)
    {
        int row = i / 5;
        int col = i % 5;
        int cx = WIN_W - 40 - col * gap;
        int cy = 40 + row * gap;
        leo_DrawCircle(cx, cy, (float)pipR, COL_P2);
    }
}

int main(void)
{
    srand((unsigned int)time(NULL));

    if (!leo_InitWindow(WIN_W, WIN_H, "Leo Pong"))
    {
        return -1;
    }
    leo_SetTargetFPS(60);

    reset_game();

    while (!leo_WindowShouldClose())
    {
        /* Use last frame's dt; first frame will be ~0, harmless */
        float dt = leo_GetFrameTime();

        /* Simple AI (no input module in this example) */
        ai_update(&p1, dt);
        ai_update(&p2, dt);

        physics_update(dt);

        /* ---- Render ---- */
        leo_BeginDrawing();
        leo_ClearBackground(COL_BG.r, COL_BG.g, COL_BG.b, COL_BG.a);

        /* Net */
        draw_net();

        /* Paddles */
        leo_DrawRectangle((int)p1.x, (int)p1.y, PAD_W, PAD_H, COL_P1);
        leo_DrawRectangle((int)p2.x, (int)p2.y, PAD_W, PAD_H, COL_P2);

        /* Ball */
        leo_DrawCircle((int)ball.x, (int)ball.y, BALL_R, COL_BALL);

        /* Score pips */
        draw_score_pips();

        leo_EndDrawing();
    }

    leo_CloseWindow();
    return 0;
}
