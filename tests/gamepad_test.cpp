#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "leo/gamepad.h"
#include "leo/engine.h"

using Catch::Approx;

// Test backend harness
struct TestPad
{
	int idx = -1;

	void attach(const char* name = "Leo Test Pad")
	{
		idx = leo__test_attach_pad(name);
		REQUIRE(idx >= 0);
	}

	void detach()
	{
		if (idx >= 0)
		{
			leo__test_detach_pad(idx);
			idx = -1;
		}
	}

	void setButton(int b, bool down) { leo__test_set_button(idx, b, down); }
	void setAxis(int a, float v) { leo__test_set_axis(idx, a, v); }
};

// Small tick helper
static void tick_frame() { leo_UpdateGamepads(); }

// Each test starts with a clean Leo state
struct LeoGamepadEnv
{
	LeoGamepadEnv() { leo_InitGamepads(); }
	~LeoGamepadEnv() { leo_ShutdownGamepads(); }
};

TEST_CASE("Gamepad connects and exposes name", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	tick_frame();
	REQUIRE(leo_IsGamepadAvailable(0));
	auto* name = leo_GetGamepadName(0);
	if (name)
		REQUIRE(name[0] != '\0');
	tp.detach();
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadAvailable(0));
}

TEST_CASE("Button edges: Pressed/Down/Released/Up", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	const int B = LEO_GAMEPAD_BUTTON_FACE_DOWN;

	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadButtonDown(0, B));
	REQUIRE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	tp.setButton(B, true);
	tick_frame();
	REQUIRE(leo_IsGamepadButtonDown(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonUp(0, B));
	REQUIRE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	tick_frame();
	REQUIRE(leo_IsGamepadButtonDown(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	tp.setButton(B, false);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadButtonDown(0, B));
	REQUIRE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE(leo_IsGamepadButtonReleased(0, B));
}

TEST_CASE("Axis normalization and deadzone", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();

	leo_SetGamepadAxisDeadzone(0.20f);

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.15f);
	tick_frame();
	REQUIRE(std::fabs(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X) - 0.0f) < 1e-6f);

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.60f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X)).margin(0.02f) == 0.60f);

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, -0.75f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X)).margin(0.02f) == -0.75f);
}

TEST_CASE("Stick virtual buttons with hysteresis (RIGHT dir)", "[gamepad][stick]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();

	leo_SetGamepadStickThreshold(0.60f, 0.40f);
	leo_SetGamepadAxisDeadzone(0.10f);

	const int ST = LEO_GAMEPAD_STICK_LEFT;
	const int DIR = LEO_GAMEPAD_DIR_RIGHT;

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.0f);
	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_Y, 0.0f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickUp(0, ST, DIR));

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.70f);
	tick_frame();
	REQUIRE(leo_IsGamepadStickPressed(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE_FALSE(leo_IsGamepadStickReleased(0, ST, DIR));

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.50f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickPressed(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE_FALSE(leo_IsGamepadStickReleased(0, ST, DIR));

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_X, 0.30f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickUp(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickReleased(0, ST, DIR));
}

TEST_CASE("Triggers clamped to [0,+1] by public API", "[gamepad][triggers]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();

	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_TRIGGER, -0.5f);
	tick_frame();
	REQUIRE(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_TRIGGER) >= 0.0f);

	tp.setAxis(LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, 1.0f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_RIGHT_TRIGGER)).margin(0.01f) == 1.0f);
}

TEST_CASE("Last button pressed (per-frame) reports lowest first", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();

	tp.setButton(LEO_GAMEPAD_BUTTON_FACE_UP, true);
	tp.setButton(LEO_GAMEPAD_BUTTON_FACE_LEFT, true);
	tick_frame();
	REQUIRE(leo_GetGamepadButtonPressed() == LEO_GAMEPAD_BUTTON_FACE_LEFT);

	tick_frame();
	REQUIRE(leo_GetGamepadButtonPressed() == -1);

	tp.setButton(LEO_GAMEPAD_BUTTON_FACE_UP, false);
	tp.setButton(LEO_GAMEPAD_BUTTON_FACE_LEFT, false);
	tp.setButton(LEO_GAMEPAD_BUTTON_START, true);
	tick_frame();
	REQUIRE(leo_GetGamepadButtonPressed() == LEO_GAMEPAD_BUTTON_START);
}

