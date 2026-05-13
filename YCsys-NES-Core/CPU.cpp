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
    if (!is_accumulator) {
        fetched = read(addr_abs);
    }
    return fetched;
}

uint8_t CPU6502::GetFlag(FLAGS6502 f) const{
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
    is_accumulator = false; // Скидаємо перед кожною інструкцією
    // 1. Читаємо опкод
    opcode = read(pc);
    pc++;

    // 2. Декодуємо та виконуємо через switch
    switch (opcode) {
        // --- ГРУПА ЗАВАНТАЖЕННЯ / ЗБЕРЕЖЕННЯ ---
        // LDA (Load Accumulator)
    case 0xA9: IMM(); LDA(); break; // Безпосередня адресація
    case 0xA5: ZP0(); LDA(); break; // Адресація нульової сторінки
    case 0xAD: ABS(); LDA(); break; // Абсолютна адресація

        // STA (Store Accumulator)
    case 0x85: ZP0(); STA(); break;
    case 0x8D: ABS(); STA(); break;

        // --- ГРУПА АРИФМЕТИКИ ---
        // ADC (Add with Carry)
    case 0x69: IMM(); ADC(); break;
    case 0x65: ZP0(); ADC(); break;
    case 0x6D: ABS(); ADC(); break;

        // SBC (Subtract with Carry)
    case 0xE9: IMM(); SBC(); break;
    case 0xE5: ZP0(); SBC(); break;
    case 0xED: ABS(); SBC(); break;

        // --- ГРУПА ІНКРЕМЕНТУ / ДЕКРЕМЕНТУ ---
        // Робота з регістрами (Implied)
    case 0xE8: IMP(); INX(); break; // INX 
    case 0xC8: IMP(); INY(); break; // INY
    case 0xCA: IMP(); DEX(); break; // DEX
    case 0x88: IMP(); DEY(); break; // DEY

        // INC (Increment Memory)
    case 0xE6: ZP0(); INC(); break; // INC нульова сторінка
    case 0xEE: ABS(); INC(); break; // INC абсолютна

        // DEC (Decrement Memory)
    case 0xC6: ZP0(); DEC(); break; // DEC нульова сторінка
    case 0xCE: ABS(); DEC(); break; // DEC абсолютна

        // --- ЛОГІЧНІ ОПЕРАЦІЇ ---
        // AND (Logical AND)
    case 0x29: IMM(); AND(); break;
    case 0x25: ZP0(); AND(); break;
    case 0x2D: ABS(); AND(); break;

        // ORA (Logical Inclusive OR)
    case 0x09: IMM(); ORA(); break;
    case 0x05: ZP0(); ORA(); break;
    case 0x0D: ABS(); ORA(); break;

        // EOR (Exclusive OR / XOR)
	case 0x49: IMM(); EOR(); break; // EOR безпосередня адресація
    case 0x45: ZP0(); EOR(); break;
    case 0x4D: ABS(); EOR(); break;
        // BIT (Bit Test)
    case 0x24: ZP0(); BIT(); break; // BIT нульова сторінка
    case 0x2C: ABS(); BIT(); break; // BIT абсолютна

        // --- ЗСУВИ (SHIFTS) ---
        // ASL (Arithmetic Shift Left)
    case 0x0A: IMP(); ASL(); break; // ASL Accumulator
    case 0x06: ZP0(); ASL(); break; // ASL Zero Page
    case 0x0E: ABS(); ASL(); break; // ASL Absolute

        // LSR (Logical Shift Right)
    case 0x4A: IMP(); LSR(); break;
    case 0x46: ZP0(); LSR(); break;
    case 0x4E: ABS(); LSR(); break;

        // ROL (Rotate Left)
    case 0x2A: IMP(); ROL(); break;
    case 0x26: ZP0(); ROL(); break;
    case 0x2E: ABS(); ROL(); break;

        // ROR (Rotate Right)
    case 0x6A: IMP(); ROR(); break;
    case 0x66: ZP0(); ROR(); break;
    case 0x6E: ABS(); ROR(); break;

    case 0x6C: IND(); JMP(); break; // JMP Indirect (з урахуванням багу!)

        // Якщо опкод ще не реалізований або невідомий - нічого не робимо
    default: break;
    }
}

// Створюємо заглужки- заготовки для режимів адресації, щоб код компілювався
uint8_t CPU6502::IMP() {
    is_accumulator = true;
    fetched = a; // Одразу завантажуємо значення акумулятора для роботи
    return 0;
}

// Режим адресації IMM (Immediate) - дані знаходяться в наступному байті після опкода
uint8_t CPU6502::IMM() {
    // В безпосередній адресації адреса даних - це поточне значення лічильника команд
    addr_abs = pc;
    pc++; // Одразу зсуваємо лічильник на наступний байт
    return 0; // Додаткові такти не потрібні
}

