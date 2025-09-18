#include "leo/animation.h"
#include "leo/graphics.h"
#include <string.h>

leo_Animation leo_LoadAnimation(const char *filename, int frameWidth, int frameHeight, 
                               int frameCount, float frameTime, bool loop) {
    leo_Animation anim = {0};
    
    anim.texture = leo_LoadTexture(filename);
    anim.frameCount = frameCount;
    anim.frameWidth = frameWidth;
    anim.frameHeight = frameHeight;
    anim.frameTime = frameTime;
    anim.loop = loop;
    
    return anim;
}

leo_AnimationPlayer leo_CreateAnimationPlayer(leo_Animation *animation) {
    leo_AnimationPlayer player = {0};
    
    player.animation = animation;
    player.currentFrame = 0;
    player.timer = 0.0f;
    player.playing = false;
    
    return player;
}

void leo_UpdateAnimation(leo_AnimationPlayer *player, float deltaTime) {
    if (!player || !player->animation || !player->playing) {
        return;
    }
    
    player->timer += deltaTime;
    
    if (player->timer >= player->animation->frameTime) {
        player->timer = 0.0f;
        player->currentFrame++;
        
        if (player->currentFrame >= player->animation->frameCount) {
            if (player->animation->loop) {
                player->currentFrame = 0;
            } else {
                player->currentFrame = player->animation->frameCount - 1;
                player->playing = false;
            }
        }
    }
}

void leo_DrawAnimation(leo_AnimationPlayer *player, int x, int y) {
    if (!player || !player->animation) {
        return;
    }
    
    leo_Animation *anim = player->animation;
    
    // Calculate frame position in spritesheet
    int framesPerRow = anim->texture.width / anim->frameWidth;
    int frameX = (player->currentFrame % framesPerRow) * anim->frameWidth;
    int frameY = (player->currentFrame / framesPerRow) * anim->frameHeight;
    
    leo_Rectangle sourceRec = {
        (float)frameX, 
        (float)frameY, 
        (float)anim->frameWidth, 
        (float)anim->frameHeight
    };
    
    leo_Rectangle destRec = {
        (float)x, 
        (float)y, 
        (float)anim->frameWidth, 
        (float)anim->frameHeight
    };
    
    leo_Vector2 origin = {0, 0};
    
    leo_DrawTexturePro(anim->texture, sourceRec, destRec, origin, 0.0f, LEO_WHITE);
}

void leo_PlayAnimation(leo_AnimationPlayer *player) {
    if (player) {
        player->playing = true;
    }
}

void leo_PauseAnimation(leo_AnimationPlayer *player) {
    if (player) {
        player->playing = false;
    }
}

void leo_ResetAnimation(leo_AnimationPlayer *player) {
    if (player) {
        player->currentFrame = 0;
        player->timer = 0.0f;
    }
}

void leo_UnloadAnimation(leo_Animation *animation) {
    if (animation) {
        leo_UnloadTexture(&animation->texture);
        memset(animation, 0, sizeof(leo_Animation));
    }
}
