# Gamepad (Caller Experience)

This document describes the caller-facing gamepad API. It mirrors the
Raylib-style "query per tick" approach and supports axis-to-button helpers.

## Goals

- Provide a simple, deterministic API via `InputFrame`.
- Support up to two gamepads.
- Make analog axes easy to treat as digital buttons.
- Handle hot-plugging gracefully.

## API Surface (Proposed/Implemented)

```cpp
namespace engine {

enum class GamepadButton : Uint8 {
    Unknown = 0,
    South, East, West, North,
    Back, Guide, Start,
    LeftStick, RightStick,
    LeftShoulder, RightShoulder,
    DpadUp, DpadDown, DpadLeft, DpadRight,
    Misc1, Touchpad,
    Count
};

enum class GamepadAxis : Uint8 {
    LeftX, LeftY,
    RightX, RightY,
    LeftTrigger, RightTrigger,
    Count
};

enum class AxisDirection : int8_t {
    Negative = -1,
    Positive = 1
};

class GamepadState {
  public:
    bool IsConnected() const noexcept;

    bool IsButtonDown(GamepadButton button) const noexcept;
    bool IsButtonPressed(GamepadButton button) const noexcept;
    bool IsButtonReleased(GamepadButton button) const noexcept;
    bool IsButtonUp(GamepadButton button) const noexcept;

    float GetAxis(GamepadAxis axis) const noexcept; // -1..1 (sticks), 0..1 (triggers)

    // Axis-as-button helpers
    bool IsAxisDown(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept;
    bool IsAxisPressed(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept;
    bool IsAxisReleased(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept;
};

struct InputFrame {
    GamepadState gamepads[2];
    // ...
};

} // namespace engine
```

## Usage (Caller View)

```cpp
void OnUpdate(leo::Engine::Context &ctx, const leo::Engine::InputFrame &input)
{
    (void)ctx;

    const engine::GamepadState &pad0 = input.gamepads[0];
    if (pad0.IsConnected() && pad0.IsButtonPressed(engine::GamepadButton::South)) {
        // Jump
    }

    if (pad0.IsAxisDown(engine::GamepadAxis::LeftX, 0.35f, engine::AxisDirection::Negative)) {
        // Move left using the stick
    }
}
```

## Semantics

- Button queries are edge-triggered (`Pressed`/`Released`) or level-triggered (`Down`/`Up`).
- Axis values are normalized floats:
  - Sticks: `-1..1`
  - Triggers: `0..1`
- Axis-as-button helpers treat the axis as a virtual button when the value
  crosses the `threshold` in the given `direction`.

## Hot-Plugging

The engine listens for SDL gamepad add/remove events and keeps the two slots
up to date. If more than two controllers are connected, extra devices are
ignored.
