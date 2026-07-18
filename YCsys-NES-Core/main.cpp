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
#include <fstream>
#include <string>
#include <windows.h>
#include <commdlg.h> 
#include <deque>
#include <vector>
#include <array>
#include <algorithm> // Для std::copy

#pragma warning(push)
#pragma warning(disable: 26819)
#pragma warning(disable: 26451)
#include <SDL.h>
#pragma warning(pop)            

#include "Bus.h"

 // Глобальний масштаб екрана (динамічний)
int current_scale = 3;

// =================================================================
// СИСТЕМА МАШИНИ ЧАСУ (REWIND RING BUFFER)
// =================================================================
struct GameState {
    uint8_t cpu_a = 0;
    uint8_t cpu_x = 0;
    uint8_t cpu_y = 0;
    uint8_t cpu_stkp = 0;
    uint8_t cpu_status = 0;
    uint16_t cpu_pc = 0;

    std::array<uint8_t, 2048> cpuRam{};
    std::vector<uint8_t> prgRam;
    std::vector<uint32_t> screen;
};

static void CaptureState(Bus* nes, std::deque<GameState>& buffer, size_t max_size) {
    GameState state;
    state.cpu_a = nes->cpu.a;
    state.cpu_x = nes->cpu.x;
    state.cpu_y = nes->cpu.y;
    state.cpu_pc = nes->cpu.pc;
    state.cpu_stkp = nes->cpu.stkp;
    state.cpu_status = nes->cpu.status;
    state.cpuRam = nes->cpuRam;

    if (nes->cart && nes->cart->vPRGRAM.size() > 0) {
        state.prgRam = nes->cart->vPRGRAM;
    }

    state.screen.assign(nes->ppu.sprScreen.begin(), nes->ppu.sprScreen.end());

    buffer.push_back(std::move(state));

    if (buffer.size() > max_size) {
        buffer.pop_front();
    }
}

static void RestoreState(Bus* nes, const GameState& state) {
    nes->cpu.a = state.cpu_a;
    nes->cpu.x = state.cpu_x;
    nes->cpu.y = state.cpu_y;
    nes->cpu.pc = state.cpu_pc;
    nes->cpu.stkp = state.cpu_stkp;
    nes->cpu.status = state.cpu_status;
    nes->cpuRam = state.cpuRam;

    if (nes->cart && nes->cart->vPRGRAM.size() > 0 && state.prgRam.size() > 0) {
        nes->cart->vPRGRAM = state.prgRam;
    }

    if (state.screen.size() == nes->ppu.sprScreen.size()) {
        std::copy(state.screen.begin(), state.screen.end(), nes->ppu.sprScreen.begin());
    }
}

// =================================================================
// СИСТЕМА SAVE STATES (Швидке збереження на диск)
// =================================================================
static void SaveGameToDisk(Bus* nes) {
    std::ofstream out("ycsys_quick.savestate", std::ios::binary);
    if (out.is_open()) {
        out.write((char*)&nes->cpu.a, sizeof(uint8_t));
        out.write((char*)&nes->cpu.x, sizeof(uint8_t));
        out.write((char*)&nes->cpu.y, sizeof(uint8_t));
        out.write((char*)&nes->cpu.pc, sizeof(uint16_t));
        out.write((char*)&nes->cpu.stkp, sizeof(uint8_t));
        out.write((char*)&nes->cpu.status, sizeof(uint8_t));
        out.write((char*)nes->cpuRam.data(), 2048);
        if (nes->cart && nes->cart->vPRGRAM.size() > 0) {
            out.write((char*)nes->cart->vPRGRAM.data(), nes->cart->vPRGRAM.size());
        }
        out.close();
        std::cout << "[YCsys] State Saved Successfully to Disk!" << std::endl;
    }
}

static void LoadGameFromDisk(Bus* nes) {
    std::ifstream in("ycsys_quick.savestate", std::ios::binary);
    if (in.is_open()) {
        in.read((char*)&nes->cpu.a, sizeof(uint8_t));
        in.read((char*)&nes->cpu.x, sizeof(uint8_t));
        in.read((char*)&nes->cpu.y, sizeof(uint8_t));
        in.read((char*)&nes->cpu.pc, sizeof(uint16_t));
        in.read((char*)&nes->cpu.stkp, sizeof(uint8_t));
        in.read((char*)&nes->cpu.status, sizeof(uint8_t));
        in.read((char*)nes->cpuRam.data(), 2048);
        if (nes->cart && nes->cart->vPRGRAM.size() > 0) {
            in.read((char*)nes->cart->vPRGRAM.data(), nes->cart->vPRGRAM.size());
        }
        in.close();
        std::cout << "[YCsys] State Loaded Successfully from Disk!" << std::endl;
    }
}

