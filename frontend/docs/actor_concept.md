# What works well

* **Mental model** stays simple: “things in a scene update and render.”
* **Performance** is fine for 2D if you add a spatial index for collisions (grid/quadtree/BVH).
* **Flexibility**: different genres without over-engineering components/systems.

# Guardrails so it doesn’t devolve into spaghetti

1. **Strict lifecycles**

   * Actor: `init → update(dt) → render() → destroy`.
   * Scene: `on_enter(args) → update(dt) → render() → on_exit()`.
   * No cross-lifecycle surprises (e.g., don’t spawn while iterating—queue it).

2. **Deterministic update order**

   * Fixed timestep sim (accumulator), variable render.
   * Update order: input → apply queued spawns/despawns → physics → collisions → actor `update` → scene `post_update` hooks.
   * Render after all updates.

3. **Ownership & lifetime**

   * Scene owns actors; actors never outlive the scene.
   * `spawn()`/`despawn()` go into queues; applied at safe points.
   * References use **ids/handles**, not raw pointers.

4. **Signals/events (loose coupling)**

   * Engine bus with named signals: `emit("player_died", data)`.
   * Actors subscribe/unsubscribe in `init/destroy`.
   * Keep payloads small (ids/ints/structs), not heavy objects.

5. **Collisions**

   * Broad phase: uniform grid (start simple) → quadtree if needed.
   * Actor opts into collision layer/mask + AABB or circle.
   * Engine resolves pairs, calls `on_collision(self, other, info)`.

6. **Data-driven prefabs**

   * Prefab = Lua table/module with defaults + hooks.
   * Scene loads prefab instances from JSON/Tiled (args merged with defaults).
   * Lets designers place “what” without wiring code every time.

7. **Hot reload**

   * On Lua change: clear module, re-require, swap vtable where safe.
   * Keep actor state in plain tables so swapping behavior is trivial.

8. **Save/Load**

   * Actor provides `serialize()`/`deserialize(data)`.
   * Scene saves list of prefab ids + args + minimal runtime state.

# Minimal shapes (C + Lua sketch)

**C (handles + queues)**

```c
typedef uint32_t leo_actor_id;

typedef struct {
  // required hooks exposed to Lua
  void (*init)(leo_actor_id);
  void (*update)(leo_actor_id, float dt);
  void (*render)(leo_actor_id);
  void (*destroy)(leo_actor_id);
  void (*on_signal)(leo_actor_id, const char* name, uintptr_t data);
} leo_actor_vtbl;

void leo_spawn_queued(const char* prefab, const char* scene, const char* json_args);
void leo_despawn_queued(leo_actor_id id);
void leo_emit_signal(const char* name, uintptr_t data);
```

**Lua (prefab)**

```lua
local P = {}

function P.defaults() return {tex="player.png", x=0, y=0, speed=80} end

function P.init(self, a)
  self.tex = leo.image.load(a.tex)
  self.x, self.y, self.speed = a.x, a.y, a.speed
end

function P.update(self, dt)
  if leo.key.down("right") then self.x = self.x + self.speed * dt end
end

function P.render(self) leo.draw.sprite(self.tex, self.x, self.y) end
function P.destroy(self) leo.image.unload(self.tex) end

return P
```

**Lua (scene)**

```lua
local S = {}

function S.on_enter(self, args)
  self.bg = leo.image.load("bg.png")
  self.player = leo.spawn("player", self, {x=16, y=16})
end

function S.update(self, dt)
  if leo.key.pressed("escape") then leo.scene.pop() end
end

function S.render(self)
  leo.draw.image(self.bg, 0, 0)
end

function S.on_exit(self) leo.image.unload(self.bg) end

return S
```

# Practical tips

* **Input**: poll once per frame; expose queries to Lua (pressed/once/released).
* **Audio**: queue play/stop requests; engine mixes.
* **Ordering**: a per-actor `z` (or layers) to sort render without coupling.
* **Perf**: profile before optimizing; grids + pooling go a long way.
* **Testing**: headless mode that runs fixed ticks and asserts state (great for CI).

# When you might want “a bit of components”

If you notice repeated patterns (health, physics, sprite), add **tiny mixins** (Lua helpers) rather than a full ECS. Keep Actors as the unit of composition; let mixins add fields + functions.

---

