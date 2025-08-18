Heck yeah—that’s a great next milestone. If you get the \*\*game loop “spine”\*\* right, everything else can attach cleanly.



Here’s a lean plan + drop-in C API to scaffold the loop and leave clean hooks for subsystems.



---



\# 1) Minimal, friendly game-loop API (fixed-step core)



\### Public header: `include/leo/app.h`



```c

\#pragma once

\#include <stdbool.h>



\#ifdef \_\_cplusplus

extern "C" {

\#endif



typedef struct leo\_app\* leo\_app\_t;     // opaque app context



typedef struct {

&nbsp;   int         width;         // window width  (px)

&nbsp;   int         height;        // window height (px)

&nbsp;   const char\* title;

&nbsp;   int         target\_hz;     // e.g. 60

&nbsp;   bool        vsync;         // true = rely on vsync; false = manual sleep

} leo\_app\_desc;



// Fixed-step update, interpolated render

typedef struct {

&nbsp;   bool (\*init)(leo\_app\_t, void\* user);

&nbsp;   void (\*shutdown)(leo\_app\_t, void\* user);

&nbsp;   void (\*update)(leo\_app\_t, float dt, void\* user);     // dt = 1/target\_hz

&nbsp;   void (\*render)(leo\_app\_t, float alpha, void\* user);  // alpha in \[0..1] for interpolation

} leo\_app\_callbacks;



// Run until quit (returns exit code)

int  leo\_app\_run(const leo\_app\_desc\* desc, const leo\_app\_callbacks\* cb, void\* user);



// Access to subsystems (opaque handles you can expand later)

typedef struct leo\_gfx\*     leo\_gfx\_t;

typedef struct leo\_input\*   leo\_input\_t;

typedef struct leo\_audio\*   leo\_audio\_t;



leo\_gfx\_t   leo\_app\_gfx(leo\_app\_t);

leo\_input\_t leo\_app\_input(leo\_app\_t);

leo\_audio\_t leo\_app\_audio(leo\_app\_t);



// Time queries (optional sugar)

double leo\_time\_now(leo\_app\_t);   // seconds since start

int    leo\_frame\_index(leo\_app\_t);



\#ifdef \_\_cplusplus

}

\#endif

```



\### Implementation sketch: `src/app.c`



```c

\#include "leo/app.h"

\#include "leo/error.h"

\#include <SDL3/SDL.h>

\#include <stdlib.h>



struct leo\_app {

&nbsp;   SDL\_Window\*   win;

&nbsp;   SDL\_Renderer\* ren;

&nbsp;   double        start\_s;

&nbsp;   int           frame;

&nbsp;   int           target\_hz;

&nbsp;   float         dt;

&nbsp;   bool          running;

&nbsp;   // future: pointers to input/audio/fs/etc

};



static double now\_s(void) {

&nbsp;   static double freq = 0.0;

&nbsp;   if (!freq) freq = 1.0 / (double)SDL\_GetPerformanceFrequency();

&nbsp;   return (double)SDL\_GetPerformanceCounter() \* freq;

}



int leo\_app\_run(const leo\_app\_desc\* desc, const leo\_app\_callbacks\* cb, void\* user) {

&nbsp;   if (SDL\_Init(SDL\_INIT\_VIDEO | SDL\_INIT\_GAMEPAD | SDL\_INIT\_AUDIO) != 0) {

&nbsp;       leo\_SetError("SDL\_Init: %s", SDL\_GetError()); return 1;

&nbsp;   }



&nbsp;   SDL\_Window\* w = SDL\_CreateWindow(desc->title, desc->width, desc->height, 0);

&nbsp;   if (!w) { leo\_SetError("CreateWindow: %s", SDL\_GetError()); SDL\_Quit(); return 2; }



&nbsp;   Uint32 flags = desc->vsync ? SDL\_RENDERER\_PRESENTVSYNC : 0;

&nbsp;   SDL\_Renderer\* r = SDL\_CreateRenderer(w, NULL, flags);

&nbsp;   if (!r) { leo\_SetError("CreateRenderer: %s", SDL\_GetError()); SDL\_DestroyWindow(w); SDL\_Quit(); return 3; }



&nbsp;   struct leo\_app app = {0};

&nbsp;   app.win = w; app.ren = r;

&nbsp;   app.start\_s = now\_s();

&nbsp;   app.frame = 0;

&nbsp;   app.target\_hz = (desc->target\_hz > 0 ? desc->target\_hz : 60);

&nbsp;   app.dt = 1.0f / (float)app.target\_hz;

&nbsp;   app.running = true;



&nbsp;   if (cb \&\& cb->init) {

&nbsp;       if (!cb->init(\&app, user)) { app.running = false; }

&nbsp;   }



&nbsp;   double prev = now\_s();

&nbsp;   double acc  = 0.0;



&nbsp;   while (app.running) {

&nbsp;       // Pump events (you can route into leo\_input later)

&nbsp;       SDL\_Event e;

&nbsp;       while (SDL\_PollEvent(\&e)) {

&nbsp;           if (e.type == SDL\_EVENT\_QUIT) app.running = false;

&nbsp;           // TODO: forward to input subsystem

&nbsp;       }



&nbsp;       double t = now\_s();

&nbsp;       double frame\_dt = t - prev;

&nbsp;       prev = t;

&nbsp;       if (frame\_dt > 0.25) frame\_dt = 0.25;   // avoid spiral of death

&nbsp;       acc += frame\_dt;



&nbsp;       // Fixed-step updates

&nbsp;       while (acc >= app.dt) {

&nbsp;           if (cb \&\& cb->update) cb->update(\&app, app.dt, user);

&nbsp;           acc -= app.dt;

&nbsp;           app.frame++;

&nbsp;       }



&nbsp;       // Interpolated render (alpha in \[0..1])

&nbsp;       float alpha = (float)(acc / app.dt);

&nbsp;       if (cb \&\& cb->render) cb->render(\&app, alpha, user);



&nbsp;       SDL\_RenderPresent(r);



&nbsp;       // If vsync is off, you can optionally sleep to target

&nbsp;       if (!desc->vsync) {

&nbsp;           double frame\_s = now\_s() - t;

&nbsp;           double target\_s = app.dt;

&nbsp;           if (frame\_s < target\_s) {

&nbsp;               SDL\_Delay((Uint32)((target\_s - frame\_s) \* 1000.0));

&nbsp;           }

&nbsp;       }

&nbsp;   }



&nbsp;   if (cb \&\& cb->shutdown) cb->shutdown(\&app, user);



&nbsp;   SDL\_DestroyRenderer(r);

&nbsp;   SDL\_DestroyWindow(w);

&nbsp;   SDL\_Quit();

&nbsp;   return 0;

}



// Accessors (keep SDL private)

leo\_gfx\_t   leo\_app\_gfx(leo\_app\_t a)   { (void)a; return (leo\_gfx\_t)(uintptr\_t)1; /\* TODO: your gfx ctx \*/ }

leo\_input\_t leo\_app\_input(leo\_app\_t a) { (void)a; return (leo\_input\_t)(uintptr\_t)1; }

leo\_audio\_t leo\_app\_audio(leo\_app\_t a) { (void)a; return (leo\_audio\_t)(uintptr\_t)1; }

double      leo\_time\_now(leo\_app\_t a)  { return now\_s() - a->start\_s; }

int         leo\_frame\_index(leo\_app\_t a){ return a->frame; }

```



