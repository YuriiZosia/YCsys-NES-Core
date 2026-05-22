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
 * |      [#]  YCsys NES CORE - SYSTEM BUS & RAM IMPLEMENTATION  [#]         |
 * |      Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.      |
 * |      Started: 2026-05-11 | Project: YCsys-NES-Core                      |
 * |_________________________________________________________________________|
 * [|||]        [|||]        [BUS]        [|||]        [|||]
 */

#include "Bus.h"

Bus::Bus() : cpuRam{ 0 } {
    // Підключаємо CPU до цієї шини при створенні
    cpu.ConnectBus(this);

    // Очищуємо оперативну пам'ять нулями
    for (auto& i : cpuRam) i = 0x00;
}

Bus::~Bus() {}

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cartridge) {
    // Фізично "вставляємо" касету в гніздо шини CPU
    this->cart = cartridge;

    // Передаємо цей же картридж у відеочип PPU, щоб він бачив CHR-пам'ять графіки
    ppu.ConnectCartridge(cartridge);
}

void Bus::clock() {
    // PPU - найшвидший компонент на платі. Він робить один крок КОЖЕН системний такт.
    ppu.clock();

    // CPU працює в 3 рази повільніше. Він робить крок лише кожен третій системний такт.
    if (nSystemClockCounter % 3 == 0) {

        // ФІКС OAM DMA: Якщо активна передача спрайтів - процесор повністю ЗАМОРОЖЕНО!
        if (dma_cycles > 0) {
            dma_cycles--;
        }
        else {
            // Маршрутизація переривань від PPU до CPU. Якщо PPU встановив прапорець nmi_occurred, це означає, що він хоче викликати немасковане переривання (NMI) на CPU. Ми перевіряємо цей прапорець кожного системного такту і, якщо він встановлений, викликаємо метод cpu.nmi() для генерації переривання на CPU.
            if (ppu.nmi_occurred && cpu.cycles == 0) {
                ppu.nmi_occurred = false; // Миттєво гасимо сигнал
                cpu.nmi();                // Передаємо команду переривання на CPU
            }

            // ЛІНІЯ ЗВ'ЯЗКУ IRQ: Якщо мапер картриджа згенерував переривання — б'ємо процесор струмом
            if (cart && cart->irqState()) {
                cart->irqClear(); // Скидаємо прапорець на картриджі
                cpu.irq();        // Викликаємо апаратне переривання процесора!
            }
            cpu.clock(); // Виконуємо інструкцію тільки якщо не заморожені
        }

        apu.clock(); // Звуковий чіп працює завжди, незалежно від заморозки CPU
       
    }

    // Збільшуємо глобальний лічильник часу
    nSystemClockCounter++;
}

void Bus::write(uint16_t addr, uint8_t data) {
    // 1. Спочатку питаємо картридж (можливо, це команда перемикання банків для мапера)
    if (cart && cart->cpuWrite(addr, data)) {
        // Картридж обробив запис, виходимо
    }
    // 2. Оперативна пам'ять CPU (0x0000 - 0x1FFF)
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Маска 0x07FF дзеркалює 2КБ оперативки на весь простір до 0x1FFF
        cpuRam[addr & 0x07FF] = data;
    }
    // 3. Регістри PPU (0x2000 - 0x3FFF)
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // Маска & 0x0007 відсікає дзеркалювання і видає чистий номер регістра від 0 до 7
        ppu.cpuWrite(addr & 0x0007, data);
    }
    // 4. Запуск OAM DMA ($4014) - копіювання сторінки пам'яті в спрайти
    else if (addr == 0x4014) {
        uint16_t dma_page = static_cast<uint16_t>(data) << 8;
        // Безпечно перетворюємо наш масив структур на плоский масив байтів
        uint8_t* pOAM = reinterpret_cast<uint8_t*>(ppu.OAM.data());
        for (uint16_t i = 0; i < 256; i++) {
            pOAM[i] = read(dma_page + i, false);
        }

        // ЗАМОРОЖУЄМО CPU НА 513 АБО 514 ТАКТІВ!
        dma_cycles = 513;
        if (nSystemClockCounter % 2 == 1) dma_cycles++;
    }
    // 5. Регістри звуку APU ($4000-$4013, $4015, $4017)
    else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
        apu.cpuWrite(addr, data);
    }
    // 6. Контролери (Запис у $4016 дає команду "зафіксувати стан кнопок")
    else if (addr == 0x4016) {
        // Апаратний тригер стробування
        bStrobe = (data & 0x01) > 0;

        if (bStrobe) {
            // Коли CPU пише сюди, ми копіюємо поточний стан обох геймпадів у зсувні регістри
            controller_state[0] = controller[0];
            controller_state[1] = controller[1];
        }
    }
}

uint8_t Bus::read(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;

    // 1. Спочатку питаємо картридж (чи є там ROM за цією адресою?)
    if (cart && cart->cpuRead(addr, data)) {
        // Картридж знайшов дані і поклав їх у змінну data
    }
    // 2. Оперативна пам'ять CPU (0x0000 - 0x1FFF)
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Читаємо з RAM з урахуванням дзеркалювання 2КБ
        data = cpuRam[addr & 0x07FF];
    }
    // 3. Регістри PPU (0x2000 - 0x3FFF)
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // Читаємо з PPU, передаючи чистий номер регістра (0-7)
        data = ppu.cpuRead(addr & 0x0007, bReadOnly);
    }
    // 4. Регістри звуку APU ($4000-$4013, $4015)
    else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015) {
        data = apu.cpuRead(addr);
    }
    // 5. Контролери (Читання стану по одному біту за раз)
    else if (addr == 0x4016) {
        if (bStrobe) {
            // Якщо строб активний, завжди повертаємо статус першої кнопки (A)
            data = (controller_state[0] & 0x80) > 0 ? 1 : 0;
        }
        else {
            data = (controller_state[0] & 0x80) > 0 ? 1 : 0;
            // Зсуваємо регістр, щоб наступне читання віддало наступну кнопку
            controller_state[0] <<= 1;
            // ФІКС BOMBERMAN: Всі порожні зчитування контролера мають повертати 1!
            controller_state[0] |= 0x01;
        }
        
    }
	// 6. Читаємо стан контролера 2
    else if (addr == 0x4017) {
        // Читаємо стан контролера 2
        if (bStrobe) {
            data = (controller_state[1] & 0x80) > 0 ? 1 : 0;
        }
        else {
            data = (controller_state[1] & 0x80) > 0 ? 1 : 0;
            controller_state[1] <<= 1;
            controller_state[1] |= 0x01;
        }
    }

    return data;
}