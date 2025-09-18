#include <catch2/catch_test_macros.hpp>
#include "leo/animation.h"
#include "leo/engine.h"

TEST_CASE("Animation system basic functionality", "[animation]") {
    // Initialize engine for texture loading
    REQUIRE(leo_InitWindow(800, 600, "Animation Test"));
    
    SECTION("Animation creation and basic properties") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, true);
        
        REQUIRE(anim.frameCount == 4);
        REQUIRE(anim.frameWidth == 32);
        REQUIRE(anim.frameHeight == 32);
        REQUIRE(anim.frameTime == 0.1f);
        REQUIRE(anim.loop == true);
    }
    
    SECTION("Animation player creation") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, true);
        leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim);
        
        REQUIRE(player.animation == &anim);
        REQUIRE(player.currentFrame == 0);
        REQUIRE(player.timer == 0.0f);
        REQUIRE(player.playing == false);
    }
    
    SECTION("Animation playback control") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, true);
        leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim);
        
        leo_PlayAnimation(&player);
        REQUIRE(player.playing == true);
        
        leo_PauseAnimation(&player);
        REQUIRE(player.playing == false);
        
        leo_ResetAnimation(&player);
        REQUIRE(player.currentFrame == 0);
        REQUIRE(player.timer == 0.0f);
    }
    
    SECTION("Animation frame progression") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, true);
        leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim);
        leo_PlayAnimation(&player);
        
        // Update with exactly one frame time
        leo_UpdateAnimation(&player, 0.1f);
        REQUIRE(player.currentFrame == 1);
        
        // Update to next frame
        leo_UpdateAnimation(&player, 0.1f);
        REQUIRE(player.currentFrame == 2);
    }
    
    SECTION("Animation looping") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, true);
        leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim);
        leo_PlayAnimation(&player);
        
        // Advance to last frame
        player.currentFrame = 3;
        leo_UpdateAnimation(&player, 0.1f);
        
        // Should loop back to frame 0
        REQUIRE(player.currentFrame == 0);
    }
    
    SECTION("Animation non-looping") {
        leo_Animation anim = leo_LoadAnimation("test_sprite.png", 32, 32, 4, 0.1f, false);
        leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim);
        leo_PlayAnimation(&player);
        
        // Advance to last frame
        player.currentFrame = 3;
        leo_UpdateAnimation(&player, 0.1f);
        
        // Should stay at last frame and stop playing
        REQUIRE(player.currentFrame == 3);
        REQUIRE(player.playing == false);
    }
    
    leo_CloseWindow();
}
