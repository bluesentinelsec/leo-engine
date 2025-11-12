# Leo Lua API Cheat Sheet

Every Lua binding exposed by Leo today originates from `src/lua_bindings.c` and `src/lua_game.c`. All functions live in the global namespace (prefixed with `leo_`) once `leo_LuaOpenBindings` has run, and arguments follow the same order as their C counterparts. Return values use normal Lua conventions (`true`/`false`, `nil` for missing objects, lightuserdata for native pointers). Use this sheet as a quick reference while scripting.

## Window & Frame Control
- `leo_init_window(width, height, title?)` – Initialize the SDL window/renderer pair; returns `true` on success.
- `leo_close_window()` – Destroy the window and renderer when you are done.
- `leo_get_window()` / `leo_get_renderer()` – Access the underlying SDL window or renderer as lightuserdata (or `nil`).
- `leo_set_fullscreen(enabled)` – Toggle fullscreen mode; returns `true` if the mode change succeeded.
- `leo_set_window_mode(mode)` / `leo_get_window_mode()` – Switch between windowed/borderless/fullscreen modes and query the current one.
- `leo_window_should_close()` – `true` once the OS or user asked the app to quit.
- `leo_begin_drawing()` / `leo_end_drawing()` – Bracket every frame’s draw calls.
- `leo_clear_background(r, g, b[, a=255])` – Clear the current render target to a solid color.
- `leo_get_screen_width()` / `leo_get_screen_height()` – Physical backbuffer dimensions in pixels.
- `leo_set_logical_resolution(width, height, presentation, scale_mode)` – Configure logical scaling (letterbox/fit/etc.); returns `true` when scaling is active.

## Timing & Metrics
- `leo_set_target_fps(fps)` – Request a fixed frame cap.
- `leo_get_frame_time()` – Seconds elapsed during the most recent frame (delta time).
- `leo_get_time()` – Total runtime in seconds since Leo initialized.
- `leo_get_fps()` – Current frames per second reading.

## Immediate Drawing Helpers (Lua runtime)
- `leo_draw_pixel(x, y, r, g, b[, a=255])` – Plot a single pixel.
- `leo_draw_line(x1, y1, x2, y2, r, g, b[, a])` – Draw a colored line segment.
- `leo_draw_circle(x, y, radius, r, g, b[, a])` / `leo_draw_circle_filled(...)` – Outline or fill a circle.
- `leo_draw_rectangle(x, y, width, height, r, g, b[, a])` / `leo_draw_rectangle_lines(...)` – Fill a rectangle or draw its outline.
- `leo_draw_triangle(x1, y1, x2, y2, x3, y3, r, g, b[, a])` / `leo_draw_triangle_filled(...)` – Triangle primitives.

## Textures, Images & Render Targets
- `leo_load_render_texture(width, height)` / `leo_unload_render_texture(rt)` – Create or destroy an off-screen render target.
- `leo_begin_texture_mode(rt)` / `leo_end_texture_mode()` – Render into/off of a `leo_RenderTexture2D`.
- `leo_render_texture_get_texture(rt)` / `leo_render_texture_get_size(rt)` – Get the texture handle or `(width, height)` from a render texture.
- `leo_image_load(path)` / `leo_image_load_from_memory(ext, data)` / `leo_image_load_from_texture(texture)` / `leo_image_load_from_pixels(pixels, width, height[, pitch, format])` – Produce GPU textures from disk, memory, existing textures, or raw pixel buffers.
- `leo_image_is_ready(texture)` / `leo_image_unload(texture)` – Query readiness and free texture resources (owned ones only).
- `leo_image_bytes_per_pixel(format)` – Convenience helper for determining stride.
- `leo_texture_get_size(texture)` – Returns `(width, height)` for any texture userdata.
- `leo_draw_texture_rec(texture, srcX, srcY, srcW, srcH, destX, destY[, r, g, b, a])` – Draw a source sub-rectangle with optional tint.
- `leo_draw_texture_pro(texture, srcRect, destRect, originX, originY, rotation[, r, g, b, a])` – Full-featured textured quad draw (rotations, scaling, tint) using explicit rectangles.

