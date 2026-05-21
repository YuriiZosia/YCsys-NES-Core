import os

output_filename = "combined.txt"

# Шукаємо всі .h та .cpp файли у поточній папці
files = [f for f in os.listdir('.') if os.path.isfile(f) and f.endswith(('.h', '.cpp'))]

# Відкриваємо вихідний файл із жорстким кодуванням UTF-8
with open(output_filename, 'w', encoding='utf-8') as outfile:
    for filename in files:
        # Пишемо красивий заголовок для кожного файлу
        outfile.write(f"***FILENAME*** {'='*15} {filename} {'='*15}\n\n")
        
        # Читаємо вихідний код
        try:
            with open(filename, 'r', encoding='utf-8') as infile:
                outfile.write(infile.read())
                outfile.write("\n\n") # Додаємо порожні рядки між файлами для читабельності
        except UnicodeDecodeError:
            print(f"[!] Помилка кодування у {filename}. Файл збережено не в UTF-8!")
        except Exception as e:
            print(f"[!] Не вдалося прочитати {filename}: {e}")

print(f"Готово! Знайдено та об'єднано файлів: {len(files)}.")
print(f"Результат збережено у '{output_filename}' (чистий UTF-8).")