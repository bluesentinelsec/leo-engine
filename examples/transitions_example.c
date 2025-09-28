#include "leo/leo.h"
#include "leo/transitions.h"

// Example callback for when transition completes
void on_fade_complete(void *user_data) {
    printf("Fade transition completed!\n");
    // You could trigger the next scene here
}

int main() {
    // Initialize Leo Engine
    if (!leo_Init(800, 600, "Transitions Example")) {
        return -1;
    }
    
    // Create actor system
    leo_ActorSystem *actor_sys = leo_actor_system_create();
    leo_Actor *root = leo_actor_system_root(actor_sys);
    
    // Start a fade-in transition
    leo_TransitionDesc fade_desc = {
        .type = LEO_TRANSITION_FADE,
        .duration = 2.0f,
        .color = LEO_BLACK,
        .on_complete = on_fade_complete,
        .user_data = NULL
    };
    
    leo_Actor *fade_transition = leo_transition_start(root, &fade_desc);
    
    // Main game loop
    while (!leo_WindowShouldClose()) {
        float dt = leo_GetFrameTime();
        
        // Update actors (including transitions)
        leo_actor_system_update(actor_sys, dt);
        
        // Render
        leo_BeginDrawing();
        leo_ClearBackground(LEO_WHITE);
        
        // Your game rendering here...
        
        // Render actors (including transitions)
        leo_actor_system_render(actor_sys);
        
        leo_EndDrawing();
        
        // Example: Start a circle-out transition after 3 seconds
        static bool circle_started = false;
        static float timer = 0.0f;
        timer += dt;
        
        if (!circle_started && timer > 3.0f) {
            leo_TransitionDesc circle_desc = {
                .type = LEO_TRANSITION_CIRCLE_OUT,
                .duration = 1.5f,
                .color = LEO_RED,
                .on_complete = NULL,
                .user_data = NULL
            };
            leo_transition_start(root, &circle_desc);
            circle_started = true;
        }
    }
    
    // Cleanup
    leo_actor_system_destroy(actor_sys);
    leo_Close();
    
    return 0;
}
