#include "PMRunAction.hh"
#include "PMSensitiveDetector.hh"
#include "PMDetectorConstruction.hh"
#include "PMPrimaryGenerator.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"
#include <fstream>
#include <ctime>


G4GenericMessenger* PMRunAction::fGridMessenger = nullptr;

PMRunAction::PMRunAction()
{
    G4AnalysisManager::Instance();

    // Регистрируем UI-команды /gun/ на master-потоке (только один раз).
    // PMRunAction::BuildForMaster() создаёт экземпляр на master-потоке,
    // поэтому мессенджер доступен при разборе макроса до /run/beamOn.
    if (!fGridMessenger) {
        fGridMessenger = new G4GenericMessenger(this, "/gun/",
            "Параметры сканирующей сетки источника");

        fGridMessenger->DeclareProperty("gridSize", PMPrimaryGenerator::fGridSize)
            .SetGuidance("Размер сетки N (N x N пикселей)")
            .SetParameterName("N", false)
            .SetRange("N>0")
            .SetDefaultValue("100");

        fGridMessenger->DeclareProperty("particlesPerPixel", PMPrimaryGenerator::fParticlesPerPixel)
            .SetGuidance("Число квантов на один пиксель сетки")
            .SetParameterName("N", false)
            .SetRange("N>0")
            .SetDefaultValue("3");
    }
}

PMRunAction::~PMRunAction()
{
}

void PMRunAction::BeginOfRunAction(const G4Run *run)
{
    // Сброс счётчиков сетки перед каждым прогоном (важно для цикла energy.mac)
    if (G4Threading::IsMasterThread()) {
        PMPrimaryGenerator::ResetGrid();
    }

    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->SetNtupleMerging(true);
    man->OpenFile("output.root");

    man->CreateNtuple("Hits", "Hits");
    man->CreateNtupleIColumn("fEvent");
    man->CreateNtupleDColumn("fX");
    man->CreateNtupleDColumn("fY");
    man->CreateNtupleDColumn("fZ");
    man->FinishNtuple(0);
}

void PMRunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager::Instance()->Write();
    G4AnalysisManager::Instance()->CloseFile();

    // Запись параметров симуляции только на master-потоке
    if (!G4Threading::IsMasterThread()) return;

    std::ofstream params("run_params.txt");
    if (!params.is_open()) {
        G4cerr << "ERROR: Cannot write run_params.txt" << G4endl;
        return;
    }

    // Временная метка
    std::time_t now = std::time(nullptr);
    char timebuf[64];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    const G4int gridSize      = PMPrimaryGenerator::fGridSize;
    const G4int perPixel      = PMPrimaryGenerator::fParticlesPerPixel;
    const G4int totalEvents   = run->GetNumberOfEvent();
    const G4double thickUm    = leadThickness / um;
    const G4double csiUm      = csiThickness  / um;
    const G4double detMm      = detectorThickness / mm;
    const G4double energyKeV  = energy / keV;

    params << "============================================================\n";
    params << "  ПАРАМЕТРЫ СИМУЛЯЦИИ Xray_affection\n";
    params << "============================================================\n";
    params << "Дата/время запуска : " << timebuf << "\n";
    params << "\n";
    params << "--- Источник излучения ---\n";
    params << "Тип частицы        : gamma\n";
    params << "Энергия            : " << energyKeV  << " кэВ\n";
    params << "Направление        : вдоль оси Z (0, 0, 1)\n";
    params << "Сетка сканирования : " << gridSize << " x " << gridSize << " пикселей\n";
    params << "Частиц на пиксель  : " << perPixel << "\n";
    params << "Всего событий      : " << totalEvents << "\n";
    params << "\n";
    params << "--- Объект (металлическая пластина) ---\n";
    params << "Материал           : " << material << "\n";
    params << "Толщина            : " << thickUm << " мкм\n";
    params << "Размер             : 1.0 x 1.0 мм\n";
    params << "Отверстие          : диаметр 0.4 мм (смещено к краю)\n";
    params << "Позиция Z          : 10 см от источника\n";
    params << "\n";
    params << "--- Сцинтиллятор (CsI) ---\n";
    params << "Материал           : CsI (йодид цезия)\n";
    params << "Толщина            : " << csiUm << " мкм\n";
    params << "Размер             : 1.0 x 0.5 мм\n";
    params << "Выход сцинтилляций : 54000 фотонов/МэВ\n";
    params << "Время высвечивания : 1000 нс\n";
    params << "\n";
    params << "--- Детектор (кремний Si) ---\n";
    params << "Материал           : G4_Si\n";
    params << "Толщина            : " << detMm << " мм\n";
    params << "Размер             : 1.0 x 0.5 мм\n";
    params << "КЭ @ 2.5 эВ       : 80%\n";
    params << "\n";
    params << "--- Оптическая схема ---\n";
    params << "\n";
    params << "  [gamma " << energyKeV << " кэВ]\n";
    params << "        |\n";
    params << "        v  Z = 0\n";
    params << "  ┌─────┴─────┐\n";
    params << "  │  " << material << "  пластина  │  " << thickUm << " мкм\n";
    params << "  │  ■■■■○■■■  │  (○ = отверстие)\n";
    params << "  └─────┬─────┘\n";
    params << "        |  Z = 10 см\n";
    params << "  ┌─────┴─────┐\n";
    params << "  │    CsI    │  " << csiUm << " мкм  — сцинтиллятор\n";
    params << "  └─────┬─────┘\n";
    params << "        |\n";
    params << "  ┌─────┴─────┐\n";
    params << "  │  клей 10мкм│\n";
    params << "  └─────┬─────┘\n";
    params << "        |\n";
    params << "  ┌─────┴─────┐\n";
    params << "  │  Si детект.│  " << detMm << " мм\n";
    params << "  └───────────┘\n";
    params << "\n";
    params << "--- Физические модели ---\n";
    params << "ЭМ физика          : G4EmLivermorePhysics (1 кэВ — 100 МэВ)\n";
    params << "Оптическая физика  : G4OpticalPhysics\n";
    params << "Поверхность CsI/Si : dielectric_dielectric, unified, polished\n";
    params << "\n";
    params << "--- Выходные файлы ---\n";
    params << "hits_data.csv      : Energy_eV, PosX_cm, PosY_cm, Type, EventID\n";
    params << "output.root        : ntuple Hits (fEvent, fX, fY, fZ)\n";
    params << "============================================================\n";

    params.close();
    G4cout << "[RunAction] Параметры симуляции сохранены в run_params.txt" << G4endl;
}