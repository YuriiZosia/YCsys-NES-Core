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
 * |      [A]  YCsys NES CORE - AUDIO PROCESSING UNIT (APU)     [A]          |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [SND]        [PUL]        [TRI]        [NSI]        [DAC]        [SND]
 */

#include "APU.h"

APU::APU() {}
APU::~APU() {}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        // ==========================================
        // КАНАЛ PULSE 1 (Квадратна хвиля 1)
        // ==========================================
    case 0x4000:
        pulse1.duty = (data & 0xC0) >> 6; // Верхні 2 біти - робочий цикл
        pulse1.volume = data & 0x0F;      // Нижні 4 біти - гучність
        break;
    case 0x4001: // Регістр Sweep (Поки ігноруємо)
        break;
    case 0x4002:
        // Нижні 8 бітів таймера
        pulse1.timer = (pulse1.timer & 0xFF00) | data;
        break;
    case 0x4003:
        // Верхні 3 біти таймера + скидання секвенсора (безпечний каст)
        pulse1.timer = (pulse1.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        pulse1.duty_step = 0;
        break;

        // ==========================================
        // КАНАЛ PULSE 2 (Квадратна хвиля 2)
        // ==========================================
    case 0x4004:
        pulse2.duty = (data & 0xC0) >> 6;
        pulse2.volume = data & 0x0F;
        break;
    case 0x4005: // Регістр Sweep (Поки ігноруємо)
        break;
    case 0x4006:
        pulse2.timer = (pulse2.timer & 0xFF00) | data;
        break;
    case 0x4007:
        pulse2.timer = (pulse2.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        pulse2.duty_step = 0;
        break;

        // ==========================================
        // СТАТУС ТА КЕРУВАННЯ APU
        // ==========================================
    case 0x4015:
        // Біт 0 вмикає/вимикає Pulse 1, Біт 1 - Pulse 2
        pulse1.enabled = data & 0x01;
        pulse2.enabled = data & 0x02;
        break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    (void)addr;
    uint8_t data = 0x00;
    // Логіка читання статус-регістра APU ($4015) буде тут
    return data;
}

void APU::clock() {
    // Генератори Pulse в NES тактуються кожен 2-й системний крок процесора
    if (clock_counter % 2 == 0) {
        pulse1_sample = pulse1.clock();
        pulse2_sample = pulse2.clock();
    }
    clock_counter++;
}

double APU::GetOutputSample() {
// Сигнал кожного каналу може бути від 0 до 15. Максимальна сума = 30.
    // Щоб звук був без тріску (DC offset), нам треба відцентрувати його від -0.5 до +0.5
    // Замість того, щоб сигнал був [0.0 ... 1.0], робимо зміщення.
    double mixed = (static_cast<double>(pulse1_sample + pulse2_sample)) / 30.0;
    
    // Якщо змішаний сигнал == 0 (тиша), повертаємо 0, щоб не гуділи динаміки.
    if (mixed == 0.0) return 0.0;

    return mixed - 0.5;
}