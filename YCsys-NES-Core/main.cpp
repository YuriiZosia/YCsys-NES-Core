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

int main(int argc, char* argv[]) {
    // Встановлюємо кодування UTF-8 для консолі Windows, щоб бачити українську мову
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 1. Ініціалізація підсистеми відео SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Створюємо головне вікно програми (768x720 пікселів при SCALE = 3)
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

    // Створюємо апаратний рендерер із підтримкою Vertical Sync
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Створюємо 32-бітну ARGB текстуру, куди будемо копіювати наш екранний масив
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256, 240
    );

    // 2. Створюємо екземпляр нашої віртуальної консолі
    // Створюємо консоль у Купі (надійно і не грузить стек)
    Bus* nes = new Bus();

    // =================================================================
    // ЗАВАНТАЖЕННЯ ГРИ (ROM) ТА СТАРТ СИСТЕМИ
    // =================================================================
    // Створюємо картридж
    std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>("games\\Super Mario Bros.nes");

    if (!cart->bImageValid) {
        std::cerr << "Помилка: Не вдалося завантажити ROM!" << std::endl;
        // Тут поки що буде просто чорний/сірий екран, якщо файлу немає
    }
    else {
        nes->insertCartridge(cart); // Вставляємо картридж у гніздо нашої шини
    }

    // КРИТИЧНО ВАЖЛИВО: Перезавантажуємо CPU!
    // Це змусить процесор прочитати Вектор Старту (0xFFFC) з картриджа 
    // і почати виконувати код гри з правильної адреси.
    nes->cpu.reset();
    // =================================================================

    // 3. Головний ігровий та графічний цикл
    bool bQuit = false;
    SDL_Event event;

    while (!bQuit) {
        // Обробка системних подій ОС (закриття вікна, рух миші тощо)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                bQuit = true;
            }
        }

        // =================================================================
        // ОПИТУВАННЯ КЛАВІАТУРИ ДЛЯ КОНТРОЛЕРА 1
        // =================================================================
        const Uint8* state = SDL_GetKeyboardState(NULL);
        nes->controller[0] = 0x00; // Очищуємо старий стан

        // Порядок бітів (Shift Register): A, B, Select, Start, Up, Down, Left, Right
        nes->controller[0] |= state[SDL_SCANCODE_X] ? 0x80 : 0x00; // A
        nes->controller[0] |= state[SDL_SCANCODE_Z] ? 0x40 : 0x00; // B
        nes->controller[0] |= state[SDL_SCANCODE_A] ? 0x20 : 0x00; // Select
        nes->controller[0] |= state[SDL_SCANCODE_S] ? 0x10 : 0x00; // Start
        nes->controller[0] |= state[SDL_SCANCODE_UP] ? 0x08 : 0x00; // Up
        nes->controller[0] |= state[SDL_SCANCODE_DOWN] ? 0x04 : 0x00; // Down
        nes->controller[0] |= state[SDL_SCANCODE_LEFT] ? 0x02 : 0x00; // Left
        nes->controller[0] |= state[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00; // Right

        // =================================================================

        // КРУТИМО СИСТЕМНИЙ ГОДИННИК, поки PPU не завершить рендеринг повного кадру
        while (!nes->ppu.frame_complete) {
            nes->clock();
        }
        // Скидаємо прапорець завершення кадру для наступного циклу розгортки
        nes->ppu.frame_complete = false;
        // 4. Оновлюємо текстуру вікна даними з нашого std::array
        SDL_UpdateTexture(texture, nullptr, nes->ppu.sprScreen.data(), 256 * sizeof(uint32_t));

        // Очищуємо екран, копіюємо текстуру у вікно з масштабуванням і виводимо на монітор
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Фінальне вивільнення ресурсів при закритті програми
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete nes; // Знищуємо об'єкт консолі.

    return 0;
}