// =================================================================
// ГОЛЛІВУДСЬКА BOOT-АНІМАЦІЯ
// =================================================================
static void PlayBootAnimation(SDL_Renderer* renderer) {
    for (int i = 0; i < 128; i += 4) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 200, 255, 255, 255);
        SDL_Rect line = { (128 - i) * current_scale, 119 * current_scale, (i * 2) * current_scale, 2 * current_scale };
        SDL_RenderFillRect(renderer, &line);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }
    for (int i = 0; i < 120; i += 6) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 200, 255, 255, 255);
        SDL_Rect rect = { 0, (120 - i) * current_scale, 256 * current_scale, (i * 2) * current_scale };
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(100);

    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_Rect y1 = { 80 * current_scale, 80 * current_scale, 10 * current_scale, 30 * current_scale };
    SDL_Rect y2 = { 100 * current_scale, 80 * current_scale, 10 * current_scale, 30 * current_scale };
    SDL_Rect y3 = { 80 * current_scale, 110 * current_scale, 30 * current_scale, 10 * current_scale };
    SDL_Rect y4 = { 90 * current_scale, 110 * current_scale, 10 * current_scale, 30 * current_scale };
    SDL_RenderFillRect(renderer, &y1); SDL_RenderFillRect(renderer, &y2);
    SDL_RenderFillRect(renderer, &y3); SDL_RenderFillRect(renderer, &y4);

    SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
    SDL_Rect c1 = { 130 * current_scale, 80 * current_scale, 30 * current_scale, 10 * current_scale };
    SDL_Rect c2 = { 130 * current_scale, 80 * current_scale, 10 * current_scale, 60 * current_scale };
    SDL_Rect c3 = { 130 * current_scale, 130 * current_scale, 30 * current_scale, 10 * current_scale };
    SDL_RenderFillRect(renderer, &c1); SDL_RenderFillRect(renderer, &c2); SDL_RenderFillRect(renderer, &c3);

    SDL_RenderPresent(renderer);
    SDL_Delay(1500);
}

