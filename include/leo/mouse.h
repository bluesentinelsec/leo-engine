#ifndef LEO_MOUSE_H
#define LEO_MOUSE_H

#include <SDL3/SDL_stdinc.h>
#include <array>

namespace engine
{

enum class MouseButton : Uint8
{
    Unknown = 0,
    Left,
    Middle,
    Right,
    X1,
    X2,
    Count
};

class MouseState
{
  public:
    bool IsButtonDown(MouseButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_down[Index(button)] != 0;
    }

    bool IsButtonPressed(MouseButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_pressed[Index(button)] != 0;
    }

    bool IsButtonReleased(MouseButton button) const noexcept
    {
        if (!IsValidButton(button))
        {
            return false;
        }
        return buttons_released[Index(button)] != 0;
    }

    bool IsButtonUp(MouseButton button) const noexcept
    {
        return !IsButtonDown(button);
    }

    float GetX() const noexcept
    {
        return x;
    }

    float GetY() const noexcept
    {
        return y;
    }

    float GetDeltaX() const noexcept
    {
        return delta_x;
    }

    float GetDeltaY() const noexcept
    {
        return delta_y;
    }

    float GetWheelX() const noexcept
    {
        return wheel_x;
    }

    float GetWheelY() const noexcept
    {
        return wheel_y;
    }

    // Internal: reset the state.
    void Reset() noexcept
    {
        buttons_down.fill(0);
        buttons_pressed.fill(0);
        buttons_released.fill(0);
        x = 0.0f;
        y = 0.0f;
        delta_x = 0.0f;
        delta_y = 0.0f;
        wheel_x = 0.0f;
        wheel_y = 0.0f;
    }

    // Internal: reset transient values for a new frame.
    void BeginFrame() noexcept
    {
        buttons_pressed.fill(0);
        buttons_released.fill(0);
        delta_x = 0.0f;
        delta_y = 0.0f;
        wheel_x = 0.0f;
        wheel_y = 0.0f;
    }

    // Internal: update button state transitions.
    void SetButtonDown(MouseButton button) noexcept
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

    void SetButtonUp(MouseButton button) noexcept
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

    // Internal: update pointer location and deltas.
    void SetPosition(float new_x, float new_y) noexcept
    {
        x = new_x;
        y = new_y;
    }

    void AddDelta(float dx, float dy) noexcept
    {
        delta_x += dx;
        delta_y += dy;
    }

    void AddWheel(float wx, float wy) noexcept
    {
        wheel_x += wx;
        wheel_y += wy;
    }

  private:
    static constexpr size_t Index(MouseButton button) noexcept
    {
        return static_cast<size_t>(button);
    }

    static constexpr bool IsValidButton(MouseButton button) noexcept
    {
        return button > MouseButton::Unknown && button < MouseButton::Count;
    }

    std::array<Uint8, static_cast<size_t>(MouseButton::Count)> buttons_down = {};
    std::array<Uint8, static_cast<size_t>(MouseButton::Count)> buttons_pressed = {};
    std::array<Uint8, static_cast<size_t>(MouseButton::Count)> buttons_released = {};
    float x = 0.0f;
    float y = 0.0f;
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    float wheel_x = 0.0f;
    float wheel_y = 0.0f;
};

} // namespace engine

#endif // LEO_MOUSE_H
