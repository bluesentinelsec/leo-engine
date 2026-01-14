# Keyboard (Caller Experience)

This document describes the caller-facing keyboard API for Leo Engine. It is
inspired by Raylib's simple `IsKeyDown/Pressed/Released` queries while staying
compatible with Leo's deterministic input model.

## Goals

- Provide a small, predictable API for per-tick keyboard queries.
- Make queries deterministic and tied to `InputFrame` (not raw SDL events).
- Keep the API stable for input recording and playback.
- Stay explicit about edge-triggered vs. level-triggered input.

## API Surface (Proposed)

```cpp
namespace engine {

enum class Key : uint16_t {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Escape, Enter, Space, Tab, Backspace, Delete,
    Left, Right, Up, Down,
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
    Home, End, PageUp, PageDown,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count
};

struct KeyboardState {
    bool IsDown(Key key) const;
    bool IsPressed(Key key) const;
    bool IsReleased(Key key) const;
    bool IsUp(Key key) const;
};

struct InputFrame {
    KeyboardState keyboard;
    // ... other deterministic input fields (mouse, gamepad, etc.)
};

} // namespace engine
```

## Usage (Caller View)

```cpp
void OnUpdate(leo::Engine::Context &ctx, const leo::Engine::InputFrame &input)
{
    (void)ctx;

    if (input.keyboard.IsPressed(engine::Key::Space)) {
        // fire weapon
    }

    if (input.keyboard.IsDown(engine::Key::Left)) {
        // move left (held)
    }
}
```

## Semantics

- `IsDown`: key is currently held in this frame.
- `IsPressed`: key transitioned up -> down in this frame.
- `IsReleased`: key transitioned down -> up in this frame.
- `IsUp`: key is not held in this frame.

These queries are evaluated against the `InputFrame` passed into `OnUpdate`,
not against raw SDL events.

## Recording/Playback Plan

The keyboard API itself does not handle recording, but it is designed for it:

- `InputFrame` stays a POD-like container that can be serialized per tick.
- The recorded form stores a fixed-size key-down bitset.
- `IsPressed`/`IsReleased` are computed from the current and previous frames,
  so playback only needs the recorded down bitset to reproduce the same
  transitions deterministically.
- Key codes are stable and platform-independent (mapped from SDL scancodes).

Text input (for UI typing) is intentionally out of scope for this API and will
be exposed separately.
