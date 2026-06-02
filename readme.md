# Xray_affection

Симуляция взаимодействия рентгеновского излучения с металлической пластиной на базе [Geant4](https://geant4.web.cern.ch/).

Проект позволяет изучать физические процессы взаимодействия рентгена с веществом:
- ☢️ **Фотоэффект** — поглощение фотона и вылет электрона
- 🔀 **Комптоновское рассеяние** — неупругое рассеяние с изменением энергии
- 📉 **Поглощение излучения** — затухание пучка в материале

---

## Содержание

- [Геометрия симуляции](#геометрия-симуляции)
- [Физические модели](#физические-модели)
- [Параметры симуляции](#параметры-симуляции)
- [Требования](#требования)
- [Сборка и запуск (нативно)](#сборка-и-запуск-нативно)
- [Запуск через Docker](#запуск-через-docker)
- [Выходные данные](#выходные-данные)
- [Постобработка](#постобработка)

---

## Геометрия симуляции

```
Источник (gamma, 30 keV)
Сканирующая сетка N×N
     ↓
 ┌──────────────────────────────────────┐
 │         Мировой объём (вакуум)        │
 │                                      │
 │   ┌────────────────────────┐         │
 │   │    Металлическая       │  Z=10cm │
 │   │    пластина (Cu/Pb/…)  │         │
 │   │    ■■■■  ○  ■■■■■■    │         │
 │   │    (с центральным     │         │
 │   │     отверстием)       │         │
 │   └────────────────────────┘         │
 │                                      │
 │   ┌────────────────────────┐         │
 │   │  Сцинтиллятор CsI (50мкм)│       │
 │   └────────────────────────┘         │
 │   ┌────────────────────────┐         │
 │   │  Оптический клей (10мкм)│        │
 │   └────────────────────────┘         │
 │   ┌────────────────────────┐         │
 │   │  Si детектор (0.5мм)   │         │
 │   └────────────────────────┘         │
 └──────────────────────────────────────┘
```

| Компонент | Материал | Размер |
|---|---|---|
| Мировой объём | Вакуум (галактический) | 10×10×33 см |
| Металлическая пластина | G4_Cu (по умолчанию) | 1×1 мм × 100 мкм |
| Отверстие в пластине | — | ∅ 0.4 мм, смещено к краю |
| Сцинтиллятор (CsI) | CsI (йодид цезия) | 1×0.5 мм × 50 мкм |
| Оптический клей | Клей n=1.5 | 1×0.5 мм × 10 мкм |
| Si детектор | G4_Si | 1×0.5 мм × 0.5 мм |

Источник сканирует пластину по сетке **N×N пикселей** (задаётся в макросе).

---

## Физические модели

Подключены через [`src/PMPhysicsList.cc`](src/PMPhysicsList.cc):

- **`G4EmLivermorePhysics`** — электромагнитная физика (модель Ливермора): точная обработка фотоэффекта, Комптона, Рэлеевского рассеяния для низких энергий (1 кэВ — 100 МэВ)
- **`G4OpticalPhysics`** — оптическая физика: сцинтилляция CsI, распространение оптических фотонов
- **Поверхность CsI/Si** — `dielectric_dielectric`, модель `unified`, `polished`: передача оптических фотонов по формулам Френеля

---

## Параметры симуляции

### Материал и геометрия пластины

Файл: [`src/PMDetectorConstruction.cc`](src/PMDetectorConstruction.cc)

```cpp
// Строка 7 — толщина пластины (мкм)
G4double leadThickness = 100. * um;

// Строка 8 — материал (NIST-имена)
G4String material = "G4_Cu";
```

Доступные материалы NIST (примеры):

| NIST-имя | Материал |
|---|---|
| `G4_Cu` | Медь |
| `G4_Pb` | Свинец |
| `G4_Al` | Алюминий |
| `G4_Fe` | Железо |
| `G4_WATER` | Вода |

Полный список: [NIST Material Database](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/Appendix/materialNames.html)

### Энергия излучения

Файл: [`src/PMPrimaryGenerator.cc`](src/PMPrimaryGenerator.cc)

```cpp
// Строка 50 — энергия частицы (keV)
fParticleGun->SetParticleEnergy(30. * keV);
```

### Параметры сканирования (макрос)

Файл: [`macros/run.mac`](macros/run.mac)

```
# Потоки (рекомендуется = числу ядер CPU)
/run/numberOfThreads 6

# ВАЖНО: initialize должна быть ДО команд /gun/
/run/initialize

# Размер сетки N×N пикселей (разрешение рентгенограммы)
/gun/gridSize 100

# Квантов на пиксель (статистика / шум)
/gun/particlesPerPixel 3

# Итого событий = gridSize² × particlesPerPixel = 100×100×3 = 30 000
# beamOn должен быть >= этого числа (рекомендуется с запасом 2×)
/run/beamOn 60000
```

> ⚠️ **Важно:** `/run/initialize` должна идти **перед** `/gun/gridSize` и `/gun/particlesPerPixel` — UI-мессенджер регистрируется при инициализации.

---

## Требования

### Нативная сборка

- CMake ≥ 3.2
- C++17-совместимый компилятор (GCC, Clang)
- [Geant4](https://geant4.web.cern.ch/download/) ≥ 11.0 (с поддержкой `ui_all` и `vis_all` для визуализации)

### Постобработка

- Python ≥ 3.8
- `pandas`, `numpy`, `matplotlib`, `scipy`

---

## Сборка и запуск (нативно)

```bash
# Создаём и переходим в папку сборки
mkdir -p build && cd build

# Генерируем makefile
cmake ..

# Собираем
make -j$(nproc)
```

### Режим визуализации (интерактивный)

```bash
cd build
./sim
```

В открывшемся GUI:

```
/run/beamOn 100
```

### Batch-режим

```bash
cd build
./sim run.mac
```

### Постобработка

```bash
cd build
python3 ../build/img.py
```

---

## Запуск через Docker

### Предварительные условия

- [Docker](https://docs.docker.com/get-docker/) ≥ 20.10
- [Docker Compose](https://docs.docker.com/compose/) ≥ 2.0

> ⚠️ Docker-образ работает в **headless batch-режиме** (без GUI). Визуализация в контейнере недоступна.

### Быстрый старт

```bash
# Сборка образа и запуск симуляции
docker compose up --build

# Результаты будут в папке ./results/
```

### Запуск с другим числом частиц/толщиной

Отредактируйте [`macros/run.mac`](macros/run.mac) перед запуском (пересборка Docker не требуется — макрос копируется при сборке образа):

```
/gun/gridSize 200           # 200×200 пикселей (выше разрешение)
/gun/particlesPerPixel 10   # 10 квантов на пиксель (выше статистика)
/run/beamOn 800000          # 200×200×10×2 = 800 000
```

### Запуск с конкретной толщиной пластины для постобработки

```bash
THICKNESS=200 docker compose up simulation
```

### Только постобработка (если симуляция уже выполнена)

```bash
docker compose run postprocess
```

### Ручная сборка образа

```bash
docker build -t xray-affection .
docker run --rm -v "$(pwd)/results:/app/results" xray-affection run.mac
```

---

## Выходные данные

После прогона симуляции в `./results/` появятся:

| Файл | Формат | Описание |
|---|---|---|
| `hits_data_<N>um.csv` | TSV | Hits детектора: Energy_eV, PosX_cm, PosY_cm, Type, EventID |
| `output.root` | ROOT ntuple | То же в формате ROOT: fEvent, fX, fY, fZ |
| `run_params.txt` | Text | Параметры прогона + ASCII-схема оптической цепочки |

Типы записей в CSV:

| Тип | Описание |
|---|---|
| `optical_photon` | Оптический фотон от сцинтилляции CsI, зарегистрированный Si детектором |
| `scattered` | Гамма-квант, рассеянный или ослабленный пластиной |
| `pass-through` | Гамма-квант, прошедший без изменений (угол < 0.01°, ΔE < 5%) |

---

## Постобработка

Скрипт [`build/img.py`](build/img.py) строит рентгенограммы на основе `hits_data_*.csv`.

```bash
cd build
python3 img.py
```

Результаты сохраняются в `./results/images/`. Скрипт:

- читает файлы `hits_data_<thickness>um.csv` из `./results/`
- фильтрует только `optical_photon` записи
- строит 2D-гистограммы числа фотонов по позиции XY
- применяет Gaussian-сглаживание (`scipy.ndimage.gaussian_filter`)
- сохраняет PNG: сырые counts, логарифм, ослабление, сглаженное

---

## Структура проекта

```
Xray_affection/
├── CMakeLists.txt              # Конфигурация сборки
├── sim.cc                      # Точка входа (main)
├── include/
│   ├── PMDetectorConstruction.hh
│   ├── PMPhysicsList.hh
│   ├── PMPrimaryGenerator.hh   # fGridSize, fParticlesPerPixel (public static)
│   ├── PMRunAction.hh          # fGridMessenger (UI-команды /gun/)
│   ├── PMSensitiveDetector.hh
│   └── PMActionInitialization.hh
├── src/
│   ├── PMDetectorConstruction.cc  # Геометрия: пластина + CsI + Si
│   ├── PMPhysicsList.cc           # Livermore EM + Optical physics
│   ├── PMPrimaryGenerator.cc      # Источник: гамма, сканирующая сетка, ResetGrid()
│   ├── PMRunAction.cc             # ROOT output, run_params.txt, G4GenericMessenger
│   ├── PMSensitiveDetector.cc     # Регистрация hits → hits_data.csv (5 колонок)
│   └── PMActionInitialization.cc
├── macros/
│   ├── run.mac     # Основной макрос: потоки, gridSize, particlesPerPixel, beamOn
│   ├── energy.mac  # Установка энергии для цикла
│   ├── runone.mac  # Одиночный прогон
│   └── vis.mac     # Визуализация (OpenGL)
├── build/
│   └── img.py      # Постобработка: CSV → PNG рентгенограммы
├── Dockerfile
├── docker-compose.yml
└── README.md
```

---

## Ссылки

- [Geant4 User Guide](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/)
- [G4EmLivermorePhysics](https://geant4-userdoc.web.cern.ch/UsersGuides/PhysicsReferenceManual/html/electromagnetic/lowenergy/index.html)
- [NIST Material Database](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/Appendix/materialNames.html)
