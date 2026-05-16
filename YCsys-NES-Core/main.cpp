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
#include <iomanip>
#include <memory>
#include <cstdio> // Для роботи з файлами (FILE)
#include "Bus.h"
#include "CPU.h"
#include "Cartridge.h"

// Це виправить твою помилку SDL_main
#define SDL_MAIN_HANDLED // це говорить SDL, що ми не хочемо, щоб він замінював нашу функцію main() на свою власну версію, яка викликається при запуску програми. Це корисно, коли ти хочеш мати повний контроль над точкою входу в програму і не хочеш, щоб SDL втручався в це.

int main(int argc, char* argv[]) {
    Bus nes;

    std::cout << "YCsys NES Core: System Initialized." << std::endl;
    std::cout << "Loading nestest.nes..." << std::endl;

    std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>("nestest.nes");

    if (!cart->bImageValid) {
        std::cerr << "Не вдалося завантажити nestest.nes. Перевір шлях до файлу!" << std::endl;
        return -1;
    }

    nes.insertCartridge(cart);

    // ОЖИВЛЯЄМО СИСТЕМУ
    nes.cpu.reset();

    // Налаштування під стандарти nestest
    nes.cpu.pc = 0xC000;
    nes.cpu.status = 0x24; // Встановлюємо прапорці в еталонний стартовий стан

    // Відкриваємо файл для запису нашого логу
    FILE* logfile = nullptr;
    fopen_s(&logfile, "yc_nestest_output.log", "w");
    if (!logfile) {
        std::cerr << "Помилка: Не вдалося створити файл логу!" << std::endl;
        return -1;
    }

    std::cout << "Starting nestest execution (8991 steps)..." << std::endl;
    std::cout << "Writing to yc_nestest_output.log..." << std::endl;

    // ЦИКЛ ТЕСТУВАННЯ (Проходимо всі 8991 інструкцію тесту)
    int steps = 0;
    while (steps < 8991) {
        if (nes.cpu.cycles == 0) {
            // Форматуємо рядок. Точно 44 пробіли між PC та A!
            char buffer[128];
            sprintf_s(buffer, "%04X                                        A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
                nes.cpu.pc, nes.cpu.a, nes.cpu.x, nes.cpu.y, nes.cpu.status, nes.cpu.stkp);

            // Пишемо у файл
            fprintf(logfile, "%s", buffer);

            // Для консолі виводимо лише кожен 1000-й крок, щоб показати прогрес
            if (steps % 1000 == 0) {
                std::cout << "Step " << steps << " completed..." << std::endl;
            }

            steps++;
        }
        nes.cpu.clock();
    }

    fclose(logfile);
    std::cout << "Test execution finished! Log saved to yc_nestest_output.log" << std::endl;
    std::cout << "YCsys: Code that works, not just exists." << std::endl;

    return 0;
}