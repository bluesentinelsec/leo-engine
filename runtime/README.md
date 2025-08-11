# API Design Notes (WIP / DRAFT)

API heavily inspired by rayib; will likely modify it along the way.

## **1. Window & Monitor Management**

```c
leo_err leo_init_window(int width, int height, const char * /*notnull*/ title);
void leo_close_window(void);
bool leo_window_should_close(void);

bool leo_is_window_fullscreen(void);
void leo_toggle_fullscreen(void);

int leo_get_screen_width(void);
int leo_get_screen_height(void);

int leo_get_monitor_count(void);
int leo_get_current_monitor(void);

leo_vector2 leo_get_monitor_position(int monitor);
leo_vector2 leo_get_window_position(void);

const char *leo_get_monitor_name(int monitor);
```

---

## **2. Cursor Control**

```c
void leo_show_cursor(void);
void leo_hide_cursor(void);
bool leo_is_cursor_hidden(void);

void leo_enable_cursor(void);   // unlock cursor
void leo_disable_cursor(void);  // lock cursor

bool leo_is_cursor_on_screen(void);
```

---

## **3. Rendering Control**

```c
void leo_clear_background(leo_color color);
void leo_begin_drawing(void);
void leo_end_drawing(void);

void leo_begin_mode_2d(leo_camera2d camera);
void leo_end_mode_2d(void);

void leo_begin_texture_mode(leo_render_texture2d target);
void leo_end_texture_mode(void);

void leo_begin_blend_mode(int mode);
void leo_end_blend_mode(void);
```

---

## **4. Screen-Space & Camera Math**

```c
leo_ray leo_get_screen_to_world_ray(leo_vector2 position, leo_camera camera);
leo_ray leo_get_screen_to_world_ray_ex(leo_vector2 position, leo_camera camera, int width, int height);

leo_vector2 leo_get_world_to_screen(leo_vector3 position, leo_camera camera);
leo_vector2 leo_get_world_to_screen_ex(leo_vector3 position, leo_camera camera, int width, int height);

leo_vector2 leo_get_world_to_screen_2d(leo_vector2 position, leo_camera2d camera);
leo_vector2 leo_get_screen_to_world_2d(leo_vector2 position, leo_camera2d camera);

leo_matrix leo_get_camera_matrix(leo_camera camera);
leo_matrix leo_get_camera_matrix_2d(leo_camera2d camera);
```

---

## **5. Timing & FPS**

```c
void leo_set_target_fps(int fps);
float leo_get_frame_time(void);  // delta time in seconds
double leo_get_time(void);       // time since init
int leo_get_fps(void);
```

---

## **6. Random Numbers**

```c
void leo_set_random_seed(unsigned int seed);
int leo_get_random_value(int min, int max);
```

---

## **7. Logging & Memory**

```c
void leo_trace_log(int log_level, const char * /*notnull*/ text, ...);
void leo_set_trace_log_level(int log_level);
void leo_set_trace_log_callback(leo_trace_log_callback callback);

void *leo_mem_alloc(unsigned int size);
void *leo_mem_realloc(void *ptr, unsigned int size);
void leo_mem_free(void *ptr);
```

---

## **8. File I/O**

```c
unsigned char *leo_load_file_data(const char * /*notnull*/ file_name, int * /*notnull*/ data_size);
void leo_unload_file_data(unsigned char * /*notnull*/ data);

bool leo_save_file_data(const char * /*notnull*/ file_name, void * /*notnull*/ data, int data_size);

char *leo_load_file_text(const char * /*notnull*/ file_name);
void leo_unload_file_text(char * /*notnull*/ text);
bool leo_save_file_text(const char * /*notnull*/ file_name, char * /*notnull*/ text);
```

---

## **9. File System**