## Fonts & Text
- `leo_load_font(path, pixel_size)` / `leo_load_font_from_memory([ext,] data, pixel_size)` / `leo_unload_font(font)` – Manage font atlases.
- `leo_is_font_ready(font)` – Check validity before drawing.
- `leo_set_default_font(font_or_nil)` / `leo_get_default_font()` – Configure or fetch the font used when one is not supplied.
- `leo_draw_text(text, x, y, font_size, r, g, b[, a])` – Quick bitmap text draw with the default font.
- `leo_draw_text_ex(font, text, x, y, font_size, spacing, r, g, b[, a])` – Draw with an explicit font, floating-point positions, and glyph spacing.
- `leo_draw_text_pro(font, text, x, y, originX, originY, rotation, font_size, spacing, r, g, b[, a])` – Advanced text draw supporting rotation/origin offsets.
- `leo_draw_fps(x, y)` – Debug FPS overlay at the chosen screen position.
- `leo_measure_text(text, font_size)` / `leo_measure_text_ex(font, text, font_size, spacing)` – Measure text width (and height for the `_ex` form).
- `leo_get_font_line_height(font, font_size)` / `leo_get_font_base_size(font)` – Line metrics for layouting.

## Cameras & Coordinate Helpers
- `leo_begin_mode2d(camera_table_or_numbers)` / `leo_end_mode2d()` – Push/pop a 2D camera; camera can be supplied as a table (`target_x`, `target_y`, `offset_x`, `offset_y`, `rotation`, `zoom`) or raw numbers.
- `leo_is_camera_active()` – `true` when a camera stack is active.
- `leo_get_world_to_screen2d(worldX, worldY[, camera])` / `leo_get_screen_to_world2d(screenX, screenY[, camera])` – Convert coordinates; if no camera is provided, the current one is used.
- `leo_get_current_camera2d()` – Returns the active camera as a Lua table.

## Input – Keyboard
- `leo_is_key_pressed(key)`, `leo_is_key_pressed_repeat(key)`, `leo_is_key_down(key)`, `leo_is_key_released(key)`, `leo_is_key_up(key)` – Standard instantaneous/down/up state queries for SDL keycodes (see exported `KEY_*` constants).
- `leo_get_key_pressed()` / `leo_get_char_pressed()` – Pull the next pressed keycode or UTF-32 codepoint.
- `leo_set_exit_key(key)` / `leo_is_exit_key_pressed()` – Configure which key triggers the default exit behavior and query if it fired.
- `leo_update_keyboard()` / `leo_cleanup_keyboard()` – Manual pump/cleanup helpers if you own the SDL loop.

## Input – Mouse (Lua runtime helpers)
- `leo_is_mouse_button_pressed(btn)`, `leo_is_mouse_button_down(btn)`, `leo_is_mouse_button_released(btn)`, `leo_is_mouse_button_up(btn)` – Button state tests; button values include `LEO_MOUSE_BUTTON_LEFT/MIDDLE/RIGHT/X1/X2`.
- `leo_get_mouse_x()` / `leo_get_mouse_y()` – Current cursor coordinates in screen pixels.
- `leo_get_mouse_position()` / `leo_get_mouse_delta()` – Returns `(x, y)` or `(dx, dy)` as two numbers.
- `leo_set_mouse_position(x, y)` / `leo_set_mouse_offset(offsetX, offsetY)` / `leo_set_mouse_scale(scaleX, scaleY)` – Control cursor position and scaling relative to the window.
- `leo_get_mouse_wheel_move()` / `leo_get_mouse_wheel_move_v()` – Vertical wheel delta or `(x, y)` scroll pair for the current frame.

## Input – Touch & Gestures
- `leo_is_touch_down(id)`, `leo_is_touch_pressed(id)`, `leo_is_touch_released(id)` – Per-touch contact states.
- `leo_get_touch_position(id)` / `leo_get_touch_x(id)` / `leo_get_touch_y(id)` – Access the coordinates of a touch point.
- `leo_get_touch_point_count()` / `leo_get_touch_point_id(index)` – Inspect how many touches are active and fetch their IDs.
- `leo_is_gesture_detected(mask)` / `leo_get_gesture_detected()` – Gesture flag checks.
- `leo_get_gesture_hold_duration()` / `leo_get_gesture_drag_vector()` / `leo_get_gesture_drag_angle()` – Continuous gesture information (duration, drag delta, drag angle).
- `leo_get_gesture_pinch_vector()` / `leo_get_gesture_pinch_angle()` – Pinch movement delta and angle.
- `leo_set_gestures_enabled(mask)` – Enable/disable gesture types via bitmask.

