#ifndef LEO_KEYBOARD_H
#define LEO_KEYBOARD_H

#include <SDL3/SDL_stdinc.h>
#include <array>

namespace engine
{

enum class Key : Uint16
{
    Unknown = 0,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Escape,
    Enter,
    Space,
    Tab,
    Backspace,
    Delete,
    Left,
    Right,
    Up,
    Down,
    LShift,
    RShift,
    LCtrl,
    RCtrl,
    LAlt,
    RAlt,
    Home,
    End,
    PageUp,
    PageDown,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Count
};

class KeyboardState
{
  public:
    bool IsDown(Key key) const noexcept
    {
        if (!IsValidKey(key))
        {
            return false;
        }
        return down[Index(key)] != 0;
    }

    bool IsPressed(Key key) const noexcept
    {
        if (!IsValidKey(key))
        {
            return false;
        }
        return pressed[Index(key)] != 0;
    }

    bool IsReleased(Key key) const noexcept
    {
        if (!IsValidKey(key))
        {
            return false;
        }
        return released[Index(key)] != 0;
    }

    bool IsUp(Key key) const noexcept
    {
        return !IsDown(key);
    }

    // Internal: reset all states.
    void Reset() noexcept
    {
        down.fill(0);
        pressed.fill(0);
        released.fill(0);
    }

    // Internal: clear transient presses/releases for a new frame.
    void BeginFrame() noexcept
    {
        pressed.fill(0);
        released.fill(0);
    }

    // Internal: apply a key down transition.
    void SetKeyDown(Key key) noexcept
    {
        if (!IsValidKey(key))
        {
            return;
        }
        size_t index = Index(key);
        if (down[index])
        {
            return;
        }
        down[index] = 1;
        pressed[index] = 1;
    }

    // Internal: apply a key up transition.
    void SetKeyUp(Key key) noexcept
    {
        if (!IsValidKey(key))
        {
            return;
        }
        size_t index = Index(key);
        if (!down[index])
        {
            return;
        }
        down[index] = 0;
        released[index] = 1;
    }

  private:
    static constexpr size_t Index(Key key) noexcept
    {
        return static_cast<size_t>(key);
    }

    static constexpr bool IsValidKey(Key key) noexcept
    {
        return key > Key::Unknown && key < Key::Count;
    }

    std::array<Uint8, static_cast<size_t>(Key::Count)> down = {};
    std::array<Uint8, static_cast<size_t>(Key::Count)> pressed = {};
    std::array<Uint8, static_cast<size_t>(Key::Count)> released = {};
};

} // namespace engine

#endif // LEO_KEYBOARD_H
