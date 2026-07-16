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
 * |      [004] YCsys NES CORE - MAPPER 004 (MMC3) IMPLEMENTATION [004]      |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [MMC3]        [IRQ ]        [SCAN]        [BANK]        [MMC3]
 */

#include "Mapper_004.h"

Mapper_004::Mapper_004(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    reset();
}

Mapper_004::~Mapper_004() {}

void Mapper_004::reset() {
    nTargetRegister = 0;
    bPRGBankMode = false;
    bCHRInversion = false;
    bIRQActive = false;
    bIRQEnable = false;
    bIRQUpdate = false;
    nIRQCounter = 0;
    nIRQLatch = 0;

    for (int i = 0; i < 8; i++) pRegister[i] = 0;
    for (int i = 0; i < 8; i++) pCHRBank[i] = 0;

    // Ініціалізація банків PRG (у MMC3 вікна по 8 КБ, тому множимо кількість 16КБ банків на 2)
    pPRGBank[0] = 0;
    pPRGBank[1] = 1 * 0x2000;
    pPRGBank[2] = (nPRGBanks * 2 - 2) * 0x2000; // Передостанній банк
    pPRGBank[3] = (nPRGBanks * 2 - 1) * 0x2000; // Останній банк
}

bool Mapper_004::cpuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x8000 && addr <= 0x9FFF) mapped_addr = pPRGBank[0] + (addr & 0x1FFF);
    else if (addr >= 0xA000 && addr <= 0xBFFF) mapped_addr = pPRGBank[1] + (addr & 0x1FFF);
    else if (addr >= 0xC000 && addr <= 0xDFFF) mapped_addr = pPRGBank[2] + (addr & 0x1FFF);
    else if (addr >= 0xE000 && addr <= 0xFFFF) mapped_addr = pPRGBank[3] + (addr & 0x1FFF);
    else return false;
    return true;
}

bool Mapper_004::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0x9FFF) {
        if (!(addr & 0x0001)) {
            // Парна адреса: Вибір регістра
            nTargetRegister = data & 0x07;
            bPRGBankMode = (data & 0x40);
            bCHRInversion = (data & 0x80);
        }
        else {
            // Непарна адреса: Запис даних у вибраний регістр
            pRegister[nTargetRegister] = data;

            // Оновлення маршрутизації CHR (графіки)
            if (bCHRInversion) {
				pCHRBank[0] = pRegister[2] * 0x0400;// множення потрібно для отримання адреси в байтах, оскільки кожен банк 1 КБ (0x0400 байт)
                pCHRBank[1] = pRegister[3] * 0x0400;
                pCHRBank[2] = pRegister[4] * 0x0400;
                pCHRBank[3] = pRegister[5] * 0x0400;
                pCHRBank[4] = (pRegister[0] & 0xFE) * 0x0400;
                pCHRBank[5] = pCHRBank[4] + 0x0400;
                pCHRBank[6] = (pRegister[1] & 0xFE) * 0x0400;
                pCHRBank[7] = pCHRBank[6] + 0x0400;
            }
            else {
                pCHRBank[0] = (pRegister[0] & 0xFE) * 0x0400;
                pCHRBank[1] = pCHRBank[0] + 0x0400;
                pCHRBank[2] = (pRegister[1] & 0xFE) * 0x0400;
                pCHRBank[3] = pCHRBank[2] + 0x0400;
                pCHRBank[4] = pRegister[2] * 0x0400;
                pCHRBank[5] = pRegister[3] * 0x0400;
                pCHRBank[6] = pRegister[4] * 0x0400;
                pCHRBank[7] = pRegister[5] * 0x0400;
            }

            // Оновлення маршрутизації PRG (коду)
            uint32_t num_8k_banks = nPRGBanks * 2;
            if (bPRGBankMode) {
                pPRGBank[2] = (pRegister[6] & 0x3F) * 0x2000;
                pPRGBank[0] = (num_8k_banks - 2) * 0x2000;
            }
            else {
                pPRGBank[0] = (pRegister[6] & 0x3F) * 0x2000;
                pPRGBank[2] = (num_8k_banks - 2) * 0x2000;
            }
            pPRGBank[1] = (pRegister[7] & 0x3F) * 0x2000;
            pPRGBank[3] = (num_8k_banks - 1) * 0x2000;
        }
    }
    // Керування віддзеркаленням екрану та PRG RAM (ігноруємо для базового сетапу)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!(addr & 0x0001)) {
            // Парна адреса: Зміна віддзеркалення! (Біт 0: 0 = Vertical, 1 = Horizontal)
            nMirrorMode = data & 0x01;
        }
        // Непарна адреса керує PRG RAM (захист від запису), ми поки це пропускаємо
    }
    // Керування IRQ (Таймер рядків)
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (!(addr & 0x0001)) nIRQLatch = data;
        else bIRQUpdate = true;
    }
    else if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (!(addr & 0x0001)) {
            bIRQEnable = false;
            bIRQActive = false;
        }
        else {
            bIRQEnable = true;
        }
    }
    return false;
}

bool Mapper_004::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Вікно ділиться на шматки по 1 КБ (0x0400)
        uint16_t bank_index = addr / 0x0400;
        uint16_t offset = addr % 0x0400;
        mapped_addr = pCHRBank[bank_index] + offset;
        return true;
    }
    return false;
}

bool Mapper_004::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
    return false; // Чіп CHR ROM лише для читання
}

// ==========================================================
// ЛОГІКА IRQ (ПЕРЕРИВАНЬ)
// ==========================================================
void Mapper_004::scanline() {
    if (nIRQCounter == 0 || bIRQUpdate) {
        nIRQCounter = nIRQLatch;
        bIRQUpdate = false;
    }
    else {
        nIRQCounter--;
    }
    if (nIRQCounter == 0 && bIRQEnable) {
        bIRQActive = true;
    }
}

bool Mapper_004::irqState() {
    return bIRQActive;
}

void Mapper_004::irqClear() {
    bIRQActive = false;
}

bool Mapper_004::mirrorMode(uint8_t& mode) {
    mode = nMirrorMode ? 3 : 2; // 0(vertical)->2, 1(horizontal)->3
    return true; // MMC3 підтримує динамічне дзеркало!
}