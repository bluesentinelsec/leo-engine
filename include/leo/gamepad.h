#ifndef LEO_GAMEPAD_H
#define LEO_GAMEPAD_H

#include <SDL3/SDL_stdinc.h>
#include <array>

namespace engine
{

enum class GamepadButton : Uint8
{
    Unknown = 0,
    South,
    East,
    West,
    North,
    Back,
    Guide,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    Misc1,
    Touchpad,
    Count
};

enum class GamepadAxis : Uint8
{
    LeftX = 0,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger,
    Count
};

enum class AxisDirection : int8_t
{
    Negative = -1,
    Positive = 1
};

class GamepadState
{
  public:
    bool IsConnected() const noexcept
    {
        return connected;
    }

    bool IsButtonDown(GamepadButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_down[Index(button)] != 0;
    }

    bool IsButtonPressed(GamepadButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_pressed[Index(button)] != 0;
    }

    bool IsButtonReleased(GamepadButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_released[Index(button)] != 0;
    }

    bool IsButtonUp(GamepadButton button) const noexcept
    {
        return !IsButtonDown(button);
    }

    float GetAxis(GamepadAxis axis) const noexcept
    {
        if (!IsValidAxis(axis))
        {
            return 0.0f;
        }
        return axes[Index(axis)];
    }

    bool IsAxisDown(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept
    {
        return AxisValue(GetAxis(axis), direction) >= ClampThreshold(threshold);
    }

    bool IsAxisPressed(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept
    {
        float t = ClampThreshold(threshold);
        float now = AxisValue(GetAxis(axis), direction);
        float prev = AxisValue(GetAxisPrevious(axis), direction);
        return now >= t && prev < t;
    }

    bool IsAxisReleased(GamepadAxis axis, float threshold, AxisDirection direction) const noexcept
    {
        float t = ClampThreshold(threshold);
        float now = AxisValue(GetAxis(axis), direction);
        float prev = AxisValue(GetAxisPrevious(axis), direction);
        return now < t && prev >= t;
    }

    // Internal: reset state, including connection flag.
    void Reset() noexcept
    {
        connected = false;
        buttons_down.fill(0);
        buttons_pressed.fill(0);
        buttons_released.fill(0);
        axes.fill(0.0f);
        axes_prev.fill(0.0f);
    }

    // Internal: clear transient transitions and snapshot previous axis values.
    void BeginFrame() noexcept
    {
        buttons_pressed.fill(0);
        buttons_released.fill(0);
        axes_prev = axes;
    }

    // Internal: set whether this slot is active.
    void SetConnected(bool is_connected) noexcept
    {
        connected = is_connected;
    }

    // Internal: record a button down transition.
    void SetButtonDown(GamepadButton button) noexcept
    {
        if (!IsValidButton(button))
        {
            return;
        }
        size_t index = Index(button);
        if (buttons_down[index])
        {
            return;
        }
        buttons_down[index] = 1;
        buttons_pressed[index] = 1;
    }

    // Internal: record a button up transition.
    void SetButtonUp(GamepadButton button) noexcept
    {
        if (!IsValidButton(button))
        {
            return;
        }
        size_t index = Index(button);
        if (!buttons_down[index])
        {
            return;
        }
        buttons_down[index] = 0;
        buttons_released[index] = 1;
    }

    // Internal: update axis value (normalized to -1..1 or 0..1 for triggers).
    void SetAxis(GamepadAxis axis, float value) noexcept
    {
        if (!IsValidAxis(axis))
        {
            return;
        }
        axes[Index(axis)] = ClampAxis(axis, value);
    }

  private:
    static constexpr size_t Index(GamepadButton button) noexcept
    {
        return static_cast<size_t>(button);
    }

    static constexpr size_t Index(GamepadAxis axis) noexcept
    {
        return static_cast<size_t>(axis);
    }

    static constexpr bool IsValidButton(GamepadButton button) noexcept
    {
        return button > GamepadButton::Unknown && button < GamepadButton::Count;
    }

    static constexpr bool IsValidAxis(GamepadAxis axis) noexcept
    {
        return axis < GamepadAxis::Count;
    }

    static float ClampAxis(GamepadAxis axis, float value) noexcept
    {
        if (axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger)
        {
            if (value < 0.0f)
            {
                return 0.0f;
            }
            if (value > 1.0f)
            {
                return 1.0f;
            }
            return value;
        }
        if (value < -1.0f)
        {
            return -1.0f;
        }
        if (value > 1.0f)
        {
            return 1.0f;
        }
        return value;
    }

    float GetAxisPrevious(GamepadAxis axis) const noexcept
    {
        if (!IsValidAxis(axis))
        {
            return 0.0f;
        }
        return axes_prev[Index(axis)];
    }

    static float AxisValue(float value, AxisDirection direction) noexcept
    {
        return (direction == AxisDirection::Negative) ? -value : value;
    }

    static float ClampThreshold(float threshold) noexcept
    {
        if (threshold < 0.0f)
        {
            return 0.0f;
        }
        if (threshold > 1.0f)
        {
            return 1.0f;
        }
        return threshold;
    }

    bool connected = false;
    std::array<Uint8, static_cast<size_t>(GamepadButton::Count)> buttons_down = {};
    std::array<Uint8, static_cast<size_t>(GamepadButton::Count)> buttons_pressed = {};
    std::array<Uint8, static_cast<size_t>(GamepadButton::Count)> buttons_released = {};
    std::array<float, static_cast<size_t>(GamepadAxis::Count)> axes = {};
    std::array<float, static_cast<size_t>(GamepadAxis::Count)> axes_prev = {};
};

} // namespace engine

#endif // LEO_GAMEPAD_H