// =================================================================
// НАДІЙНЕ МЕНЮ ВИБОРУ ІГОР
// =================================================================
static std::string SelectRomFile() {
    char filename[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "NES ROM Files (*.nes)\0*.nes\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "YCsys BIOS - Select Game ROM";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) return -1;

    SDL_Window* window = SDL_CreateWindow("YCsys NES Core", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * current_scale, 240 * current_scale, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    // =================================================================
    // ПІДКЛЮЧЕННЯ ГЕЙМПАДА (XInput / DualSense / Switch)
    // =================================================================
    SDL_GameController* gameController = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            gameController = SDL_GameControllerOpen(i);
            if (gameController) {
                std::cout << "[YCsys] Геймпад підключено: " << SDL_GameControllerName(gameController) << std::endl;
                break; // Беремо перший знайдений геймпад
            }
        }
    }

    PlayBootAnimation(renderer);

    std::string romPath = SelectRomFile();
    if (romPath.empty()) {
        std::cout << "Гру не вибрано. Вихід..." << std::endl;
        SDL_Quit();
        return 0;
    }

    Bus* nes = new Bus();
    std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>(romPath);

    if (!cart->bImageValid) {
        std::cerr << "Помилка завантаження ROM!" << std::endl;
        return -1;
    }
    nes->insertCartridge(cart);
    nes->cart->reset();
    nes->cpu.reset();

    // =================================================================
    // НАЛАШТУВАННЯ АУДІО
    // =================================================================
    SDL_AudioSpec audio_spec{};
    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_F32SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 1024;
    audio_spec.callback = nullptr;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
    if (audio_device > 0) SDL_PauseAudioDevice(audio_device, 0);

    bool bQuit = false;
    SDL_Event event;

    double dAudioTime = 0.0;
    const double dAudioTimePerSystemClock = 1.0 / 5360520.0;
    const double dAudioTimePerSample = 1.0 / 44100.0;
    double dAudioSampleAccumulator = 0.0;
    int nAudioSampleCount = 0;

    float lpf_out1 = 0.0f;
    float lpf_out2 = 0.0f;
    const float lpf_cutoff = 0.12f;

    // Флаги управління системою
    bool bF5_Pressed = false;
    bool bF7_Pressed = false;
    bool bP_Pressed = false;
    bool bPlus_Pressed = false;
    bool bMinus_Pressed = false;

    bool bPaused = false;

    // Змінні "Машини часу"
    std::deque<GameState> rewindBuffer;
    const size_t MAX_REWIND_STATES = 150; // 5 секунд (при 30 знімках на секунду)
    int frame_counter = 0;

    while (!bQuit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) bQuit = true;
        }

        SDL_PumpEvents();
        const Uint8* state = SDL_GetKeyboardState(NULL);

        nes->controller[0] = 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_X] ? 0x80 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_Z] ? 0x40 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_A] ? 0x20 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_S] ? 0x10 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_UP] ? 0x08 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_DOWN] ? 0x04 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_LEFT] ? 0x02 : 0x00;
        nes->controller[0] |= state[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00;

        // =================================================================
        // ЧИТАННЯ ГЕЙМПАДА (Додаємо до сигналів клавіатури)
        // =================================================================
        if (gameController) {
            // Дозволяємо грати класичним хватом (Xbox X->B, Xbox A->A) або прямим
            bool pad_A = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_A) ||
                SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_B);
            bool pad_B = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_X) ||
                SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_Y);
            bool pad_Sel = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_BACK);
            bool pad_Sta = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_START);
            bool pad_Up = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_UP);
            bool pad_Dn = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            bool pad_Lf = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            bool pad_Rt = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

            // Підмішуємо кнопки геймпада побітовим "АБО", щоб можна було грати і тим, і тим
            nes->controller[0] |= pad_A ? 0x80 : 0x00;
            nes->controller[0] |= pad_B ? 0x40 : 0x00;
            nes->controller[0] |= pad_Sel ? 0x20 : 0x00;
            nes->controller[0] |= pad_Sta ? 0x10 : 0x00;
            nes->controller[0] |= pad_Up ? 0x08 : 0x00;
            nes->controller[0] |= pad_Dn ? 0x04 : 0x00;
            nes->controller[0] |= pad_Lf ? 0x02 : 0x00;
            nes->controller[0] |= pad_Rt ? 0x01 : 0x00;
        }

        // Керування збереженням на диск
        if (state[SDL_SCANCODE_F5]) {
            if (!bF5_Pressed) { SaveGameToDisk(nes); bF5_Pressed = true; }
        }
        else { bF5_Pressed = false; }

        if (state[SDL_SCANCODE_F7]) {
            if (!bF7_Pressed) { LoadGameFromDisk(nes); bF7_Pressed = true; }
        }
        else { bF7_Pressed = false; }

        // =================================================================
        // ЛОГІКА ІНТЕРФЕЙСУ (Пауза та Масштабування)
        // =================================================================
        if (state[SDL_SCANCODE_P]) {
            if (!bP_Pressed) {
                bPaused = !bPaused;
                bP_Pressed = true;
            }
        }
        else { bP_Pressed = false; }

        bool scale_changed = false;
        // Підтримка кнопок "+" (на основній клавіатурі та NumPad)
        if (state[SDL_SCANCODE_EQUALS] || state[SDL_SCANCODE_KP_PLUS]) {
            if (!bPlus_Pressed) {
                if (current_scale < 7) { current_scale++; scale_changed = true; }
                bPlus_Pressed = true;
            }
        }
        else { bPlus_Pressed = false; }

        // Підтримка кнопок "-" (на основній клавіатурі та NumPad)
        if (state[SDL_SCANCODE_MINUS] || state[SDL_SCANCODE_KP_MINUS]) {
            if (!bMinus_Pressed) {
                if (current_scale > 3) { current_scale--; scale_changed = true; }
                bMinus_Pressed = true;
            }
        }
        else { bMinus_Pressed = false; }

        if (scale_changed) {
            SDL_SetWindowSize(window, 256 * current_scale, 240 * current_scale);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }

        // =================================================================
        // ГОЛОВНИЙ ЦИКЛ ГРИ (Пауза / Перемотування / Нормальний хід)
        // =================================================================
        if (bPaused) {
            SDL_PauseAudioDevice(audio_device, 1); // Апаратно глушимо звук
            SDL_Delay(16); // Віддаємо ресурси системі, щоб не гріти процесор
        }
        else {
            SDL_PauseAudioDevice(audio_device, 0); // Повертаємо звук

            bool bRewinding = state[SDL_SCANCODE_BACKSPACE];

            if (bRewinding) {
                if (!rewindBuffer.empty()) {
                    RestoreState(nes, rewindBuffer.back());
                    rewindBuffer.pop_back();
                    SDL_Delay(16);
                }
            }
            else {
                while (!nes->ppu.frame_complete) {
                    nes->clock();

                    dAudioSampleAccumulator += nes->apu.GetOutputSample();
                    nAudioSampleCount++;
                    dAudioTime += dAudioTimePerSystemClock;

                    if (dAudioTime >= dAudioTimePerSample) {
                        dAudioTime -= dAudioTimePerSample;
                        float raw_sample = 0.0f;
                        if (nAudioSampleCount > 0) {
                            raw_sample = static_cast<float>(dAudioSampleAccumulator / nAudioSampleCount);
                        }

                        lpf_out1 += lpf_cutoff * (raw_sample - lpf_out1);
                        lpf_out2 += lpf_cutoff * (lpf_out1 - lpf_out2);
                        float final_sample = lpf_out2;

                        SDL_QueueAudio(audio_device, &final_sample, sizeof(float));
                        dAudioSampleAccumulator = 0.0;
                        nAudioSampleCount = 0;
                    }
                }
                nes->ppu.frame_complete = false;

                frame_counter++;
                if (frame_counter % 2 == 0) {
                    CaptureState(nes, rewindBuffer, MAX_REWIND_STATES);
                }
            }
        }

        // Рендеринг екрана працює незалежно від того, граємо ми, мотаємо час чи стоїмо на паузі
        SDL_UpdateTexture(texture, nullptr, nes->ppu.sprScreen.data(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
	if (gameController) SDL_GameControllerClose(gameController);
    if (audio_device > 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete nes;
    return 0;
}