// tests/gamepad_tests.cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <SDL3/SDL.h>
#include "leo/gamepad.h"
#include "leo/engine.h" // for leo_Vector2

using Catch::Approx;

// ---------- Helpers: Virtual gamepad (SDL3) ----------
//
// SDL3 provides a virtual joystick API we can mark as a GAMEPAD and
// set axes/buttons programmatically. That lets us run deterministic tests.
//
// Notes:
// - If your SDL3 build uses slightly different names (early commits changed a few),
//   adjust the SDL_AttachVirtualJoystick / SDL_SetJoystickVirtual* calls accordingly.

struct SdlScoped
{
	SdlScoped()
	{
		// Initialize what we need; SDL_INIT_GAMEPAD implies joystick subsystem too.
		REQUIRE(SDL_Init(SDL_INIT_GAMEPAD));
	}

	~SdlScoped() { SDL_Quit(); }
};

struct VirtualGamepad
{
	SDL_JoystickID jid = 0;
	SDL_Joystick* joy = nullptr;
	bool attached = false;

	void attach(int naxes = 6, int nbuttons = 15, int nhats = 0)
	{
		SDL_VirtualJoystickDesc desc;
		SDL_zero(desc);
		// SDL3 requires a nonzero version; older headers may not define the macro.
#ifdef SDL_VIRTUAL_JOYSTICK_DESC_VERSION
		desc.version    = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
#else
		desc.version = 1; // fallback for older SDL3 headers
#endif
		desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
		desc.naxes = naxes; // 6 for gamepad: LX,LY,RX,RY,LT,RT
		desc.nbuttons = nbuttons; // per your enum
		desc.nhats = nhats;
		desc.name = "Leo Virtual Gamepad";
		desc.vendor_id = 0x045E; // optional, but helps mappings
		desc.product_id = 0x028E;

		jid = SDL_AttachVirtualJoystick(&desc);
		if (jid <= 0) { INFO(std::string("SDL_AttachVirtualJoystick failed: ") + SDL_GetError()); }
		REQUIRE(jid > 0);

		joy = SDL_OpenJoystick(jid);
		if (!joy) { INFO(std::string("SDL_OpenJoystick failed: ") + SDL_GetError()); }
		REQUIRE(joy != nullptr);

		attached = true;

		// push synthetic event into Leo
		SDL_Event ev{};
		ev.type = SDL_EVENT_GAMEPAD_ADDED;
		ev.gdevice.which = jid;
		leo_HandleGamepadEvent(&ev);
	}


	void detach()
	{
		if (!attached) return;
		SDL_Event ev{};
		ev.type = SDL_EVENT_GAMEPAD_REMOVED;
		ev.gdevice.which = jid;
		leo_HandleGamepadEvent(&ev);

		if (joy)
		{
			SDL_CloseJoystick(joy);
			joy = nullptr;
		}
		SDL_DetachVirtualJoystick(jid);
		attached = false;
	}

	void setButton(int buttonIndex, bool down)
	{
		REQUIRE(joy != nullptr);
		REQUIRE(SDL_SetJoystickVirtualButton(joy, buttonIndex, down ? 1 : 0) == 0);
	}

	void setAxisRaw(int axisIndex, Sint16 value)
	{
		REQUIRE(joy != nullptr);
		REQUIRE(SDL_SetJoystickVirtualAxis(joy, axisIndex, value) == 0);
	}

	// Helpers: map Leo axis enum to SDL axis index. Our implementation used SDL_GAMEPAD_AXIS_*,
	// which SDL orders as: LEFTX=0, LEFTY=1, RIGHTX=2, RIGHTY=3, LTRIG=4, RTRIG=5.
	static int sdlAxisIndex(int leoAxis)
	{
		return leoAxis; // identical ordering in our mapping
	}

	// Convert desired normalized float [-1,+1] to SDL 16-bit raw and set it.
	void setAxisNorm(int leoAxis, float norm)
	{
		// Clamp and convert to SDL-style signed 16-bit.
		if (norm > 1.f) norm = 1.f;
		if (norm < -1.f) norm = -1.f;
		Sint16 raw = (norm >= 0.f)
			             ? (Sint16)(norm * 32767.f + 0.5f)
			             : (Sint16)(norm * 32768.f - 0.5f);
		setAxisRaw(sdlAxisIndex(leoAxis), raw);
	}

	// Triggers in Leo are clamped to [0,+1], but SDL’s raw can be signed; we still feed signed space here.
	void setTriggerNorm(int leoAxis, float norm01)
	{
		if (norm01 < 0.f) norm01 = 0.f;
		if (norm01 > 1.f) norm01 = 1.f;
		Sint16 raw = (Sint16)(norm01 * 32767.f + 0.5f);
		setAxisRaw(sdlAxisIndex(leoAxis), raw);
	}
};

// Make sure each test starts with a clean Leo state.
struct LeoGamepadEnv
{
	LeoGamepadEnv() { leo_InitGamepads(); }
	~LeoGamepadEnv() { leo_ShutdownGamepads(); }
};

// Helper: pump one “frame” worth of updates (and clear any stale SDL events).
static void tick_frame()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		// The engine loop would call this; tests do too for completeness.
		leo_HandleGamepadEvent(&e);
	}
	leo_UpdateGamepads();
}

// ---------- TESTS ----------