TEST_CASE("Rumble call is safe on virtual gamepad", "[gamepad][rumble]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	(void)leo_SetGamepadVibration(0, 0.5f, 0.5f, 0.05f);
	SUCCEED();
}

TEST_CASE("Gamepad API handles invalid inputs", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();

	// Invalid gamepad index
	REQUIRE_FALSE(leo_IsGamepadAvailable(-1));
	REQUIRE_FALSE(leo_IsGamepadAvailable(LEO_MAX_GAMEPADS));
	REQUIRE(leo_GetGamepadName(-1) == nullptr);
	REQUIRE_FALSE(leo_IsGamepadButtonDown(-1, LEO_GAMEPAD_BUTTON_FACE_DOWN));
	REQUIRE(leo_GetGamepadAxisMovement(-1, LEO_GAMEPAD_AXIS_LEFT_X) == 0.0f);

	// Invalid button/axis/stick/dir
	REQUIRE_FALSE(leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_COUNT));
	REQUIRE(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_COUNT) == 0.0f);
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, 999, LEO_GAMEPAD_DIR_RIGHT));
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, LEO_GAMEPAD_STICK_LEFT, 999));
}

TEST_CASE("Multiple gamepads", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad pads[LEO_MAX_GAMEPADS];
	for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
	{
		pads[i].attach();
		REQUIRE(leo_IsGamepadAvailable(i));
	}
	// Try to attach one more (should fail)
	REQUIRE(leo__test_attach_pad("Overflow") == -1);
	tick_frame();
	for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
	{
		pads[i].detach();
	}
	tick_frame();
	for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
	{
		REQUIRE_FALSE(leo_IsGamepadAvailable(i));
	}
}

TEST_CASE("All buttons map correctly", "[gamepad]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	for (int b = 0; b < LEO_GAMEPAD_BUTTON_COUNT; ++b)
	{
		tp.setButton(b, true);
		tick_frame();
		REQUIRE(leo_IsGamepadButtonDown(0, b));
		REQUIRE(leo_IsGamepadButtonPressed(0, b));
		tp.setButton(b, false);
		tick_frame();
		REQUIRE(leo_IsGamepadButtonUp(0, b));
		REQUIRE(leo_IsGamepadButtonReleased(0, b));
	}
}

TEST_CASE("Stick vector and all directions", "[gamepad][stick]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	leo_SetGamepadStickThreshold(0.60f, 0.40f);
	leo_SetGamepadAxisDeadzone(0.10f);

	struct
	{
		int stick;
		int axis_x, axis_y;
	} sticks[] = {
				{ LEO_GAMEPAD_STICK_LEFT, LEO_GAMEPAD_AXIS_LEFT_X, LEO_GAMEPAD_AXIS_LEFT_Y },
				{ LEO_GAMEPAD_STICK_RIGHT, LEO_GAMEPAD_AXIS_RIGHT_X, LEO_GAMEPAD_AXIS_RIGHT_Y }
			};
	for (const auto& s: sticks)
	{
		// Test RIGHT direction
		tp.setAxis(s.axis_x, 0.70f);
		tick_frame();
		leo_Vector2 v = leo_GetGamepadStick(0, s.stick);
		REQUIRE(Approx(v.x).margin(0.01f) == 0.70f);
		REQUIRE(leo_IsGamepadStickDown(0, s.stick, LEO_GAMEPAD_DIR_RIGHT));

		// Test UP direction (negative Y)
		tp.setAxis(s.axis_x, 0.0f);
		tp.setAxis(s.axis_y, -0.70f);
		tick_frame();
		v = leo_GetGamepadStick(0, s.stick);
		REQUIRE(Approx(v.y).margin(0.01f) == -0.70f);
		REQUIRE(leo_IsGamepadStickDown(0, s.stick, LEO_GAMEPAD_DIR_UP));
	}
}

TEST_CASE("Trigger edge cases", "[gamepad][triggers]")
{
	LeoGamepadEnv env;
	TestPad tp;
	tp.attach();
	tp.setAxis(LEO_GAMEPAD_AXIS_LEFT_TRIGGER, -0.01f);
	tick_frame();
	REQUIRE(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_TRIGGER) == 0.0f);

	tp.setAxis(LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, 1.01f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_RIGHT_TRIGGER)).margin(0.01f) == 1.0f);
}
