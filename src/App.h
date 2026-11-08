#pragma once
#include <memory>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>

class App {
public:
    App(int argc, char **argv);

    SDL_AppResult Init();
    SDL_AppResult Iterate();
    SDL_AppResult Event(const SDL_Event *event);
    void Quit(SDL_AppResult result);

    enum AppLogCategory {
        APP_LOG_CATEGORY_GENERIC = SDL_LOG_CATEGORY_CUSTOM
    };

private:
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window;

    SDL_AppResult OnQuit();
    SDL_AppResult OnRender();
    SDL_AppResult OnUpdate();
};
