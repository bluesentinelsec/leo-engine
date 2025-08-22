#include <catch2/catch_all.hpp>
#include <SDL3/SDL.h>

extern "C" {
#include "leo/keyboard.h"
}

// Direct state manipulation test harness via accessors
class KeyboardTestHarness
{
public:
	void EnableTestMode() { leo_EnableTestMode(); }
	void DisableTestMode() { leo_DisableTestMode(); }

	void SetKeyState(SDL_Scancode scancode, bool pressed)
	{
		leo_SetTestKeyState(scancode, pressed);
	}

	void SimulateFrame() { leo_UpdateKeyboard(); }

	void ResetState()
	{
		const int n = leo__test_num_keys();
		for (int i = 0; i < n; ++i)
		{
			leo_SetTestKeyState((SDL_Scancode)i, false);
		}
		leo_UpdateKeyboard(); // copy current -> prev
		for (int i = 0; i < n; ++i)
		{
			leo_SetTestKeyState((SDL_Scancode)i, false);
		}
	}

	bool GetCurrentKeyState(SDL_Scancode scancode)
	{
		const int n = leo__test_num_keys();
		if (scancode >= 0 && scancode < n)
		{
			return leo__test_current()[scancode];
		}
		return false;
	}

	bool GetPreviousKeyState(SDL_Scancode scancode)
	{
		const int n = leo__test_num_keys();
		if (scancode >= 0 && scancode < n)
		{
			return leo__test_prev()[scancode];
		}
		return false;
	}

	int GetNumKeys() { return leo__test_num_keys(); }
};

TEST_CASE("Keyboard initialization and cleanup", "[keyboard][init]")
{
	SECTION("Initial state")
	{
		leo_UpdateKeyboard(); // initialize
		KeyboardTestHarness harness;
		CHECK(harness.GetNumKeys() > 0);
		CHECK(harness.GetCurrentKeyState(SDL_SCANCODE_SPACE) == false);
		CHECK(harness.GetPreviousKeyState(SDL_SCANCODE_SPACE) == false);
	}

	SECTION("Cleanup and reinitialization")
	{
		leo_UpdateKeyboard();
		KeyboardTestHarness harness;
		int initialNumKeys = harness.GetNumKeys();

		leo_CleanupKeyboard();
		leo_UpdateKeyboard();
		CHECK(harness.GetNumKeys() == initialNumKeys);
	}
}

TEST_CASE("Basic key state functions", "[keyboard][basic]")
{
	leo_UpdateKeyboard();
	KeyboardTestHarness harness;
	harness.EnableTestMode();
	harness.ResetState();

	SECTION("leo_IsKeyDown - key currently pressed")
	{
		harness.SetKeyState(SDL_SCANCODE_SPACE, true);
		leo_UpdateKeyboard();
		CHECK(leo_IsKeyDown(SDL_SCANCODE_SPACE) == true);

		harness.SetKeyState(SDL_SCANCODE_SPACE, false);
		leo_UpdateKeyboard();
		CHECK(leo_IsKeyDown(SDL_SCANCODE_SPACE) == false);
	}

	SECTION("leo_IsKeyUp - key not currently pressed")
	{
		harness.SetKeyState(SDL_SCANCODE_SPACE, false);
		leo_UpdateKeyboard();
		CHECK(leo_IsKeyUp(SDL_SCANCODE_SPACE) == true);

		harness.SetKeyState(SDL_SCANCODE_SPACE, true);
		leo_UpdateKeyboard();
		CHECK(leo_IsKeyUp(SDL_SCANCODE_SPACE) == false);
	}

	harness.DisableTestMode();
}

TEST_CASE("Key press detection", "[keyboard][press]")
{
	leo_UpdateKeyboard();
	KeyboardTestHarness harness;
	harness.EnableTestMode();
	harness.ResetState();

	SECTION("leo_IsKeyPressed - detects initial key press")
	{
		harness.SetKeyState(SDL_SCANCODE_SPACE, false);
		leo_UpdateKeyboard();

		harness.SetKeyState(SDL_SCANCODE_SPACE, true);
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_SPACE) == true);
	}

	SECTION("leo_IsKeyPressed - only true on first frame")
	{
		harness.SetKeyState(SDL_SCANCODE_SPACE, true);
		leo_UpdateKeyboard();

		leo_UpdateKeyboard();
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_SPACE) == false);
	}

	harness.DisableTestMode();
}

TEST_CASE("Key repeat detection", "[keyboard][repeat]")
{
	KeyboardTestHarness harness;
	harness.EnableTestMode();
	leo_UpdateKeyboard();
	harness.ResetState();

	SECTION("leo_IsKeyPressedRepeat - detects held keys")
	{
		harness.SetKeyState(SDL_SCANCODE_W, true);
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_W) == true);
		leo_UpdateKeyboard();

		CHECK(leo_IsKeyPressed(SDL_SCANCODE_W) == false);
		CHECK(leo_IsKeyPressedRepeat(SDL_SCANCODE_W) == true);

		leo_UpdateKeyboard();
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_W) == false);
		CHECK(leo_IsKeyPressedRepeat(SDL_SCANCODE_W) == true);

		leo_UpdateKeyboard();
		CHECK(leo_IsKeyPressedRepeat(SDL_SCANCODE_W) == true);
	}
}

