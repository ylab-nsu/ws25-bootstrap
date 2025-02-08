# ws25-bootstrap

## Мотивация

Современные версии практически всех операционных систем написаны на языках высокого уровня. В случае unix используется C и C++, компиляторы которых зачастую также написаны на C и C++.

Подобные циклические зависимости приводят к проблеме Trusting trust. Представим, что компилятор содержит в себе модуль, который при определенных условиях выполняет инъекцию вредоносного кода в результирующий файл. В таком случае программа, скомпилированная этим компилятором (к примеру, другой компилятор), также может содержать инъекции.

Решение проблемы trusting trust непосредственно связано с получением доверенного компилятора и доверенной операционной системы для его исполнения.

В данном случае под доверенными подразумеваются программы, которые удовлетворяют одному из следующих условий

1. Программа скомпилирована с использованием доверенных инструментов
2. Бинарный файл программы может быть явно проверен на наличие инъекций

## Цель

За основу взят проект [fosslinux/live-bootstrap](https://github.com/fosslinux/live-bootstrap), позволяющий произвести компиляцию ядра linux "с нуля".

Одной из важных составляющих live-bootstrap &mdash; доверенное ядро builder-hex0, написанное для x86. Цель проекта &mdash; произвести портирование данного ядра на RISC-V.

## Задачи

Ядро live-bootstrap состоит из двух частей:

- stage-1 &mdash; часть builder-hex0, выполняющая следующие задачи
    1. Загрузка с диска stage-2 в формате hex0
    2. Конвертация hex0 в бинарный вид
    3. Исполнение stage 2
- stage 2 &mdash; полноценная миниатюрная unix-подобная операционная система

Цель данного проекта:

- [x] Произвести портирование stage-1 на архитектуру RISC-V
- [x] Изучить stage-2
  - [x] Написать псевдокод для stage-2 для облегчения понимания системы
  - [x] Задокументировать основные особенности stage-2 для облегчения будущего портирования
  - [x] Выявить потенциальные точки улучшения системы

## stage-1

### Запуск stage-1 из исходного кода

1. Установите требуемые зависимости

    ```bash
    sudo apt install gcc-riscv64-unknown-elf
    ```

    Для запуска на qemu дополнительно установите `qemu-system-riscv64`:

    ```bash
    sudo apt install qemu-system-riscv64
    ```

2. Соберите объектный файл

    ```bash
    riscv64-unknown-elf-as ./src/stage1.s -o stage1.o
    ```

3. Сгенерируйте elf-файл

    ```bash
    riscv64-unknown-elf-ld -Ttext 0 stage1.o -o stage1.elf
    ```

4. Сгенерируйте бинарный файл

    ```bash
    riscv64-unknown-elf-objcopy -O binary stage1.elf stage1.bin
    ```

5. Запустите бинарный файл

    1. Для запуска на реальном устройстве скопируйте бинарный файл на диск (в примере `/dev/sda`)

        ```bash
        dd if=stage1.bin of=/dev/sda
        ```

    2. Для запуска на qemu:

        ```bash
        qemu-system-riscv64 //TBD
        ```

### Запуск stage-1 из hex0

1. Установите требуемые зависимости

    ```bash
    sudo apt install xxd
    ```

    Для запуска на qemu установите `qemu-system-riscv64`:

    ```bash
    sudo apt install qemu-system-riscv64
    ```

2. Получите из hex0 бинарный файл

    ```bash
    sed 's/[;#].*$//g' stage1.hex0 | xxd -r -p > stage1.bin
    ```

3. Запустите бинарный файл

    1. Для запуска на реальном устройстве скопируйте бинарный файл на диск (в примере `/dev/sda`)

        ```bash
        dd if=stage1.bin of=/dev/sda
        ```

    2. Для запуска на qemu:

        ```bash
        qemu-system-riscv64 //TBD

## stage-2

- [Документация на wiki](./wiki)
- [Псевдокод](./disassemple.c)
