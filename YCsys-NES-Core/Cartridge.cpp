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
 * |      [M]  YCsys NES CORE - CARTRIDGE & MAPPER IMPLEMENTATION [M]        |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [ROM]        [PRG]        [CHR]        [MAP]        [ROM]
 */

#include "Cartridge.h"
#include <iostream>

Cartridge::Cartridge(const std::string& sFileName) {
    // Відкриваємо файл у бінарному режимі
    std::ifstream ifs;
    ifs.open(sFileName, std::ifstream::binary);

    if (ifs.is_open()) {
        // 1. Читаємо заголовок (точно 16 байт)
        ifs.read((char*)&header, sizeof(sHeader));

        // 2. ПЕРЕВІРКА СИГНАТУРИ
        if (header.name[0] != 'N' || header.name[1] != 'E' ||
            header.name[2] != 'S' || header.name[3] != 0x1A) {
            std::cerr << "Помилка: Файл не має формату iNES!" << std::endl;
            ifs.close();
            return; // Виходимо, bImageValid залишиться false
        }

        // 3. Пропуск "Trainer" (сміття на 512 байт, якщо прапорець встановлений)
        if (header.mapper1 & 0x04) {
            ifs.seekg(512, std::ios_base::cur);
        }

        // Визначаємо ID мапера (склеюємо старші 4 біти з mapper2 та mapper1)
        nMapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);

        // Визначаємо кількість банків
        nPRGBanks = header.prg_rom_chunks;
        nCHRBanks = header.chr_rom_chunks;

        // 4. Читаємо пам'ять PRG (код гри - банки по 16 КБ)
        vPRGMemory.resize(nPRGBanks * 16384);
        ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

        // 5. Читаємо пам'ять CHR (графіка гри - банки по 8 КБ)
        if (nCHRBanks == 0) {
            // Якщо банків 0, гра використовує CHR RAM (створюємо пусті 8 КБ для роботи PPU)
            vCHRMemory.resize(8192);
        }
        else {
            vCHRMemory.resize(nCHRBanks * 8192);
            ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
        }

        bImageValid = true; // Успіх!
        ifs.close();
    }
    else {
        std::cerr << "Помилка: Не вдалося відкрити файл " << sFileName << std::endl;
    }
}

Cartridge::~Cartridge() {}