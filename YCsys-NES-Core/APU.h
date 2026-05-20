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

#pragma once
#include <cstdint>

class APU {
public:
    APU();
    ~APU();

    // Зв'язок з CPU шиною
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    // Системний годинник APU
    void clock();

    // Отримання поточного змішаного звукового сигналу (для мікшера)
    double GetOutputSample() ;

private:
    double global_time = 0.0; // Тимчасовий акумулятор часу для хвильових функцій
    uint32_t clock_counter = 0; // Системний лічильник тактів APU

    // Структура генератора прямокутної хвилі (Pulse)
    struct Pulse {
        uint8_t duty = 0;       // Робочий цикл (0-3)
        uint16_t timer = 0;     // 11-бітний таймер (частота)
        uint8_t volume = 0;     // Гучність (0-15)
        bool enabled = false;   // Чи увімкнений канал

        // Внутрішні лічильники
        uint16_t timer_value = 0;
        uint8_t duty_step = 0;

        uint8_t clock() {
            // Звук генерується лише якщо канал увімкнено і частота не надто висока
            if (!enabled || timer < 8) return 0;

            timer_value--;
            if (timer_value == 0xFFFF) { // Переповнення таймера (відлік вниз)
                timer_value = timer;
                duty_step = (duty_step + 1) & 0x07; // Зсуваємо секвенсор (0..7)
            }

            // ФІКС: Правильний розмір масиву 4x8
            static const uint8_t duty_table[4][8] = {
                {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
                {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
                {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
                {1, 0, 0, 1, 1, 1, 1, 1}  // 75%
            };

            // Якщо поточний крок містить 1 — видаємо гучність, інакше 0
            return duty_table[duty][duty_step] ? volume : 0;
        }
    };

    // Два звукових канали
    Pulse pulse1;
    Pulse pulse2;
    uint8_t pulse1_sample = 0;
    uint8_t pulse2_sample = 0;
};