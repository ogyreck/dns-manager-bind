# dns-manager-admin

Графический менеджер DNS-сервера BIND9 для Astra Linux (и Debian-совместимых систем).

## Запуск из deb пакета

скоро 

## Быстрый старт (для сборки из исходников)

### 1. Зависимости

```bash
sudo apt-get install -y \
    build-essential cmake \
    qtbase5-dev qttools5-dev-tools \
    bind9
```

### 2. Сборка из исходников

```bash
cmake -S . -B build
cmake --build build
```

### 3. Запуск

```bash
./build/dns-manager-admin
```

Приложение при старте читает `/etc/bind/named.conf`. Если BIND9 не запущен или файл недоступен - ошибка отобразится в статусбаре, GUI откроется(коряво).

### 4. Тесты парсеров

```bash
cmake --build build --target dns_parser_tests
cd build && ctest -V
```

## Требования

- CMake ≥ 3.10
- Qt 5.15
- GCC с поддержкой C++17
- BIND9 (пакет `bind9`)
