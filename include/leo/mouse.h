#pragma once

#include "leo/export.h"
#include "leo/engine.h"   // leo_Vector2
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* Buttons (match SDL for convenience) */
	enum {
		LEO_MOUSE_BUTTON_LEFT   = 1, /* SDL_BUTTON_LEFT   */
		LEO_MOUSE_BUTTON_MIDDLE = 2, /* SDL_BUTTON_MIDDLE */
		LEO_MOUSE_BUTTON_RIGHT  = 3, /* SDL_BUTTON_RIGHT  */
		LEO_MOUSE_BUTTON_X1     = 4, /* SDL_BUTTON_X1     */
		LEO_MOUSE_BUTTON_X2     = 5  /* SDL_BUTTON_X2     */
	};

	/* Frame lifecycle — called internally by engine; exposed in case you embed */
	LEO_API void leo_InitMouse(void);
	LEO_API void leo_UpdateMouse(void);               /* call once per frame after polling events */
	LEO_API void leo_HandleMouseEvent(void* sdl_evt); /* engine forwards SDL_Event* here */
	LEO_API void leo_ShutdownMouse(void);

	/* Button queries (raylib parity) */
	LEO_API bool leo_IsMouseButtonPressed(int button);   /* pressed this frame */
	LEO_API bool leo_IsMouseButtonDown(int button);      /* currently down */
	LEO_API bool leo_IsMouseButtonReleased(int button);  /* released this frame */
	LEO_API bool leo_IsMouseButtonUp(int button);        /* currently up */

	/* Position & delta (in renderer coordinates: logical pixels if logical resolution is enabled) */
	LEO_API int         leo_GetMouseX(void);
	LEO_API int         leo_GetMouseY(void);
	LEO_API leo_Vector2 leo_GetMousePosition(void);
	LEO_API leo_Vector2 leo_GetMouseDelta(void);

	/* Set position (renderer coords), and optional offset/scale like raylib */
	LEO_API void leo_SetMousePosition(int x, int y);
	LEO_API void leo_SetMouseOffset(int offsetX, int offsetY);
	LEO_API void leo_SetMouseScale(float scaleX, float scaleY);

	/* Wheel (raylib parity) */
	LEO_API float       leo_GetMouseWheelMove(void);   /* max(|x|,|y|) with sign of the larger axis */
	LEO_API leo_Vector2 leo_GetMouseWheelMoveV(void);  /* {x, y} */

#ifdef __cplusplus
} /* extern "C" */
#endif
