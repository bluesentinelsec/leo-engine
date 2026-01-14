# Mouse (Caller Experience)

This document describes the caller-facing mouse API. It follows the same
per-tick query style as keyboard and gamepad input.

## Goals

- Provide simple button and motion queries per tick.
- Track both position and delta for deterministic gameplay.
- Expose wheel movement per tick.

## API Surface (Proposed/Implemented)

```cpp
namespace engine {

enum class MouseButton : Uint8 {
    Unknown = 0,
    Left,
    Middle,
    Right,
    X1,
    X2,
    Count
};

class MouseState {
  public:
    bool IsButtonDown(MouseButton button) const noexcept;
    bool IsButtonPressed(MouseButton button) const noexcept;
    bool IsButtonReleased(MouseButton button) const noexcept;
    bool IsButtonUp(MouseButton button) const noexcept;

    float GetX() const noexcept;
    float GetY() const noexcept;
    float GetDeltaX() const noexcept;
    float GetDeltaY() const noexcept;
    float GetWheelX() const noexcept;
    float GetWheelY() const noexcept;
};

struct InputFrame {
    MouseState mouse;
    // ...
};

} // namespace engine
```

## Usage (Caller View)

```cpp
void OnUpdate(leo::Engine::Context &ctx, const leo::Engine::InputFrame &input)
{
    (void)ctx;

    if (input.mouse.IsButtonPressed(engine::MouseButton::Left)) {
        // fire or select
    }

    float dx = input.mouse.GetDeltaX();
    float dy = input.mouse.GetDeltaY();
    (void)dx;
    (void)dy;
}
```

## Semantics

- `IsButtonPressed`/`IsButtonReleased` are edge-triggered for the current tick.
- `GetDeltaX`/`GetDeltaY` are per-tick accumulated motion deltas.
- `GetWheelX`/`GetWheelY` report per-tick wheel movement (normalized to match
  SDL's "non-flipped" convention).
