<div style="text-align: justify;">

# ws25-bootstrap

## Мотивация

Современные версии операционных систем и программ для них зачастую написаны на C и C++. При этом компиляторы этих языков также написаны на языках C и C++, что приводит к циклической зависимости. Для сборки компилятора нужен компилятор, нужна последовательная сборка тулчейна от минимального компилятора к полноценному компилятору. 

Проблема Trusting Trust заключается в том, что мы должны доверять всей этой цепочке компиляторов. В статье "Reflections on Trusting Trust" Кена Томпсона представлен пример вредоносного кода с поведением вида: обычную программу собрать без изменений, компилятор собрать вставив самого себя, login собрать с возможностью root доступа для злоумышленника. 

Решением этой проблемы будет автоматическое развертывание доверенного окружения, начиная с простого компилятора, исходный код которого легко сравнить с получившимся исполняемым файлом. Проект [fosslinux/live-bootstrap](https://github.com/fosslinux/live-bootstrap) предназначен для сборки ядра Linux на процессорах архитектуры x86. Минимальное стартовое окружение называется builder-hex0, и написано на языке hex0. hex0 состоит из двух частей builder-hex0-stage1 и builder-hex0-stage2. builder-hex0-stage2 представляет собой минимальное ядро пригодное для компиляции дальнейших шагов. builder-hex0-stage1 - минимальный транслятор hex0 кода в исполняемый код.

## Цель

За основу взят проект [fosslinux/live-bootstrap](https://github.com/fosslinux/live-bootstrap), позволяющий произвести компиляцию ядра Linux из минимального набора бинарных файлов. Целью было развернуть builder-hex0-stage2 на архитектуре RISCV.

## Задачи

- [x] Портировать builder-hex0-stage1 на LecheePi 4A
  -  [x] Настроить процесс сборки загрузочных образов для LecheePi 4A
  -  [x] Реализовать передачу данных по UART 
  -  [x] Переписать builder-hex0-stage1 на ассемблер RISCV
- [x] Изучить builder-hex0-stage2
  - [x] Написать псевдокод builder-hex0-stage2
  - [x] Задокументировать основные особенности builder-hex0-stage2
  - [x] Выявить потенциальные точки улучшения системы

## Сборка прототипа stage1

Если префикс вашего cross toolchain отличен от `riscv64-elf-` (или сборка проходит нативно на RISC-V) то выставите переменную префикса:
```
$ export CROSS_COMPILE=<your-prefix->
```

### Образ SD карты для Lichee Pi 4A

```console
$ make image_lpi4a
```

###  Образ для QEMU virt

```console
$ make image_qemu
```

### Сборка руками

1. Соберём ELF файл программы
```console
$ <prefix>-gcc -march=rv64gc\
			   -nostdlib\
			   -ffreestanding\
			   -Ttext 0\
			   ./src/stage1.S -o stage1.elf
```
2. Скопируем .text секцию в чистый бинарный файл
```console
$ <prefix>-objcopy -O binary stage1.elf stage1.bin
```
3. Сконкатенируем `stage1.bin` и код полезной нагрузки (`payload_lpi4a.hex0` или `payload_qemu.hex0`)

```console
$ cat stage1.bin <payload>.hex0 > image_<platform>.bin
```

## Запуск прототипа stage1

### На Lichee Pi 4A

1. Выставите DIP-переключатель выбора режима загрузки в режим загрузки с SD карты
2. Загрузите образ на SD карту (она должна быть заполнена нулями хотя бы на 1.5 МБ)
```console
$ dd if=image_lpi4a.bin of=<SD Card>
```
3. Вставьте SD карту в слот и включите питание платы

### На QEMU RISC-V virt

```console
$ qemu-system-riscv64 -M virt -bios image_qemu.bin
```

## Документация проекта

- [Документация на wiki](https://github.com/ylab-nsu/ws25-bootstrap/wiki)
- [Псевдокод](https://github.com/ylab-nsu/ws25-bootstrap/blob/dev/builder-hex0-stage2/disassembly.c)