## Input – Gamepads
- `leo_init_gamepads()` / `leo_update_gamepads()` / `leo_shutdown_gamepads()` – Manage the global gamepad subsystem.
- `leo_handle_gamepad_event(lightuserdata_event)` – Feed SDL controller events delivered from your host loop.
- `leo_is_gamepad_available(index)` / `leo_get_gamepad_name(index)` – Detect connected controllers and read their friendly names.
- `leo_is_gamepad_button_pressed(index, button)`, `leo_is_gamepad_button_down(...)`, `leo_is_gamepad_button_released(...)`, `leo_is_gamepad_button_up(...)` – Button state helpers mirroring the keyboard API.
- `leo_get_gamepad_button_pressed()` – Last button that transitioned to pressed.
- `leo_get_gamepad_axis_count(index)` / `leo_get_gamepad_axis_movement(index, axis)` – Inspect analog axis availability/state.
- `leo_set_gamepad_vibration(index, left_motor, right_motor, duration_seconds)` – Fire rumble; returns `true` when supported.
- `leo_set_gamepad_axis_deadzone(deadzone)` – Clamp the global analog deadzone threshold.
- `leo_set_gamepad_stick_threshold(press_threshold, release_threshold)` – Configure hysteresis per stick direction events.
- `leo_get_gamepad_stick(index, stick)` – Returns `(x, y)` for either stick.
- `leo_is_gamepad_stick_pressed(index, stick, direction)`, `leo_is_gamepad_stick_down(...)`, `leo_is_gamepad_stick_released(...)`, `leo_is_gamepad_stick_up(...)` – Directional stick state helpers using `leo_GAMEPAD_STICK_*` and `leo_GAMEPAD_DIR_*` constants.

## Actor System
- `leo_actor_system_create()` / `leo_actor_system_destroy(system)` – Own the lifetime of a `leo_ActorSystem` from Lua.
- `leo_actor_system_update(system, dt)` / `leo_actor_system_render(system)` – Let the system tick and draw with your frame’s delta time.
- `leo_actor_system_set_paused(system, paused)` / `leo_actor_system_is_paused(system)` – Pause or query the whole system.
- `leo_actor_system_root(system)` – Fetch the invisible root actor (lightuserdata) if you need to walk the tree manually.
- `leo_actor_spawn(system, parent_actor, desc_table)` – Spawn actors under a parent using a descriptor table (`name`, `groups`, `start_paused`, `on_init/on_update/on_render/on_exit`, `user_ptr`, `user_value`, or a custom `vtable`). Returns the actor pointer or `nil` if creation failed.
- `leo_actor_kill(actor)` – Schedule an actor for destruction.
- `leo_actor_uid(actor)` – Retrieve the stable integer ID assigned at spawn.
- `leo_actor_name(actor)` / `leo_actor_parent(actor)` – Query metadata/relationships.
- `leo_actor_find_child_by_name(parent, name)` / `leo_actor_find_recursive_by_name(parent, name)` – Search descendants.
- `leo_actor_find_by_uid(system, uid)` – Locate an actor anywhere in the system via UID.
- `leo_actor_group_get_or_create(system, name)` / `leo_actor_group_find(system, name)` – Manage named group bits.
- `leo_actor_add_to_group(actor, bit)` / `leo_actor_remove_from_group(actor, bit)` / `leo_actor_in_group(actor, bit)` / `leo_actor_groups(actor)` – Maintain and inspect an actor’s group membership mask.
- `leo_actor_for_each_in_group(system, bit, callback(actor, user), user_data?)` – Iterate actors belonging to a specific group.
- `leo_actor_set_paused(actor, paused)` / `leo_actor_is_paused(actor)` / `leo_actor_is_effectively_paused(actor)` – Control or query per-actor pause state (effective includes parents).
- `leo_actor_emitter(actor)` / `leo_actor_emitter_const(actor)` – Access the actor’s optional emitter component as lightuserdata (mutable or const view).
- `leo_actor_userdata(actor)` / `leo_actor_set_userdata(actor, pointer_or_nil)` – Store/retrieve opaque pointers associated with Lua-created actors.
- `leo_actor_for_each_child(parent, callback(child, user), user_data?)` – Iterate all direct children of an actor.
- `leo_actor_set_z(actor, z)` / `leo_actor_get_z(actor)` – Update or inspect draw-order depth.

