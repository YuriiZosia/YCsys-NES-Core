/*
 *  _________________________________________________________________________
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

 // Конструктор ініціалізує заголовок нулями через список ініціалізації
Cartridge::Cartridge(const std::string& sFileName) : header{ 0 } {
    // Створюємо потік для читання файлу в бінарному режимі
    std::ifstream ifs;
    ifs.open(sFileName, std::ifstream::binary);

    if (ifs.is_open()) {
        // 1. Читаємо перші 16 байт файлу прямо в нашу структуру заголовка
        ifs.read((char*)&header, sizeof(sHeader));

        // 2. Перевірка сигнатури (магічне число формату iNES)
        // Якщо перші байти не "NES" і символ EOF (0x1A), це не гра для нашого емулятора
        if (header.name[0] != 'N' || header.name[1] != 'E' ||
            header.name[2] != 'S' || header.name[3] != 0x1A) {
            std::cerr << "Помилка: Файл не має формату iNES!" << std::endl;
            ifs.close();
            return;
        }

        // 3. Перевірка на наявність "Trainer" (сміттєві дані для старих копіювальників)
        // Якщо 3-й біт (0x04) у mapper1 встановлений, пропускаємо 512 байт трейнера
        if (header.mapper1 & 0x04) {
            ifs.seekg(512, std::ios_base::cur);
        }

        // 4. Визначення ID мапера
        // Мапер ID розбитий на два байти у заголовку. Ми беремо старші 4 біти з кожного
        // і склеюємо їх у повноцінне 8-бітне число.
        nMapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);

        // Визначаємо тип віддзеркалення (біт 0 регістра mapper1)
        mirror = (header.mapper1 & 0x01) ? MIRROR::VERTICAL : MIRROR::HORIZONTAL;

        // 5. Витягуємо кількість банків пам'яті
        nPRGBanks = header.prg_rom_chunks; // Банки програмного коду
        nCHRBanks = header.chr_rom_chunks; // Банки графіки

        // 6. Завантаження PRG-ROM (Програмна пам'ять)
        // Кожен банк PRG має розмір 16 КБ (16384 байт). Виділяємо місце у векторі.
		vPRGMemory.resize(nPRGBanks * 16384ULL); // ULL - це суфікс для позначення константи як unsigned long long, щоб уникнути проблем з переповненням при великих розмірах.
        ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

        // 7. Завантаження CHR-ROM / CHR-RAM (Графічна пам'ять)
        if (nCHRBanks == 0) {
            // Якщо банків CHR немає, картридж використовує CHR-RAM (оперативку на платі).
            // Створюємо порожні 8 КБ, куди гра сама буде записувати свої тайли.
            vCHRMemory.resize(8192);
        }
        else {
            // Якщо банки є, виділяємо пам'ять (по 8 КБ на банк) і читаємо дані з файлу.
            vCHRMemory.resize(nCHRBanks * 8192ULL);
            ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
        }

        // 8. ВСТАНОВЛЮЄМО МАПЕР
        // Залежно від ID, створюємо відповідний об'єкт мапера і передаємо йому кількість банків
        switch (nMapperID) {
        case 0: pMapper = std::make_shared<Mapper_000>(nPRGBanks, nCHRBanks); break;
		case 1: pMapper = std::make_shared<Mapper_001>(nPRGBanks, nCHRBanks); break;
		case 2: pMapper = std::make_shared<Mapper_002>(nPRGBanks, nCHRBanks); break;
            // У майбутньому тут будуть інші мапери (case 1:, case 2: і т.д.)
        }

        // Встановлюємо прапорець успішного завантаження
        bImageValid = true;
        ifs.close();
    }
    else {
        std::cerr << "Помилка: Не вдалося відкрити файл " << sFileName << std::endl;
    }
}

Cartridge::~Cartridge() {}

// ==========================================================
// МАРШРУТИЗАЦІЯ ДЛЯ CPU (Процесор)
// ==========================================================

bool Cartridge::cpuRead(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;
    // Питаємо мапер: "Чи є за цією адресою щось цікаве для CPU?"
    if (pMapper->cpuMapRead(addr, mapped_addr)) {
        // Якщо так, мапер повертає фізичну адресу (mapped_addr), і ми читаємо з масиву
        data = vPRGMemory[mapped_addr];
        return true;
    }
    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    // Питаємо мапер: "Чи дозволяєш ти запис за цією адресою?"
    // ФІКС: Передаємо data всередину мапера!
    if (pMapper->cpuMapWrite(addr, mapped_addr, data)) {
        // Якщо так, пишемо дані (зазвичай використовується для перемикання банків)
        vPRGMemory[mapped_addr] = data;
        return true;
    }
    return false;
}

// ==========================================================
// МАРШРУТИЗАЦІЯ ДЛЯ PPU (Відеопроцесор)
// ==========================================================

bool Cartridge::ppuRead(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;
    // Відеочіп просить графіку (спрайти/фон). Питаємо мапер, де вони лежать фізично.
    if (pMapper->ppuMapRead(addr, mapped_addr)) {
        data = vCHRMemory[mapped_addr];
        return true;
    }
    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    // Гра намагається змінити графіку. Це спрацює лише для CHR-RAM, де nCHRBanks == 0.
    if (pMapper->ppuMapWrite(addr, mapped_addr)) {
        vCHRMemory[mapped_addr] = data;
        return true;
    }
    return false;
}

void Cartridge::reset() {
    if (pMapper) {
        pMapper->reset();
    }
}