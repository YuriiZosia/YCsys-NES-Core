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
#include <deque>  // Для кільцевого буфера "Машини часу"
#include <vector>
#include <array>
#include <algorithm>

#pragma warning(push)
#pragma warning(disable: 26819)
#pragma warning(disable: 26451)
#include <SDL.h>
#pragma warning(pop)            

#include "Bus.h"

const int SCALE = 3;

// =================================================================
// СИСТЕМА МАШИНИ ЧАСУ (REWIND RING BUFFER)
// =================================================================

// Структура для зберігання одного мікро-знімка системи
struct GameState {
    // Ініціалізуємо всі змінні нулями, щоб задовольнити суворі перевірки (type.6)
    uint8_t cpu_a = 0;
    uint8_t cpu_x = 0;
    uint8_t cpu_y = 0;
    uint8_t cpu_stkp = 0;
    uint8_t cpu_status = 0;
    uint16_t cpu_pc = 0;

    std::array<uint8_t, 2048> cpuRam{};
    std::vector<uint8_t> prgRam;

    // ФІКС: Використовуємо динамічний вектор замість статичного масиву. 
    // Це переносить 245 КБ даних у Heap і рятує стек від переповнення!
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

    // Копіюємо дані масиву sprScreen у динамічний вектор
    state.screen.assign(nes->ppu.sprScreen.begin(), nes->ppu.sprScreen.end());

    buffer.push_back(std::move(state));

    // Якщо перевищили ліміт пам'яті (наприклад, 5 секунд) — видаляємо найстаріший кадр
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

    // Безпечно повертаємо зображення з вектора назад у масив PPU
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
        SDL_Rect line = { (128 - i) * SCALE, 119 * SCALE, (i * 2) * SCALE, 2 * SCALE };
        SDL_RenderFillRect(renderer, &line);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }
    for (int i = 0; i < 120; i += 6) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 200, 255, 255, 255);
        SDL_Rect rect = { 0, (120 - i) * SCALE, 256 * SCALE, (i * 2) * SCALE };
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
    SDL_Rect y1 = { 80 * SCALE, 80 * SCALE, 10 * SCALE, 30 * SCALE };
    SDL_Rect y2 = { 100 * SCALE, 80 * SCALE, 10 * SCALE, 30 * SCALE };
    SDL_Rect y3 = { 80 * SCALE, 110 * SCALE, 30 * SCALE, 10 * SCALE };
    SDL_Rect y4 = { 90 * SCALE, 110 * SCALE, 10 * SCALE, 30 * SCALE };
    SDL_RenderFillRect(renderer, &y1); SDL_RenderFillRect(renderer, &y2);
    SDL_RenderFillRect(renderer, &y3); SDL_RenderFillRect(renderer, &y4);

    SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
    SDL_Rect c1 = { 130 * SCALE, 80 * SCALE, 30 * SCALE, 10 * SCALE };
    SDL_Rect c2 = { 130 * SCALE, 80 * SCALE, 10 * SCALE, 60 * SCALE };
    SDL_Rect c3 = { 130 * SCALE, 130 * SCALE, 30 * SCALE, 10 * SCALE };
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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return -1;

    SDL_Window* window = SDL_CreateWindow("YCsys NES Core", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * SCALE, 240 * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

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

    bool bF5_Pressed = false;
    bool bF7_Pressed = false;

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
        // ЛОГІКА МАШИНИ ЧАСУ (Відмотування)
        // =================================================================
        bool bRewinding = state[SDL_SCANCODE_BACKSPACE];

        if (bRewinding) {
            if (!rewindBuffer.empty()) {
                // Витягуємо останній кадр і завантажуємо його
                RestoreState(nes, rewindBuffer.back());
                rewindBuffer.pop_back();

                // Штучна затримка, щоб ми бачили плавний рух назад (а не пролетіли 5 секунд за мить)
                SDL_Delay(16);
            }
            // ВАЖЛИВО: під час перемотування ми ПРОПУСКАЄМО виконання інструкцій процесора і звуку!
        }
        else {
            // ЗВИЧАЙНИЙ РЕЖИМ ГРИ
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

                    // Low-Pass фільтр
                    lpf_out1 += lpf_cutoff * (raw_sample - lpf_out1);
                    lpf_out2 += lpf_cutoff * (lpf_out1 - lpf_out2);
                    float final_sample = lpf_out2;

                    SDL_QueueAudio(audio_device, &final_sample, sizeof(float));
                    dAudioSampleAccumulator = 0.0;
                    nAudioSampleCount = 0;
                }
            }
            nes->ppu.frame_complete = false;

            // Зберігаємо поточний стан у кільцевий буфер кожен другий кадр (30 FPS для знімків)
            frame_counter++;
            if (frame_counter % 2 == 0) {
                CaptureState(nes, rewindBuffer, MAX_REWIND_STATES);
            }
        }

        // Оновлюємо текстуру (працює і для звичайної гри, і для перемотування!)
        SDL_UpdateTexture(texture, nullptr, nes->ppu.sprScreen.data(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    if (audio_device > 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete nes;
    return 0;
}