// Режим адресації ZP0 (Zero Page) - 8-бітна адреса вказується в наступному байті після опкода, і звертається до першої сторінки пам'яті (0x0000-0x00FF)
uint8_t CPU6502::ZP0() {
    // Читаємо 1 байт адреси
	addr_abs = read(pc);
	pc++; // Зсуваємо лічильник команд на наступний байт
	addr_abs &= 0x00FF; // Маскуємо старший байт, щоб залишити лише 8 біт (адреса в нульовій сторінці)
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

// Режим адресації ABS (Absolute) - 16-бітна адреса вказується безпосередньо в наступних двох байтах після опкода
uint8_t CPU6502::ABS() {
    // Читаємо спочатку молодший байт (lo), потім старший (hi)
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;

    // Склеюємо їх у 16-бітну адресу: зсуваємо hi на 8 біт вліво і додаємо lo
    addr_abs = (hi << 8) | lo;
    return 0;
}
uint8_t CPU6502::ABX() {
    return 0;
}
uint8_t CPU6502::ABY() {
    return 0;
}

// Режим адресації IND (Indirect) - використовується лише для JMP, де 16-бітна адреса вказується в наступних двох байтах після опкода, але фактична адреса для переходу береться з пам'яті за цією адресою
uint8_t CPU6502::IND() {
    uint16_t ptr_lo = read(pc);
    pc++;
    uint16_t ptr_hi = read(pc);
    pc++;

    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    // Відтворення апаратного багу 6502 з межею сторінки
    if (ptr_lo == 0x00FF) {
        // Симулюємо баг переходу сторінки
        addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr);
    }
    else {
        // Нормальна поведінка
        addr_abs = (read(ptr + 1) << 8) | read(ptr);
    }

    return 0;
}
uint8_t CPU6502::IZX() {
    return 0;
}
uint8_t CPU6502::IZY() {
    return 0;
}

// ==================================================================
// ------------------------ Інструкції процесора ------------------------

//
// Завантаження та Збереження (Load/Store)
//

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

// STA (Store Accumulator)
uint8_t CPU6502::STA() {
	write(addr_abs, a); // Записуємо значення акумулятора в пам'ять за адресою addr_abs
    return 0;
}

// STX (Store X Register)
uint8_t CPU6502::STX() {
    write(addr_abs, x); // Записуємо значення регістра X в пам'ять за адресою addr_abs
    return 0;
}

// STY (Store Y Register)
uint8_t CPU6502::STY() {
    write(addr_abs, y); // Записуємо значення регістра Y в пам'ять за адресою addr_abs
    return 0;
}

// --------------------------------------------------------------

//
// Арифметичні операції (Arithmetic)
//

// ADC (Add with Carry)
uint8_t CPU6502::ADC() {
	fetch(); // Отримуємо дані, які потрібно додати до акумулятора

    // Додаємо значення акумулятора, отримані дані та прапорець перенесення (Carry)
	uint16_t temp = (uint16_t)a + (uint16_t)fetched + (uint16_t)GetFlag(FLAGS6502::C);

    // Встановлюємо прапорці
	SetFlag(FLAGS6502::C, temp > 0xFF); // Встановлюємо прапорець перенесення, якщо результат перевищує 255
	SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0); // Встановлюємо прапорець Zero, якщо результат дорівнює нулю
	SetFlag(FLAGS6502::N, (temp & 0x80) != 0); // Встановлюємо прапорець Negative, якщо старший біт результату встановлений

    // Магія обчислення переповнення (Overflow) для 6502
	SetFlag(FLAGS6502::V, ((~((uint16_t)a ^ (uint16_t)fetched) & ((uint16_t)a ^ (uint16_t)temp)) & 0x80) != 0);

	a = temp & 0x00FF; // Зберігаємо лише молодший байт результату в акумуляторі (6502 працює з 8-бітними числами)
    return 1; // Ця команда може потребувати додаткового такту в деяких режимах
}