> Why fixed-step? Deterministic \*\*updates\*\* (collisions/physics) + \*\*interpolated rendering\*\* give smooth visuals at any monitor refresh without exploding your sim when FPS dips.



---



\# 2) Where the subsystems plug in



Add subsystem singletons/handles inside `leo\_app` and route them through your opaque types:



\* \*\*Input\*\*: capture SDL events in the loop; update `leo\_input` state; expose polling (`leo\_input\_is\_key\_down`) and edge queries (`pressed/released`).

\* \*\*Images\*\*: use the opaque \*\*gfx context\*\* with `leo\_texture\_t` (you already sketched this), loading via memory/stream and caching by path.

\* \*\*Audio\*\*: keep a simple mixer abstraction first (play sound / music, volume, pause).

\* \*\*Font\*\*: bitmap font first; later SDL\\\_ttf or your own rasterizer.

\* \*\*Shapes\*\*: immediate-mode helpers (`leo\_draw\_rect`, `leo\_draw\_line`) in the gfx module.

\* \*\*Camera/view\*\*: 2D camera struct with transform; pass to draw calls or store on gfx.

\* \*\*Collisions/physics\*\*: start with AABB and grid; only add a physics lib if/when needed.

\* \*\*Actors/scene\*\*: keep thin—entities with components can come later; don’t block loop work.

\* \*\*FS \& I/O\*\*: design now for \*\*memory/stream\*\* inputs so Android/Web is easy later.

\* \*\*Threads/timers/sockets\*\*: put behind tiny wrappers; keep the main thread deterministic.



---



\# 3) CMake toggles (keep builds lean)



Add feature flags so CI can quickly sanity-check subsets:



```cmake

option(LEO\_WITH\_INPUT   "Enable input subsystem"   ON)

option(LEO\_WITH\_AUDIO   "Enable audio subsystem"   ON)

option(LEO\_WITH\_IMAGES  "Enable image loading"     ON)

\# …etc…



if(LEO\_WITH\_AUDIO)

&nbsp; target\_compile\_definitions(leo PUBLIC LEO\_WITH\_AUDIO=1)

&nbsp; # later: link to SDL\_mixer or your own decoder

endif()

```



---



\# 4) Suggested order of work (1–2 day bursts)



1\. \*\*Loop \& timing\*\* (above), quit handling, clear/present.

2\. \*\*Input\*\* (keyboard/mouse/gamepad); pressed/released/held.

3\. \*\*Textures\*\*: load → cache → draw (blit) → draw sub-rect.

4\. \*\*Camera\*\* (pan/zoom) + world→screen helpers.

5\. \*\*Sprites/animations\*\* (sprite sheet, frame timer).

6\. \*\*Text\*\* (bitmap font) or stub.

7\. \*\*Audio\*\* (play wav/ogg, volume).

8\. \*\*Collisions\*\* (AABB, grid queries).

9\. \*\*Particles\*\* (simple CPU emitter).

10\. \*\*Filesystem abstraction\*\* (path + memory).

11\. \*\*Threads/timers\*\* (deferred jobs, main-thread marshaling).



Each step can ship a tiny sample under `examples/` and a CI run.



---



\# 5) Testing hooks



\* Add a \*\*headless\*\* mode (`LEO\_HEADLESS`) that skips window/renderer but runs the update loop—great for CI.

\* Seeded RNG for deterministic smoke tests.

\* Frame budget assert (warn if `frame\_dt > 3\*dt`).



---



Want me to turn the loop code above into a pair of files in your tree (`include/leo/app.h`, `src/app.c`) and wire it into your existing CMake target (`leo`)? I can also add a tiny example (`examples/spin\_sprite.c`) that compiles in all three OS targets you already cover.