## JSON Utilities
- `leo_json_parse(json_text)` / `leo_json_load(path)` – Create a `leo_JsonDoc` from a string or file; returns `(doc)` or `(nil, error)`.
- `leo_json_free(doc)` – Destroy a JSON document once you are done.
- `leo_json_root(doc)` – Get the root `leo_JsonNode` handle.
- `leo_json_is_null(node)`, `leo_json_is_object(node)`, `leo_json_is_array(node)`, `leo_json_is_string(node)`, `leo_json_is_number(node)`, `leo_json_is_bool(node)` – Type predicates.
- `leo_json_obj_get(node, key)` – Fetch a child node from an object (always returns a node handle, even if `null`).
- `leo_json_arr_size(node)` / `leo_json_arr_get(node, index)` – Work with array nodes (1-based indexing in Lua).
- `leo_json_get_string(node, key)`, `leo_json_get_int(...)`, `leo_json_get_double(...)`, `leo_json_get_bool(...)` – Convenience getters returning `(success, value|nil)` pairs.
- `leo_json_as_string(node)`, `leo_json_as_int(node)`, `leo_json_as_double(node)`, `leo_json_as_bool(node)` – Convert a node directly into Lua primitives when you already know its type.

## Tiled Map Helpers
- `leo_tiled_load(path[, options_table])` / `leo_tiled_free(map)` – Load or free a `.tmx`/`.json` Tiled map; `options_table` supports `image_base` and `allow_compression`.
- `leo_tiled_map_get_size(map)` / `leo_tiled_map_get_tile_size(map)` – World and tile dimensions.
- `leo_tiled_map_get_metadata(map)` – Returns a table with `orientation` and `renderorder`.
- `leo_tiled_map_get_properties(map)` – Convert all custom map properties into a Lua table of `{ name, type, value }` entries.
- `leo_tiled_map_tile_layer_count(map)` / `leo_tiled_map_object_layer_count(map)` – Number of layers by type.
- `leo_tiled_map_get_tile_layer(map, index)` / `leo_tiled_map_get_object_layer(map, index)` – Fetch a layer wrapper (1-based index); returns `nil` if out of range.
- `leo_tiled_find_tile_layer(map, name)` / `leo_tiled_find_object_layer(map, name)` – Locate layers by name.
- `leo_tiled_tile_layer_get_name(layer)` / `leo_tiled_tile_layer_get_size(layer)` – Inspect tile layer metadata.
- `leo_tiled_get_gid(layer, x, y)` – Read the global tile ID at a tile coordinate.
- `leo_tiled_object_layer_get_name(layer)` / `leo_tiled_object_layer_get_count(layer)` / `leo_tiled_object_layer_get_object(layer, index)` – Iterate objects and read their exported structs.
- `leo_tiled_tileset_count(map)` / `leo_tiled_map_get_tileset(map, index)` – Work with loaded tilesets.
- `leo_tiled_gid_info(gid)` – Decode flip flags and raw IDs into a Lua table (`gid_raw`, `id`, `flip_h`, `flip_v`, `flip_d`).
- `leo_tiled_resolve_gid(map, gid)` – Returns `(true, tileset, src_rect_table)` when a GID maps to a tileset/texture rectangle; otherwise `(false)`.

