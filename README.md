<div align="center">

  # YCsys-NES-Core
  
  **Код, що працює, а не існує. © 2026**
  
  [![Status](https://img.shields.io/badge/Status-In_Development-00FFFF?style=for-the-badge&logo=github)](https://github.com/)
  [![C++](https://img.shields.io/badge/C++-Visual_Studio_2026-blue?style=for-the-badge&logo=c%2B%2B)](https://github.com/)
  [![SDL2](https://img.shields.io/badge/Graphics-SDL2-red?style=for-the-badge)](https://github.com/)
</div>

---

## ⚠️ Статус проєкту
Цей емулятор знаходиться на стадії **активної розробки**. 
Мета проєкту — створення точного (cycle-accurate) ядра процесора MOS 6502 та архітектури NES з нуля на чистому C++ для досягнення максимальної швидкодії та повного контролю над пам'яттю.

---

## 🚀 Етапи розробки (Roadmap)

### 1. ⚙️ Налаштування
- [x] **Visual Studio:** C++ Console App (Empty Project)
- [x] **Графіка:** Підключення бібліотеки SDL2 через NuGet
- [x] **Архітектура:** Базові класи (Bus, CPU, PPU, Cartridge)

### 2. 🧠 CPU (MOS 6502) - База
- [x] **Регістри:** A, X, Y, PC, S, P (Status Flags)
- [x] **Пам'ять:** Реалізація читання/запису на загальній шині (Bus 64KB)
- [x] **Адресація:** 12 режимів адресації 

### 3. ⚡ CPU (MOS 6502) - Опкоди
- [x] **Завантаження/Збереження:** LDA, STA ...
- [x] **Арифметика:** ADC, SBC
- [x] **Інкремент/Декремент:** INC, DEC, INX, INY...
- [x] **Зсуви та Логіка:** AND, ORA, ASL, ROL...
- [x] **Стрибки та Переходи:** JMP, Branches (BNE, BEQ...)
- [x] **Робота з прапорцями та Стеком**

### 4. 💾 Cartridge & Mapper
- [ ] **iNES:** Парсинг заголовку файлу `.nes`
- [ ] **NROM:** Реалізація базового Mapper 000
- [ ] **Маршрутизація:** PRG ROM (процесор) / CHR ROM (графіка)

### 5. 👁️ PPU - База та Логіка (Графіка)
- [ ] **Регістри PPU:** PPUCTRL, PPUDATA, PPUADDR тощо
- [ ] **Пам'ять:** VRAM, Palettes, Pattern Tables
- [ ] **Фон:** Рендеринг бекграунду та скролінг
- [ ] **Спрайти:** OAM (Object Attribute Memory) та Sprite 0 Hit

### 6. 🔗 Інтеграція та Периферія
- [ ] **Синхронізація:** 1 такт CPU = 3 такти PPU
- [ ] **Рендеринг SDL2:** Вивід текстури 256x240 на екран
- [ ] **Керування:** Читання стану Joypad через порт `$4016`

### 7. 🛠 Системні механіки
- [ ] **VRAM Mirroring:** Горизонтальне та вертикальне віддзеркалення
- [ ] **Переривання:** NMI (Non-Maskable Interrupt)
- [ ] **APU:** Аудіо співпроцесор (Pulse, Triangle, Noise)

### 8. 🧪 Тестування
- [ ] **Nestest.nes:** Проходження тестового ROM-файлу
- [ ] **Логи:** Порівняння виводу з еталонним логом виконання Nintendulator

---
<div align="center">
  <i>Developed by Yurii</i>
</div>