```c
bool leo_file_exists(const char * /*notnull*/ file_name);
bool leo_directory_exists(const char * /*notnull*/ dir_path);
bool leo_is_file_extension(const char * /*notnull*/ file_name, const char * /*notnull*/ ext);

int leo_get_file_length(const char * /*notnull*/ file_name);
const char *leo_get_file_extension(const char * /*notnull*/ file_name);
const char *leo_get_file_name(const char * /*notnull*/ file_path);
const char *leo_get_file_name_without_ext(const char * /*notnull*/ file_path);

const char *leo_get_directory_path(const char * /*notnull*/ file_path);
const char *leo_get_prev_directory_path(const char * /*notnull*/ dir_path);
const char *leo_get_working_directory(void);
const char *leo_get_application_directory(void);

int leo_make_directory(const char * /*notnull*/ dir_path);
bool leo_change_directory(const char * /*notnull*/ dir);
bool leo_is_path_file(const char * /*notnull*/ path);
bool leo_is_file_name_valid(const char * /*notnull*/ file_name);
```

---

## **10. Compression & Encoding**

```c
unsigned char *leo_compress_data(const unsigned char * /*notnull*/ data, int data_size, int * /*notnull*/ comp_data_size);
unsigned char *leo_decompress_data(const unsigned char * /*notnull*/ comp_data, int comp_data_size, int * /*notnull*/ data_size);

char *leo_encode_data_base64(const unsigned char * /*notnull*/ data, int data_size, int * /*notnull*/ output_size);
unsigned char *leo_decode_data_base64(const unsigned char * /*notnull*/ data, int * /*notnull*/ output_size);

unsigned int leo_compute_crc32(unsigned char * /*notnull*/ data, int data_size);
unsigned int *leo_compute_md5(unsigned char * /*notnull*/ data, int data_size);
unsigned int *leo_compute_sha1(unsigned char * /*notnull*/ data, int data_size);
```

---

## **11. Input – Keyboard**

```c
bool leo_is_key_pressed(int key);
bool leo_is_key_pressed_repeat(int key);
bool leo_is_key_down(int key);
bool leo_is_key_released(int key);
bool leo_is_key_up(int key);

int leo_get_key_pressed(void);
int leo_get_char_pressed(void);

void leo_set_exit_key(int key);
```

---

## **12. Input – Gamepad**

```c
bool leo_is_gamepad_available(int gamepad);
const char *leo_get_gamepad_name(int gamepad);

bool leo_is_gamepad_button_pressed(int gamepad, int button);
bool leo_is_gamepad_button_down(int gamepad, int button);
bool leo_is_gamepad_button_released(int gamepad, int button);
bool leo_is_gamepad_button_up(int gamepad, int button);

int leo_get_gamepad_button_pressed(void);
int leo_get_gamepad_axis_count(int gamepad);
float leo_get_gamepad_axis_movement(int gamepad, int axis);

int leo_set_gamepad_mappings(const char * /*notnull*/ mappings);
void leo_set_gamepad_vibration(int gamepad, float left_motor, float right_motor, float duration);
```

---

## **13. Input – Mouse**

```c
bool leo_is_mouse_button_pressed(int button);
bool leo_is_mouse_button_down(int button);
bool leo_is_mouse_button_released(int button);
bool leo_is_mouse_button_up(int button);

int leo_get_mouse_x(void);
int leo_get_mouse_y(void);
leo_vector2 leo_get_mouse_position(void);
leo_vector2 leo_get_mouse_delta(void);

void leo_set_mouse_position(int x, int y);
void leo_set_mouse_offset(int offset_x, int offset_y);
void leo_set_mouse_scale(float scale_x, float scale_y);

float leo_get_mouse_wheel_move(void);
leo_vector2 leo_get_mouse_wheel_move_v(void);

void leo_set_mouse_cursor(int cursor);
```

---

## **14. Shapes**

```c
void leo_set_shapes_texture(leo_texture2d texture, leo_rectangle source);
leo_texture2d leo_get_shapes_texture(void);
leo_rectangle leo_get_shapes_texture_rectangle(void);

// Examples – all shape functions would follow same leo_ prefix style
void leo_draw_pixel(int x, int y, leo_color color);
void leo_draw_line(int x1, int y1, int x2, int y2, leo_color color);
void leo_draw_circle(int x, int y, float radius, leo_color color);
void leo_draw_rectangle(int x, int y, int width, int height, leo_color color);
```

---

## **15. Collision**

```c
bool leo_check_collision_recs(leo_rectangle r1, leo_rectangle r2);
bool leo_check_collision_circles(leo_vector2 c1, float r1, leo_vector2 c2, float r2);
bool leo_check_collision_circle_rec(leo_vector2 center, float radius, leo_rectangle rec);
```

---


