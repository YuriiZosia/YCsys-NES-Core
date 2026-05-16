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
 * |      [#]  YCsys NES CORE - SYSTEM BUS & RAM IMPLEMENTATION  [#]         |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [|||]        [|||]        [BUS]        [|||]        [|||]
 */

#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "CPU.h"
#include "Cartridge.h"

class Bus {
public:
    Bus();
    ~Bus();

public: // Пристрої на шині
    CPU6502 cpu;
    std::array<uint8_t, 2048> cpuRam;

    // Вказівник на підключений картридж
    std::shared_ptr<Cartridge> cart;

public: // Управління системою
    // Метод для вставлення картриджа
    void insertCartridge(const std::shared_ptr<Cartridge>& cartridge);

public: // Читання та запис
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);
};