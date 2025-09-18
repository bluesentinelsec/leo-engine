#ifndef LEO_ANIMATION_H
#define LEO_ANIMATION_H

#include "leo/export.h"
#include "leo/image.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    leo_Texture2D texture;     // Source spritesheet
    int frameCount;            // Total frames
    int frameWidth, frameHeight; // Frame dimensions
    float frameTime;           // Time per frame (seconds)
    bool loop;                 // Should animation loop?
} leo_Animation;

typedef struct {
    leo_Animation *animation;  // Reference to animation data
    int currentFrame;          // Current frame index
    float timer;               // Internal timer
    bool playing;              // Is animation playing?
} leo_AnimationPlayer;

// Create animation from spritesheet
LEO_API leo_Animation leo_LoadAnimation(const char *filename, int frameWidth, int frameHeight, 
                                       int frameCount, float frameTime, bool loop);

// Create player instance
LEO_API leo_AnimationPlayer leo_CreateAnimationPlayer(leo_Animation *animation);

// Update animation (call each frame)
LEO_API void leo_UpdateAnimation(leo_AnimationPlayer *player, float deltaTime);

// Draw current frame
LEO_API void leo_DrawAnimation(leo_AnimationPlayer *player, int x, int y);

// Control functions
LEO_API void leo_PlayAnimation(leo_AnimationPlayer *player);
LEO_API void leo_PauseAnimation(leo_AnimationPlayer *player);
LEO_API void leo_ResetAnimation(leo_AnimationPlayer *player);

// Cleanup
LEO_API void leo_UnloadAnimation(leo_Animation *animation);

#ifdef __cplusplus
}
#endif

#endif // LEO_ANIMATION_H