## Collision Helpers
- `leo_check_collision_recs(ax, ay, aw, ah, bx, by, bw, bh)` – `true` when two rectangles overlap.
- `leo_check_collision_circles(x1, y1, r1, x2, y2, r2)` – Circle-circle collision.
- `leo_check_collision_circle_rec(cx, cy, radius, rx, ry, rw, rh)` – Circle vs. rectangle test.
- `leo_check_collision_circle_line(cx, cy, radius, x1, y1, x2, y2)` – Circle vs. line segment.
- `leo_check_collision_point_rec(px, py, rx, ry, rw, rh)` / `leo_check_collision_point_circle(px, py, cx, cy, radius)` / `leo_check_collision_point_triangle(px, py, ax, ay, bx, by, cx, cy)` – Point inclusion helpers.
- `leo_check_collision_point_line(px, py, x1, y1, x2, y2, threshold)` – Distance-from-line test in pixels.
- `leo_check_collision_lines(x1, y1, x2, y2, x3, y3, x4, y4)` – Returns `(hit, table_or_nil)` where the table holds the intersection point.
- `leo_get_collision_rec(ax, ay, aw, ah, bx, by, bw, bh)` – Returns a rectangle table describing the overlapping area (or zeros when there is none).

## Base64 Helpers
- `leo_base64_encoded_len(byte_count)` / `leo_base64_decoded_cap(byte_count)` – Predict buffer sizes before encoding/decoding.
- `leo_base64_encode(bytes)` – Returns a Base64 string or `(nil, reason)` on failure.
- `leo_base64_decode(b64_string)` – Returns the decoded binary blob (as a Lua string) or `(nil, reason)`.

## Animation
- `leo_load_animation(path, frame_w, frame_h, frame_count, frame_time, loop)` / `leo_unload_animation(animation)` – Manage sprite-sheet-driven animations.
- `leo_create_animation_player(animation)` – Create a stateful player userdata tied to a loaded animation.
- `leo_update_animation(player, dt)` – Advance the animation by `dt` seconds.
- `leo_draw_animation(player, x, y)` – Draw the animation at integer coordinates using its internal texture.
- `leo_play_animation(player)` / `leo_pause_animation(player)` / `leo_reset_animation(player)` – Control playback state.

## Audio
- `leo_init_audio()` / `leo_shutdown_audio()` – Initialize or tear down the audio mixer; `leo_init_audio` returns `true` on success.
- `leo_load_sound(path)` / `leo_unload_sound(sound)` – Load WAV/OGG/etc. into a `leo_Sound` userdata, and release it when finished.
- `leo_is_sound_ready(sound)` – Confirm that a sound handle is valid.
- `leo_play_sound(sound[, volume=1.0, loop=false])` – Start playback; returns `true` if it could start.
- `leo_stop_sound(sound)` / `leo_pause_sound(sound)` / `leo_resume_sound(sound)` – Immediate playback control.
- `leo_is_sound_playing(sound)` – `true` while the clip is active.
- `leo_set_sound_volume(sound, volume)` / `leo_set_sound_pitch(sound, pitch)` / `leo_set_sound_pan(sound, pan)` – Adjust playback properties on the fly.

## Screen Transitions & Fades
- `leo_start_fade_in(duration_seconds, color_or_{r,g,b,a})` – Fade from black/any color without invoking a callback.
- `leo_start_fade_out(duration_seconds, color_or_table[, on_complete_fn])` – Fade out to a color and optionally invoke `on_complete_fn()` once finished.
- `leo_start_transition(type, duration_seconds, color_or_table[, on_complete_fn])` – Run any registered transition by type ID (see your `leo_TransitionType` enum) with an optional callback.
- `leo_update_transitions(dt)` / `leo_render_transitions()` – Pump and draw the transition manager if you integrate it manually.
- `leo_is_transitioning()` – Query whether any fade/transition is currently active.

## Touch-Friendly Logical Resolution (already mentioned)
- `leo_set_logical_resolution(width, height, presentation, scale_mode)` – (See Window section) ensures pixel-perfect scaling for low-res games.

## Game Loop Helpers & Miscellaneous
- `leo_quit()` – Request the currently running Lua game loop (`leo_LuaGameRun`) to exit at the next safe point.

### Notes
- Mouse button constants (`LEO_MOUSE_BUTTON_LEFT`, etc.), keyboard constants (`KEY_A`, `KEY_SPACE`, …), and all gamepad constants (`leo_GAMEPAD_BUTTON_*`, `leo_GAMEPAD_AXIS_*`, `leo_GAMEPAD_STICK_*`, `leo_GAMEPAD_DIR_*`) are injected as global integers alongside these functions for convenience.
- Functions returning lightuserdata expose native handles; only store them if you understand the engine’s lifetime rules.