TEST_CASE("Key release detection", "[keyboard][release]")
{
	KeyboardTestHarness harness;
	harness.EnableTestMode();
	leo_UpdateKeyboard();
	harness.ResetState();

	SECTION("leo_IsKeyReleased - detects key release")
	{
		harness.SetKeyState(SDL_SCANCODE_A, true);
		leo_UpdateKeyboard();
		CHECK(leo_IsKeyDown(SDL_SCANCODE_A) == true);

		leo_UpdateKeyboard();

		harness.SetKeyState(SDL_SCANCODE_A, false);
		CHECK(leo_IsKeyReleased(SDL_SCANCODE_A) == true);
		CHECK(leo_IsKeyDown(SDL_SCANCODE_A) == false);
	}
}

TEST_CASE("Multiple key handling", "[keyboard][multiple]")
{
	KeyboardTestHarness harness;
	harness.EnableTestMode();
	leo_UpdateKeyboard();
	harness.ResetState();

	SECTION("Multiple keys pressed simultaneously")
	{
		harness.SetKeyState(SDL_SCANCODE_W, true);
		harness.SetKeyState(SDL_SCANCODE_A, true);
		harness.SetKeyState(SDL_SCANCODE_S, true);
		harness.SetKeyState(SDL_SCANCODE_D, true);

		CHECK(leo_IsKeyPressed(SDL_SCANCODE_W) == true);
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_A) == true);
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_S) == true);
		CHECK(leo_IsKeyPressed(SDL_SCANCODE_D) == true);

		leo_UpdateKeyboard();
	}

	SECTION("Mixed key states")
	{
		harness.SetKeyState(SDL_SCANCODE_W, true);
		harness.SetKeyState(SDL_SCANCODE_A, true);
		harness.SetKeyState(SDL_SCANCODE_S, false);
		harness.SetKeyState(SDL_SCANCODE_D, true);
		leo_UpdateKeyboard();

		harness.SetKeyState(SDL_SCANCODE_W, true);
		harness.SetKeyState(SDL_SCANCODE_A, true);
		harness.SetKeyState(SDL_SCANCODE_S, true);
		harness.SetKeyState(SDL_SCANCODE_D, false);

		CHECK(leo_IsKeyPressed(SDL_SCANCODE_S) == true);
		CHECK(leo_IsKeyPressedRepeat(SDL_SCANCODE_W) == true);
		CHECK(leo_IsKeyPressedRepeat(SDL_SCANCODE_A) == true);
		CHECK(leo_IsKeyReleased(SDL_SCANCODE_D) == true);

		leo_UpdateKeyboard();
	}
}

TEST_CASE("Key code conversion", "[keyboard][conversion]")
{
	leo_UpdateKeyboard();
	KeyboardTestHarness harness;
	harness.ResetState();

	SECTION("leo_GetKeyPressed - returns key codes")
	{
		harness.SetKeyState(SDL_SCANCODE_SPACE, true);

		int keyPressed = leo_GetKeyPressed();
		CHECK(keyPressed != 0);
		CHECK(keyPressed == SDL_GetKeyFromScancode(SDL_SCANCODE_SPACE, SDL_KMOD_NONE, false));

		leo_UpdateKeyboard();
	}

	SECTION("leo_GetCharPressed - returns printable characters")
	{
		harness.SetKeyState(SDL_SCANCODE_A, true);

		int charPressed = leo_GetCharPressed();
		CHECK((charPressed == 'A' || charPressed == 'a'));

		leo_UpdateKeyboard();
	}

	SECTION("leo_GetCharPressed - returns 0 for non-printable keys")
	{
		harness.SetKeyState(SDL_SCANCODE_ESCAPE, true);
		leo_UpdateKeyboard();

		int charPressed = leo_GetCharPressed();
		CHECK(charPressed == 0);
	}
}

TEST_CASE("Edge cases and error handling", "[keyboard][edge]")
{
	leo_UpdateKeyboard();
	KeyboardTestHarness harness;
	harness.ResetState();

	SECTION("Invalid scan codes")
	{
		CHECK(leo_IsKeyDown(-1) == false);
		CHECK(leo_IsKeyDown(99999) == false);
		CHECK(leo_IsKeyPressed(-1) == false);
		CHECK(leo_IsKeyReleased(99999) == false);
	}

	SECTION("Exit key functionality")
	{
		harness.SetKeyState(SDL_SCANCODE_ESCAPE, true);
		CHECK(leo_IsExitKeyPressed() == true);
		leo_UpdateKeyboard();

		leo_SetExitKey(SDL_SCANCODE_SPACE);
		harness.SetKeyState(SDL_SCANCODE_ESCAPE, false);
		harness.SetKeyState(SDL_SCANCODE_SPACE, true);
		CHECK(leo_IsExitKeyPressed() == true);
		leo_UpdateKeyboard();
	}
}