TEST_CASE("Gamepad connects and exposes name", "[gamepad]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;

	VirtualGamepad vp;
	vp.attach();

	tick_frame();

	REQUIRE(leo_IsGamepadAvailable(0));
	auto* name = leo_GetGamepadName(0);
	// Some virtuals may have no name; allow nullptr or non-empty
	if (name == nullptr)
	{
		SUCCEED();
	}
	else
	{
		REQUIRE(name[0] != '\0');
	}

	vp.detach();
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadAvailable(0));
}

TEST_CASE("Button edges: Pressed/Down/Released/Up", "[gamepad]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	// Choose a face button (A / Cross)
	const int B = LEO_GAMEPAD_BUTTON_FACE_DOWN;

	// Frame 1: nothing pressed
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadButtonDown(0, B));
	REQUIRE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	// Frame 2: press
	vp.setButton(B, true);
	tick_frame();
	REQUIRE(leo_IsGamepadButtonDown(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonUp(0, B));
	REQUIRE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	// Frame 3: hold
	tick_frame();
	REQUIRE(leo_IsGamepadButtonDown(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonReleased(0, B));

	// Frame 4: release
	vp.setButton(B, false);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadButtonDown(0, B));
	REQUIRE(leo_IsGamepadButtonUp(0, B));
	REQUIRE_FALSE(leo_IsGamepadButtonPressed(0, B));
	REQUIRE(leo_IsGamepadButtonReleased(0, B));

	vp.detach();
}

TEST_CASE("Axis normalization and deadzone", "[gamepad]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	// Ensure a known deadzone
	leo_SetGamepadAxisDeadzone(0.20f);

	// Within deadzone -> 0
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.15f);
	tick_frame();
	REQUIRE(std::fabs(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X) - 0.0f) < 1e-6f);

	// Above deadzone -> tracks value (no rescale in implementation)
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.60f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X)).margin(0.02f) == 0.60f);

	// Negative values
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, -0.75f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X)).margin(0.02f) == -0.75f);

	vp.detach();
}

TEST_CASE("Stick virtual buttons with hysteresis (RIGHT dir)", "[gamepad][stick]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	// Hysteresis: press at 0.6, release at 0.4
	leo_SetGamepadStickThreshold(0.60f, 0.40f);
	leo_SetGamepadAxisDeadzone(0.10f);

	const int ST = LEO_GAMEPAD_STICK_LEFT;
	const int DIR = LEO_GAMEPAD_DIR_RIGHT;

	// Baseline
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.0f);
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_Y, 0.0f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickUp(0, ST, DIR));

	// Cross press threshold -> pressed & down
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.70f);
	tick_frame();
	REQUIRE(leo_IsGamepadStickPressed(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE_FALSE(leo_IsGamepadStickReleased(0, ST, DIR));

	// Between thresholds -> still down, no edges
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.50f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickPressed(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE_FALSE(leo_IsGamepadStickReleased(0, ST, DIR));

	// Below release threshold -> released & up
	vp.setAxisNorm(LEO_GAMEPAD_AXIS_LEFT_X, 0.30f);
	tick_frame();
	REQUIRE_FALSE(leo_IsGamepadStickDown(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickUp(0, ST, DIR));
	REQUIRE(leo_IsGamepadStickReleased(0, ST, DIR));

	vp.detach();
}

TEST_CASE("Triggers clamped to [0,+1] by public API", "[gamepad][triggers]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	// Feed "negative" raw to simulate noisy driver; public API should clamp to [0,+1].
	vp.setAxisRaw(VirtualGamepad::sdlAxisIndex(LEO_GAMEPAD_AXIS_LEFT_TRIGGER), -12000);
	tick_frame();
	REQUIRE(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_TRIGGER) >= 0.0f);

	// Full press
	vp.setTriggerNorm(LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, 1.0f);
	tick_frame();
	REQUIRE(Approx(leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_RIGHT_TRIGGER)).margin(0.01f) == 1.0f);

	vp.detach();
}

TEST_CASE("Last button pressed (per-frame) reports lowest first", "[gamepad]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	// Press two different buttons in same frame; API returns the lowest-numbered.
	vp.setButton(LEO_GAMEPAD_BUTTON_FACE_UP, true);
	vp.setButton(LEO_GAMEPAD_BUTTON_FACE_LEFT, true);
	tick_frame();

	int last = leo_GetGamepadButtonPressed();
	REQUIRE(last == LEO_GAMEPAD_BUTTON_FACE_LEFT);

	// Next frame (no new edges) -> -1
	tick_frame();
	REQUIRE(leo_GetGamepadButtonPressed() == -1);

	// Release and press another
	vp.setButton(LEO_GAMEPAD_BUTTON_FACE_UP, false);
	vp.setButton(LEO_GAMEPAD_BUTTON_FACE_LEFT, false);
	vp.setButton(LEO_GAMEPAD_BUTTON_START, true);
	tick_frame();
	REQUIRE(leo_GetGamepadButtonPressed() == LEO_GAMEPAD_BUTTON_START);

	vp.detach();
}

// We don't assert rumble == true because virtual gamepads typically don't support it.
// This just exercises the code path to ensure it doesn't crash.
TEST_CASE("Rumble call is safe on virtual gamepad", "[gamepad][rumble]")
{
	SdlScoped sdl;
	LeoGamepadEnv env;
	VirtualGamepad vp;
	vp.attach();

	(void)leo_SetGamepadVibration(0, 0.5f, 0.5f, 0.05f);
	SUCCEED();

	vp.detach();
}
