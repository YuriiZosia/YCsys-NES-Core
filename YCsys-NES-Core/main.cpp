/*
 * _________________________________________________________________________
 * |     __   __  ______   ______              __    _  _______  _______     |
 * |     \ \ / / |  ____| |  ____|     _      |  \  | ||  _____||  _____|    |
 * |      \   /  | |      | |____     (_)     |   \ | || |_____ | |_____     |
 * |       | |   | |      |____  |    _       | |\   ||  _____| \____  |     |
 * |       | |   | |____   ____| |   (_)      | | \  || |_____  _____| |     |
 * |       |_|   |______| |______|            |_|  \_||_______||_______|     |
 * |        Y C s y s                          N E S   C O R E               |
 * |_________________________________________________________________________|
 * |                                                                         |
 * |      [!]  YCsys NES CORE - MAIN ENTRY POINT & SYSTEM LOOP  [!]          |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [PWR]        [JOY]        [RST]        [SYS]        [PWR]
 */

#include <iostream>
#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 26819) // Вимикаємо попередження про fallthrough
#pragma warning(disable: 26451) // Вимикаємо попередження про переповнення для SDL
#include <SDL.h>
#pragma warning(pop)            // Повертаємо аналізатор до звичного суворого режиму
#include "Bus.h"

 // Коефіцієнт масштабування вікна (оригінал 256x240 занадто малий для моніторів)
const int SCALE = 3;

// Безпечна Callback-функція звуку SDL2 (Контекст шини передається через userdata)
static void AudioCallback(void* userdata, Uint8* stream, int len) {
    float* fStream = reinterpret_cast<float*>(stream);
    int samples = len / static_cast<int>(sizeof(float));

    Bus* nes_context = reinterpret_cast<Bus*>(userdata);

    for (int i = 0; i < samples; i++) {
        if (nes_context) {
            fStream[i] = static_cast<float>(nes_context->apu.GetOutputSample());
        }
        else {
            fStream[i] = 0.0f;
        }
    }
}

int main(int argc, char* argv[]) {
    // Встановлюємо кодування UTF-8 для консолі Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 1. Ініціалізація підсистем ВІДЕО та АУДІО SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "YCsys NES Core",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256 * SCALE, 240 * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window Creation Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256, 240
    );

    // 2. Створюємо екземпляр віртуальної консолі у Купі
    Bus* nes = new Bus();

    // =================================================================
    // ЗАВАНТАЖЕННЯ ГРИ (ROM) ТА СТАРТ СИСТЕМИ
    // =================================================================
    std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>("games\\Super Mario Bros.nes");

    if (!cart->bImageValid) {
        std::cerr << "Помилка: Не вдалося завантажити ROM!" << std::endl;
    }
    else {
        nes->insertCartridge(cart);
    }

    nes->cpu.reset();

    // =================================================================
    // НАЛАШТУВАННЯ АУДІО СИСТЕМИ SDL2
    // =================================================================
    SDL_AudioSpec audio_spec{};
    audio_spec.freq = 44100;          // Частота дискретизації (44.1 кГц)
    audio_spec.format = AUDIO_F32SYS; // 32-бітний float
    audio_spec.channels = 1;          // Моно
    audio_spec.samples = 1024;        // Розмір звукового буфера
    audio_spec.callback = nullptr;    // ФІКС: Вимикаємо Callback! Будемо пушити звук вручну
    audio_spec.userdata = nes;        // ПЕРЕДАЄМО НАШУ ШИНУ ЯК КОНТЕКСТ!

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
    if (audio_device > 0) {
        SDL_PauseAudioDevice(audio_device, 0); // Вмикаємо аудіо-пристрій
    }
    else {
        std::cerr << "Audio Device Error: " << SDL_GetError() << std::endl;
    }

    // 3. Головний ігровий та графічний цикл
    bool bQuit = false;
    SDL_Event event;
    double dAudioTime = 0.0;
    const double dAudioTimePerSystemClock = 1.0 / 5369318.0; // 1 системний такт PPU
    const double dAudioTimePerSample = 1.0 / 44100.0;        // 1 семпл для SDL

    while (!bQuit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                bQuit = true;
            }
        }

        // Опитування клавіатури
        const Uint8* state = SDL_GetKeyboardState(NULL);
        nes->controller[0] = 0x00;

        nes->controller[0] |= state[SDL_SCANCODE_X] ? 0x80 : 0x00; // A
        nes->controller[0] |= state[SDL_SCANCODE_Z] ? 0x40 : 0x00; // B
        nes->controller[0] |= state[SDL_SCANCODE_A] ? 0x20 : 0x00; // Select
        nes->controller[0] |= state[SDL_SCANCODE_S] ? 0x10 : 0x00; // Start
        nes->controller[0] |= state[SDL_SCANCODE_UP] ? 0x08 : 0x00; // Up
        nes->controller[0] |= state[SDL_SCANCODE_DOWN] ? 0x04 : 0x00; // Down
        nes->controller[0] |= state[SDL_SCANCODE_LEFT] ? 0x02 : 0x00; // Left
        nes->controller[0] |= state[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00; // Right

        while (!nes->ppu.frame_complete) {
            nes->clock();
            // Синхронізація звуку
            dAudioTime += dAudioTimePerSystemClock;
            if (dAudioTime >= dAudioTimePerSample) {
                dAudioTime -= dAudioTimePerSample; // Скидаємо таймер

                // Беремо готовий звук з APU і миттєво відправляємо в динаміки
                float sample = static_cast<float>(nes->apu.GetOutputSample());
                SDL_QueueAudio(audio_device, &sample, sizeof(float));
            }
        }
        nes->ppu.frame_complete = false;

        SDL_UpdateTexture(texture, nullptr, nes->ppu.sprScreen.data(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Фінальне вивільнення ресурсів при закритті програми
    if (audio_device > 0) {
        SDL_CloseAudioDevice(audio_device);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete nes;
    return 0;
}