#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <CLI11.hpp>
#include <stb_image.h>
#include <stb_truetype.h>
#include "version.h"

int main(int argc, char* argv[]) {
    CLI::App app{"Leo Engine"};
    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (show_version) {
        SDL_Log("leo-engine version %s", LEO_ENGINE_VERSION);
        return 0;
    }

    // Reference STB symbols to prove linking works
    (void)&stbi_load;
    (void)&stbtt_InitFont;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization Error",
                                  SDL_GetError(), nullptr);
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Leo Engine", 1280, 720, 0);
    if (!window) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Window Creation Error",
                                  SDL_GetError(), nullptr);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Creation Error",
                                  SDL_GetError(), nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect rect = {540.0f, 310.0f, 200.0f, 100.0f};
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
