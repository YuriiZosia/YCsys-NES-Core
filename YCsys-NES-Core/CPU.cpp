// YCsys NES Core - 6502 CPU implementation
// Yurii Code system (YCsys) © 2026. Код, що працює, а не існує.
#include "CPU.h"
#include "Bus.h" // Обов'язково додаємо, щоб бачити методи класу Bus

// Конструктор: виконується при створенні об'єкта
CPU6502::CPU6502() {
    a = 0x00;
    x = 0x00;
    y = 0x00;
    stkp = 0x00;
    pc = 0x0000;
    status = 0x00;
}
// Деструктор: виконується при видаленні об'єкта
CPU6502::~CPU6502() {
    
}

// Реалізація приватного методу читання
uint8_t CPU6502::read(uint16_t addr) {
    // Якщо процесор підключений до шини, читаємо дані
    if (bus != nullptr) {
        return bus->read(addr);
    }
    return 0x00; // Повертаємо 0, якщо шина не підключена
}

// Реалізація приватного методу запису
void CPU6502::write(uint16_t addr, uint8_t data) {
    // Якщо процесор підключений до шини, записуємо дані
    if (bus != nullptr) {
        bus->write(addr, data);
    }
}

// Допоміжна функція fetch() для читання даних за адресою addr_abs
uint8_t CPU6502::fetch() {
    // Якщо режим адресації не неявний (IMP), читаємо дані з пам'яті
    fetched = read(addr_abs);
    return fetched;
}

uint8_t CPU6502::GetFlag(FLAGS6502 f) {
    // Якщо біт за маскою 'f' встановлений, повертаємо 1, інакше 0
    return ((status & f) > 0) ? 1 : 0;
}

void CPU6502::SetFlag(FLAGS6502 f, bool v) {
    if (v) {
        status |= f;    // Встановлюємо біт (OR)
    }
    else {
        status &= ~f;   // Скидаємо біт (AND з інверсією)
    }
}

void CPU6502::clock() {
    // 1. Читаємо опкод
    opcode = read(pc);
    pc++;

    // 2. Декодуємо та виконуємо. 
    // Опкод 0xA9 - це LDA з режимом IMM.
    if (opcode == 0xA9) {
        IMM(); // Викликаємо режим адресації (він запише адресу в addr_abs)
        LDA(); // Виконуємо команду
    }
}

// Створюємо заглужки- заготовки для режимів адресації, щоб код компілювався
uint8_t CPU6502::IMP() {
    return 0;
}
uint8_t CPU6502::IMM() {
    // В безпосередній адресації адреса даних - це поточне значення лічильника команд
    addr_abs = pc;
    pc++; // Одразу зсуваємо лічильник на наступний байт
    return 0; // Додаткові такти не потрібні
}
uint8_t CPU6502::ZP0() {
    return 0;
}
uint8_t CPU6502::ZPX() {
    return 0;
}
uint8_t CPU6502::ZPY() {
    return 0;
}
uint8_t CPU6502::REL() {
    return 0;
}
uint8_t CPU6502::ABS() {
    return 0;
}
uint8_t CPU6502::ABX() {
    return 0;
}
uint8_t CPU6502::ABY() {
    return 0;
}
uint8_t CPU6502::IND() {
    return 0;
}
uint8_t CPU6502::IZX() {
    return 0;
}
uint8_t CPU6502::IZY() {
    return 0;
}

// ========================================================================
// Описи інструкцій процесора.
// LDA (Load Accumulator)
uint8_t CPU6502::LDA() {
	fetch(); // Отримуємо дані, які потрібно завантажити в акумулятор
    a = fetched; // Завантажуємо дані в акумулятор
	// Встановлюємо прапорці Zero та Negative на основі нового значення акумулятора
	SetFlag(FLAGS6502::Z, a == 0x00); // Якщо a == 0, встановлюємо прапорець Zero
	SetFlag(FLAGS6502::N, (a & 0x80) != 0); // Якщо старший біт a встановлений, встановлюємо прапорець Negative. 0x80 - це 10000000 у двійковій. a & 0x80 це перевірка, чи встановлений старший біт (бит 7) в a.
    return 1; // Ця команда може потребувати додаткового такту, тому повертаємо 1 (на майбутнє)
}
// LDX (Load X Register)
uint8_t CPU6502::LDX() {
    fetch();
    x = fetched;
    SetFlag(FLAGS6502::Z, x == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 1;
}
// LDY (Load Y Register)
uint8_t CPU6502::LDY() {
    fetch();
    y = fetched;
    SetFlag(FLAGS6502::Z, y == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 1;
}
uint8_t CPU6502::STA() {
    return 0;
}
uint8_t CPU6502::STX() {
    return 0;
}
uint8_t CPU6502::STY() {
    return 0;
}
