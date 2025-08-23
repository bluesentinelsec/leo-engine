int W = 800, H = 440;
leo_RenderTexture2D screenCamera1 = leo_LoadRenderTexture(W/2, H);
leo_RenderTexture2D screenCamera2 = leo_LoadRenderTexture(W/2, H);

leo_Rectangle splitScreenRect = {
    0.0f, 0.0f,
    (float)screenCamera1.texture.width,
    (float)-screenCamera1.texture.height   // NEGATIVE height => flip vertically (raylib trick)
};

while (!leo_WindowShouldClose())
{
    // Update players & cameras...
    camera1.target = (leo_Vector2){ player1.x, player1.y };
    camera2.target = (leo_Vector2){ player2.x, player2.y };

    // Left eye
    leo_BeginTextureMode(screenCamera1);
        leo_ClearBackground(255,255,255,255);   // RAYWHITE
        leo_BeginMode2D(camera1);
            // draw grid, text, rectangles, etc...
        leo_EndMode2D();
    leo_EndTextureMode();

    // Right eye
    leo_BeginTextureMode(screenCamera2);
        leo_ClearBackground(255,255,255,255);
        leo_BeginMode2D(camera2);
            // draw scene again...
        leo_EndMode2D();
    leo_EndTextureMode();

    // Composite to backbuffer
    leo_BeginDrawing();
        leo_ClearBackground(0,0,0,255);

        // left
        leo_DrawTextureRec(
            screenCamera1.texture,
            splitScreenRect,
            (leo_Vector2){ 0.0f, 0.0f },
            (leo_Color){255,255,255,255}
        );

        // right
        leo_DrawTextureRec(
            screenCamera2.texture,
            splitScreenRect,
            (leo_Vector2){ W/2.0f, 0.0f },
            (leo_Color){255,255,255,255}
        );

        // center divider
        leo_DrawRectangle(W/2 - 2, 0, 4, H, (leo_Color){ 200,200,200,255 });
    leo_EndDrawing();
}

leo_UnloadRenderTexture(screenCamera1);
leo_UnloadRenderTexture(screenCamera2);

