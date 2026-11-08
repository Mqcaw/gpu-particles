#include "App.h"
#include <SDL3/SDL.h>

App::App(int argc, char **argv): m_window(nullptr, &SDL_DestroyWindow) {

}

SDL_AppResult App::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    m_window.reset(SDL_CreateWindow("Gpu Particles", 800, 600, SDL_WINDOW_RESIZABLE));
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::Iterate() {
    if (const auto result = OnUpdate(); result != SDL_APP_CONTINUE)
        return result;
    if (const auto result = OnRender(); result != SDL_APP_CONTINUE)
        return result;
}

SDL_AppResult App::Event(const SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return OnQuit();
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (SDL_GetWindowID(m_window.get()) == event->window.windowID) {
                return OnQuit();
            }
        default: return SDL_APP_CONTINUE;
    }
}

void App::Quit(SDL_AppResult result) {
}

SDL_AppResult App::OnQuit() {
    return SDL_APP_SUCCESS;
}

SDL_AppResult App::OnRender() {
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::OnUpdate() {
    return SDL_APP_CONTINUE;
}
