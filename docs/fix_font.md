# Font Debugging Prompt

## Context

We added a new font/text system using stb_truetype with a VFS-based `Font` +
`Text` API and an FPS overlay in the engine loop. Fonts load, and the label
"FPS" appears, but the numeric count still does not render.

## What We Observed

- In `make run`, the string "FPS" is visible but the numeric portion is not.
- The FPS text updates via `Text::SetString` every ~1 second.
- We suspected SDL3 draw behavior when a glyph quad has zero width/height
  (spaces can yield zero-sized quads).

## What We Already Tried

- Filtered zero-sized quads in layout to avoid drawing spaces with `w=0/h=0`.
  This logic is in `Text::RebuildLayout()` in `src/font.cpp`.
- The issue persists after that change.

## Files to Inspect

- `src/font.cpp` (layout and draw logic)
- `include/leo/font.h` (API / ownership assumptions)
- `src/engine_core.cpp` (FPS text update and draw)
- `resources/font/font.ttf` (font asset)
- `old/src/font.c` (known working implementation)

## Hypotheses to Validate

1. **Layout math is wrong after spaces**  
   Compare `stbtt_GetBakedQuad` usage in `old/src/font.c` vs `src/font.cpp`.
   The old code uses absolute positions and applies scaling relative to `x,y`.
   We currently use `quad.x0 * scale` etc, which may cause glyph placement
   errors after advances.

2. **Renderer is refusing draws after zero-size source rects**  
   We now skip zero-size destination quads, but the source rect may still be
   zero or invalid for some glyphs (space or punctuation). Verify `src_quads`
   values for "FPS: 60".

3. **Text update is not rebuilding correctly**  
   Confirm that `Text::SetString` triggers `RebuildLayout()` for the new
   string and that `quad_count` is non-zero for digits.

4. **Font atlas or glyph data corruption**  
   Confirm atlas pixel format (currently `SDL_PIXELFORMAT_RGBA32`) and glyph
   data ownership. The old code used `SDL_PIXELFORMAT_ABGR8888` with nearest
   scaling.

5. **Text color modulation**  
   Ensure the atlas tint reset doesn’t interfere with digits. The code resets
   after each draw, but if the texture state is shared, check render order.

## Suggested Debug Steps

1. Add temporary logging to `Text::RebuildLayout()`:
   - Print `quad_count` after layout.
   - For each glyph, print `ch`, `src_quads[i]`, and `dst_quads[i]`.
   - Specifically check digits '0'–'9' after the space.

2. Compare layout math to old implementation:
   - The old code uses:
     ```
     float dx0 = (q.x0 - x) * scale + x;
     float dx1 = (q.x1 - x) * scale + x;
     ```
     We currently use:
     ```
     float dx0 = q.x0 * scale;
     float dx1 = q.x1 * scale;
     ```
   - Try adopting the old scaling math (relative to origin) and see if digits
     show up.

3. Test a string with no spaces like "FPS:60" and see if digits appear.

4. Try using `SDL_SCALEMODE_NEAREST` and `SDL_PIXELFORMAT_ABGR8888` to match
   old behavior.

5. Re-run `make run` and validate on screen.

## Current Code Locations

- Layout: `src/font.cpp` in `Text::RebuildLayout()`
- Draw: `src/font.cpp` in `Text::Draw()`
- FPS update: `src/engine_core.cpp` in `Simulation::OnRender()`

## Expected Outcome

Digits render next to "FPS:" and update once per second.