// SBC (Subtract with Carry)
uint8_t CPU6502::SBC() {
    fetch(); // Отримуємо дані, які потрібно додати до акумулятора

    // Робимо інверсію через XOR & 0x00FF, бо віднімання в 6502 - це додавання інвертованого значення
    uint16_t value = (uint16_t)fetched ^ 0x00FF;

    uint16_t temp = (uint16_t)a + value + (uint16_t)GetFlag(FLAGS6502::C);

    // Встановлюємо прапорці
    SetFlag(FLAGS6502::C, temp > 0xFF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    SetFlag(FLAGS6502::V, (~((uint16_t)a ^ value) & ((uint16_t)a ^ (uint16_t)temp)) & 0x0080);

    a = temp & 0x00FF;
    return 1;
}

// --------------------------------------------------------------
// Інкремент та Декремент (Increment/Decrement)
//

// INX (Increment X Register)
uint8_t CPU6502::INX() {
    x++;
    SetFlag(FLAGS6502::Z, (x & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// INY (Increment Y Register)
uint8_t CPU6502::INY() {
    y++;
    SetFlag(FLAGS6502::Z, (y & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 0;
}

// INC (Increment Memory) - інкрементує значення в пам'яті за адресою addr_abs
uint8_t CPU6502::INC() {
    fetch();
    uint16_t temp = (uint16_t)fetched + 1;
    write(addr_abs, temp & 0x00FF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    return 0;
}

// DEX (Decrement X Register)
uint8_t CPU6502::DEX() {
    x--;
    SetFlag(FLAGS6502::Z, (x & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (x & 0x80) != 0);
    return 0;
}

// DEY (Decrement Y Register)
uint8_t CPU6502::DEY() {
    y--;
    SetFlag(FLAGS6502::Z, (y & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (y & 0x80) != 0);
    return 0;
}

// DEC (Decrement Memory) - декрементує значення в пам'яті за адресою addr_abs
uint8_t CPU6502::DEC() {
    fetch();
    uint16_t temp = (uint16_t)fetched - 1;
    write(addr_abs, temp & 0x00FF);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);
    return 0;
}

// --------------------------------------------------------------
//
// Логічні операції (Logical)
//

// Логічне І (AND)
uint8_t CPU6502::AND() {
    fetch();
    a = a & fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// Логічне АБО (ORA - OR with Accumulator)
uint8_t CPU6502::ORA() {
    fetch();
    a = a | fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// Виключне АБО (EOR - Exclusive OR / XOR)
uint8_t CPU6502::EOR() {
    fetch();
    a = a ^ fetched;
    SetFlag(FLAGS6502::Z, (a & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (a & 0x80) != 0);
    return 1;
}

// BIT (Bit Test) - тестує біти в акумуляторі та встановлює прапорці на основі даних з пам'яті
uint8_t CPU6502::BIT() {
    fetch();

    // Z-flag: Встановлюється, якщо (A AND M) == 0
    uint8_t temp = a & fetched;
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);

    // N-flag: Набуває значення 7-го біта даних прямо з пам'яті
    SetFlag(FLAGS6502::N, (fetched & (1 << 7)) != 0);

    // V-flag: Набуває значення 6-го біта даних прямо з пам'яті
    SetFlag(FLAGS6502::V, (fetched & (1 << 6)) != 0);

    return 0; // BIT не потребує додаткових тактів
}

// --------------------------------------------------------------
//
// Операції зсуву та обертання (Shifts and Rotates)
//

// ASL (Arithmetic Shift Left) - зсуває біти вліво, нуль потрапляє в правий бік, а лівий біт потрапляє в Carry
uint8_t CPU6502::ASL() {
    fetch(); // Завдяки логіці в IMP() та fetch(), тут уже буде або 'a', або значення з пам'яті
    uint16_t temp = (uint16_t)fetched << 1; // Зсуваємо вліво. Використовуємо 16 біт, щоб "зловити" 7-й біт, який випаде у 8-й

    SetFlag(FLAGS6502::C, (temp & 0xFF00) != 0);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);

    // А тепер вирішуємо, куди повернути результат
    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }

    return 0;
}

// LSR (Logical Shift Right) - зсуває біти вправо, нуль потрапляє в лівий бік, а правий біт потрапляє в Carry
uint8_t CPU6502::LSR() {
    fetch();
    // Нульовий біт потрапляє в Carry
    SetFlag(FLAGS6502::C, (fetched & 0x0001) != 0);

    // Зсуваємо вправо
    uint8_t temp = fetched >> 1;

    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x0080) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ROL (Rotate Left) - зсуває біти вліво, нуль потрапляє в правий бік, а лівий біт потрапляє в Carry. При цьому, старий Carry потрапляє в правий біт.
uint8_t CPU6502::ROL() {
    fetch();

    /// Зсуваємо вліво, звільняючи 0-й біт, і записуємо в нього поточний Carry
    uint16_t temp = (uint16_t)(fetched << 1) | GetFlag(FLAGS6502::C);

    SetFlag(FLAGS6502::C, (temp & 0xFF00) != 0);
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x0080) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ROR (Rotate Right) - зсуває біти вправо, нуль потрапляє в лівий бік, а правий біт потрапляє в Carry. При цьому, старий Carry потрапляє в лівий біт.
uint8_t CPU6502::ROR() {
    fetch();
    // Новий Carry — це 0-й біт ДО зсуву
    bool bNewCarry = (fetched & 0x01) != 0;

    // Зсуваємо вправо, звільняючи 7-й біт, і «десантуємо» в нього Carry (зсунувши його на 7 позицій вліво)
    uint16_t temp = (uint16_t)(fetched >> 1) | ((uint16_t)GetFlag(FLAGS6502::C) << 7);

    SetFlag(FLAGS6502::C, bNewCarry); // Встановлюємо той самий 0-й біт
    SetFlag(FLAGS6502::Z, (temp & 0x00FF) == 0x00);
    SetFlag(FLAGS6502::N, (temp & 0x80) != 0);

    if (is_accumulator) {
        a = temp & 0x00FF;
    }
    else {
        write(addr_abs, temp & 0x00FF);
    }
    return 0;
}