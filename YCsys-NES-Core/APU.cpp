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
 * |      [A]  YCsys NES CORE - AUDIO PROCESSING UNIT (APU)     [A]          |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [SND]        [PUL]        [TRI]        [NSI]        [DAC]        [SND]
 */

#include "APU.h"
#include "Bus.h"

APU::APU() {}
APU::~APU() {}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        // ==========================================
        // PULSE 1
        // ==========================================
    case 0x4000:
        pulse1.duty = (data & 0xC0) >> 6;
        pulse1.halt = (data & 0x20) != 0;
        pulse1.env.disable = (data & 0x10) != 0;
        pulse1.env.volume = data & 0x0F;
        break;
    case 0x4001:
        pulse1.sweep.enabled = (data & 0x80) != 0;
        pulse1.sweep.period = (data & 0x70) >> 4;
        pulse1.sweep.down = (data & 0x08) != 0;
        pulse1.sweep.shift = data & 0x07;
        pulse1.sweep.reload = true;
        break;
    case 0x4002:
        pulse1.timer = (pulse1.timer & 0xFF00) | data;
        break;
    case 0x4003:
        pulse1.timer = (pulse1.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        pulse1.duty_step = 0;
        pulse1.env.start = true; // Рестарт огинаючої
        if (pulse1.enabled) pulse1.length_counter = length_table[data >> 3];
        break;

        // ==========================================
        // PULSE 2
        // ==========================================
    case 0x4004:
        pulse2.duty = (data & 0xC0) >> 6;
        pulse2.halt = (data & 0x20) != 0;
        pulse2.env.disable = (data & 0x10) != 0;
        pulse2.env.volume = data & 0x0F;
        break;
    case 0x4005:
        pulse2.sweep.enabled = (data & 0x80) != 0;
        pulse2.sweep.period = (data & 0x70) >> 4;
        pulse2.sweep.down = (data & 0x08) != 0;
        pulse2.sweep.shift = data & 0x07;
        pulse2.sweep.reload = true;
        break;
    case 0x4006:
        pulse2.timer = (pulse2.timer & 0xFF00) | data;
        break;
    case 0x4007:
        pulse2.timer = (pulse2.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        pulse2.duty_step = 0;
        pulse2.env.start = true;
        if (pulse2.enabled) pulse2.length_counter = length_table[data >> 3];
        break;

        // ==========================================
        // TRIANGLE
        // ==========================================
    case 0x4008:
        triangle.halt = (data & 0x80) != 0;
        triangle.linear_reload = data & 0x7F; // Завантаження лінійного лічильника
        break;
    case 0x400A:
        triangle.timer = (triangle.timer & 0xFF00) | data;
        break;
    case 0x400B:
        triangle.timer = (triangle.timer & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
        triangle.linear_reload_flag = true; // Взводимо прапорець перезавантаження
        if (triangle.enabled) triangle.length_counter = length_table[data >> 3];
        break;

        // ==========================================
        // NOISE
        // ==========================================
    case 0x400C:
        noise.halt = (data & 0x20) != 0;
        noise.env.disable = (data & 0x10) != 0;
        noise.env.volume = data & 0x0F;
        break;
    case 0x400E: {
        static const uint16_t noise_periods[16] = {
            4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
        };
        noise.timer = noise_periods[data & 0x0F];
		noise.mode = (data & 0x80) != 0; // Режим генерації шуму (звичайний або з коротким циклом). Зчитуємо 7-й біт (Mode)
        break;
    }
    case 0x400F:
        noise.env.start = true;
        if (noise.enabled) noise.length_counter = length_table[data >> 3];
        break;

        // ==========================================
        // DMC (Delta Modulation Channel)
        // ==========================================
    case 0x4010:
        dmc.irq = (data & 0x80) != 0;
        dmc.loop = (data & 0x40) != 0;
        dmc.tick_period = data & 0x0F; // Індекс частоти дискретизації
        break;
    case 0x4011:
        // Пряме завантаження рівня виходу (7 біт)
        dmc.output_level = data & 0x7F;
        break;
    case 0x4012:
        // Стартова адреса семплу: $C000 + (data * 64)
        dmc.sample_address = 0xC000 + (static_cast<uint16_t>(data) * 64);
        break;
    case 0x4013:
        // Довжина семплу: (data * 16) + 1
        dmc.sample_length = (static_cast<uint16_t>(data) * 16) + 1;
        break;

        // ==========================================
        // СТАТУС ТА СЕКВЕНСОР
        // ==========================================
    case 0x4015:
        pulse1.enabled = (data & 0x01) > 0;
        if (!pulse1.enabled) pulse1.length_counter = 0;

        pulse2.enabled = (data & 0x02) > 0;
        if (!pulse2.enabled) pulse2.length_counter = 0;

        triangle.enabled = (data & 0x04) > 0;
        if (!triangle.enabled) triangle.length_counter = 0;

        // ФІКС: Повертаємо ШУМ на законне місце!
        noise.enabled = (data & 0x08) > 0;
        if (!noise.enabled) noise.length_counter = 0;
        
        dmc.enabled = (data & 0x10) > 0;
        if (!dmc.enabled) {
            dmc.current_length = 0;
        }
        else {
            if (dmc.current_length == 0) {
                dmc.current_address = dmc.sample_address;
                dmc.current_length = dmc.sample_length;
                // ФІКС: Одразу просимо перший семпл у шини!
                if (pBus != nullptr) {
                    dmc.fetch_sample(pBus);
                }
            }
        }
        break;

    case 0x4017:
        frame_mode = (data & 0x80) ? 1 : 0;
        // Якщо біт 7 встановлено, негайно тактуємо чверть- і напівкадр
        if (frame_mode == 1) {
            clock_quarter_frame();
            clock_half_frame();
        }
        break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    (void)addr;
    uint8_t data = 0x00;
    // (Логіка 4015 буде тут пізніше, якщо потрібно для конкретних ігор)
    return data;
}

// ---------------------------------------------------------
// ДОПОМІЖНІ МЕТОДИ СЕКВЕНСОРА
// ---------------------------------------------------------
void APU::clock_quarter_frame() {
    pulse1.env.clock(pulse1.halt);
    pulse2.env.clock(pulse2.halt);
    noise.env.clock(noise.halt);
    triangle.clock_linear(); // Лінійний лічильник трикутника тікає тут
}

void APU::clock_half_frame() {
    if (!pulse1.halt && pulse1.length_counter > 0) pulse1.length_counter--;
    if (!pulse2.halt && pulse2.length_counter > 0) pulse2.length_counter--;
    if (!triangle.halt && triangle.length_counter > 0) triangle.length_counter--;
    if (!noise.halt && noise.length_counter > 0) noise.length_counter--;

    pulse1.sweep.clock(pulse1.timer, true);
    pulse2.sweep.clock(pulse2.timer, false);
}

// ---------------------------------------------------------
// ГОЛОВНИЙ ГОДИННИК APU
// ---------------------------------------------------------
void APU::clock() {
    // 1. Апаратне тактування каналів
    triangle_sample = triangle.clock();
    if (clock_counter % 2 == 0) {
        pulse1_sample = pulse1.clock();
        pulse2_sample = pulse2.clock();
        noise_sample = noise.clock();
		dmc_sample = dmc.clock(pBus);
    }

    // 2. Секвенсор кадрів (керує оболонками та довжинами кожні ~7457 тактів)
    if (clock_counter % 7457 == 0) {
        if (frame_mode == 0) {
            // 4-кроковий режим
            frame_clock_counter = (frame_clock_counter + 1) % 4;
            clock_quarter_frame();
            if (frame_clock_counter == 1 || frame_clock_counter == 3) {
                clock_half_frame();
            }
        }
        else {
            // 5-кроковий режим
            frame_clock_counter = (frame_clock_counter + 1) % 5;
            if (frame_clock_counter != 4) { // На 4-му кроці нічого не відбувається
                clock_quarter_frame();
            }
            if (frame_clock_counter == 1 || frame_clock_counter == 3) {
                clock_half_frame();
            }
        }
    }

    clock_counter++;
}

// ---------------------------------------------------------
// ЛОГІКА КАНАЛУ DMC (Delta Modulation Channel)
// ---------------------------------------------------------
void APU::DMC::fetch_sample(Bus* bus) {
    if (!has_sample && current_length > 0) {
        // Читаємо байт напряму з оперативної пам'яті гри (DMA)
        sample_buffer = bus->read(current_address, true);
        has_sample = true;

        // Інкрементуємо адресу з обгортанням (за апаратним правилом 6502)
        current_address++;
        if (current_address == 0xFFFF) {
            current_address = 0x8000;
        }

        current_length--;

        // Якщо семпл закінчився
        if (current_length == 0) {
            if (loop) {
                // Перезапускаємо семпл
                current_address = sample_address;
                current_length = sample_length;
            }
            else if (irq) {
                // В майбутньому тут можна кидати bus->cpu.irq()
            }
        }
    }
}

uint8_t APU::DMC::clock(Bus* bus) {
    if (!enabled) return output_level;

    if (tick_value > 0) {
        tick_value--;
    }

    if (tick_value == 0) {
        // Скидаємо таймер згідно з поточним індексом
        tick_value = rate_table[tick_period & 0x0F];

        // Якщо 8 біт закінчилися, починаємо новий цикл
        if (bit_count == 0) {
            bit_count = 8;
            if (has_sample) {
                silence = false;
                shift_register = sample_buffer;
                has_sample = false;
                fetch_sample(bus); // Одразу просимо шину завантажити наступний байт
            }
            else {
                silence = true;
            }
        }

        // Міняємо рівень виходу залежно від біта
        if (!silence) {
            if (shift_register & 0x01) {
                if (output_level <= 125) output_level += 2; // Збільшуємо гучність
            }
            else {
                if (output_level >= 2) output_level -= 2;   // Зменшуємо гучність
            }
        }

        // Зсуваємо регістр для наступного такту
        shift_register >>= 1;
        bit_count--;
    }

    return output_level;
}

double APU::GetOutputSample() const {
    // ФІКС 4B: Автентична апроксимація аудіо мікшера NES (Нелінійна)
    // Ці вагові коефіцієнти ідеально відтворюють баланс між басом, пульсом та ударними.
    double pulse_out = 0.00752 * (pulse1_sample + pulse2_sample);
    // ФІКС: Додано наближений коефіцієнт (0.00335) для каналу DMC
    double tnd_out = 0.00851 * triangle_sample + 0.00494 * noise_sample + 0.00335 * dmc_sample;

    // Результат лежить у безпечних межах ~0.0 до 0.42
    double mixed = pulse_out + tnd_out;

    if (mixed == 0.0) return 0.0;

    // Відцентровуємо хвилю для усунення DC Offset (щоб динаміки не гуділи)
    return mixed - 0.21;
}