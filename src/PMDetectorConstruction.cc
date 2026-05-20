#include "PMDetectorConstruction.hh"
#include "G4PhysicalConstants.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"

G4double leadThickness = 100. * um; //изменение толщины пластины в мкм
G4String material = "G4_Cu"; //Объявление материала

PMDetectorConstruction::PMDetectorConstruction()
{
}

PMDetectorConstruction::~PMDetectorConstruction()
{
}

G4VPhysicalVolume* PMDetectorConstruction::Construct()
{
    G4bool checkOverlaps = true;

    G4double density = universe_mean_density;                //from PhysicalConstants.h
    G4double pressure = 1.e-19 * pascal;
    G4double temperature = 0.1 * kelvin;
    new G4Material("Galactic", 1., 1.01 * g / mole, density,
        kStateGas, temperature, pressure);


    G4NistManager* nist = G4NistManager::Instance();
    G4Material* worldMat = nist->FindOrBuildMaterial("Galactic");
    G4Material* leadMat = nist->FindOrBuildMaterial(material);
    G4Material* detMat = nist->FindOrBuildMaterial("G4_SODIUM_IODIDE");

    // Оптические свойства для вакуума
    G4MaterialPropertiesTable* vacMPT = new G4MaterialPropertiesTable();
    const G4int nVac = 2;
    G4double vacEnergies[] = { 1.0 * eV, 6.0 * eV };
    G4double vacRindex[] = { 1.0, 1.0 };
    vacMPT->AddProperty("RINDEX", vacEnergies, vacRindex, nVac);
    worldMat->SetMaterialPropertiesTable(vacMPT);

    G4double xWorld = 1. * m;
    G4double yWorld = 1. * m;
    G4double zWorld = 1. * m;

    G4Box* solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicalWorld");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(0., 0., 0.), logicWorld, "physWorld", 0, false, 0);

    G4double leadSize = 10.0 * cm;

    // Основное тело (сплошная пластина)
    G4Box* solidLeadBase = new G4Box("solidLeadBase", 0.5 * leadSize, 0.5 * leadSize, 0.5 * leadThickness);

    // Общие параметры для всех полос
    G4double slitLength = 1.0 * cm;            // длина (по X)
    G4double gapBetweenSlits = 0.5 * mm;       // расстояние между полосами в одной группе (по Y)
    G4double slitHalfZ = 0.5 * leadThickness + 0.1 * mm; // запас по Z для сквозного вырезания

    // Левый нижний угол пластины (центр пластины в (0,0,0))
    G4double leftBottomX = -0.5 * leadSize;
    G4double leftBottomY = -0.5 * leadSize;

    // Отступ от левого нижнего угла для начала первой полосы
    G4double offsetFromCorner = 0.8 * cm;

    // Начальная точка по Y для всех полос (одинаковая для обеих групп)
    G4double startY = leftBottomY + offsetFromCorner;

    // -------- Первая группа: 10 полос, ширина 0.5 мм --------
    G4int nSlitsGroup1 = 10;
    G4double slitWidth1 = 0.5 * mm;

    // Стартовая X для первой группы (левый край первой полосы)
    G4double startX1 = leftBottomX + offsetFromCorner;

    // Текущее составное тело (начинаем с основного)
    G4VSolid* solidLeadWithSlits = solidLeadBase;

    // Вырезаем первую группу
    for (int i = 0; i < nSlitsGroup1; ++i) {
        // Левый нижний угол i-й полосы
        G4double slitLowX = startX1;
        G4double slitLowY = startY + i * (slitWidth1 + gapBetweenSlits);
        // Центр полосы
        G4double slitCenterX = slitLowX + 0.5 * slitLength;
        G4double slitCenterY = slitLowY + 0.5 * slitWidth1;

        G4Box* slitBox = new G4Box("slit1_" + std::to_string(i), 0.5 * slitLength, 0.5 * slitWidth1, slitHalfZ);
        G4Transform3D transform = G4Translate3D(slitCenterX, slitCenterY, 0.0);
        solidLeadWithSlits = new G4SubtractionSolid("leadWithSlits1_" + std::to_string(i), solidLeadWithSlits, slitBox, transform);
    }

    // -------- Вторая группа: 6 полос, ширина 0.9 мм, расстояние между полосами = ширине (0.9 мм) --------
    G4int nSlitsGroup2 = 6;
    G4double slitWidth2 = 0.9 * mm;
    G4double stepY2 = slitWidth2 + slitWidth2;   // шаг по Y (ширина + зазор = 0.9+0.9)

    // Правый край первой группы полос
    G4double rightEdgeGroup1 = startX1 + slitLength;   // предполагается, что startX1 и slitLength определены ранее
    G4double gapToRight = 0.8 * cm;
    G4double startX2 = rightEdgeGroup1 + gapToRight;

    // Вырезаем вторую группу
    for (int i = 0; i < nSlitsGroup2; ++i) {
        G4double slitLowX = startX2;
        G4double slitLowY = startY + i * stepY2;      // вертикальный шаг = 2 * ширина
        G4double slitCenterX = slitLowX + 0.5 * slitLength;
        G4double slitCenterY = slitLowY + 0.5 * slitWidth2;

        G4Box* slitBox = new G4Box("slit2_" + std::to_string(i), 0.5 * slitLength, 0.5 * slitWidth2, slitHalfZ);
        G4Transform3D transform = G4Translate3D(slitCenterX, slitCenterY, 0.0);
        solidLeadWithSlits = new G4SubtractionSolid("leadWithSlits2_" + std::to_string(i), solidLeadWithSlits, slitBox, transform);
    }

    // -------- Третья группа: 6 полос, ширина 1.0 мм, расстояние между полосами = ширине (1.0 мм) --------
    G4int nSlitsGroup3 = 6;                // количество полос в третьей группе
    G4double slitWidth3 = 1.0 * mm;        // ширина полосы
    G4double stepY3 = slitWidth3 + slitWidth3;   // шаг по Y: ширина + зазор = 2*ширина

    // Правый край второй группы полос (предполагается, что startX2 и slitLength определены ранее)
    G4double rightEdgeGroup2 = startX2 + slitLength;   // правый край второй группы
    G4double gapToRight2 = 0.8 * cm;                  // расстояние от правого края второй группы до левого края третьей
    G4double startX3 = rightEdgeGroup2 + gapToRight2; // стартовая X для третьей группы

    // Вырезаем третью группу
    for (int i = 0; i < nSlitsGroup3; ++i) {
        G4double slitLowX = startX3;
        G4double slitLowY = startY + i * stepY3;      // вертикальный шаг
        G4double slitCenterX = slitLowX + 0.5 * slitLength;
        G4double slitCenterY = slitLowY + 0.5 * slitWidth3;

        G4Box* slitBox = new G4Box("slit3_" + std::to_string(i), 0.5 * slitLength, 0.5 * slitWidth3, slitHalfZ);
        G4Transform3D transform = G4Translate3D(slitCenterX, slitCenterY, 0.0);
        solidLeadWithSlits = new G4SubtractionSolid("leadWithSlits3_" + std::to_string(i), solidLeadWithSlits, slitBox, transform);
    }

    // -------- Четвертая группа: 6 полос, ширина 2.0 мм, расстояние между полосами = ширине (2.0 мм) --------
    G4int nSlitsGroup4 = 6;                // количество полос в четвертой группе
    G4double slitWidth4 = 2.0 * mm;        // ширина полосы
    G4double stepY4 = slitWidth4 + slitWidth4;   // шаг по Y: ширина + зазор (= 2*ширина)

    // Правый край третьей группы полос (предполагается, что startX3 и slitLength определены ранее)
    G4double rightEdgeGroup3 = startX3 + slitLength;
    G4double gapToRight3 = 0.8 * cm;                  // отступ от правого края третьей группы
    G4double startX4 = rightEdgeGroup3 + gapToRight3;

    // Вырезаем четвертую группу
    for (int i = 0; i < nSlitsGroup4; ++i) {
        G4double slitLowX = startX4;
        G4double slitLowY = startY + i * stepY4;
        G4double slitCenterX = slitLowX + 0.5 * slitLength;
        G4double slitCenterY = slitLowY + 0.5 * slitWidth4;

        G4Box* slitBox = new G4Box("slit4_" + std::to_string(i), 0.5 * slitLength, 0.5 * slitWidth4, slitHalfZ);
        G4Transform3D transform = G4Translate3D(slitCenterX, slitCenterY, 0.0);
        solidLeadWithSlits = new G4SubtractionSolid("leadWithSlits4_" + std::to_string(i), solidLeadWithSlits, slitBox, transform);
    }

    // -------- Создание логического и физического объёмов --------
    G4LogicalVolume* logicLead = new G4LogicalVolume(solidLeadWithSlits, leadMat, "logicLead");
    G4VPhysicalVolume* physLead = new G4PVPlacement(0, G4ThreeVector(0., 0., 10. * cm), logicLead, "physLead", logicWorld, false, checkOverlaps);

    // Визуализация пластины
    G4VisAttributes* leadVisAtt = new G4VisAttributes(G4Color(1.0, 0.0, 0.0, 0.5));
    leadVisAtt->SetForceSolid(true);
    logicLead->SetVisAttributes(leadVisAtt);

    // ========== СЦИНТИЛЛЯЦИОННАЯ ПЛАСТИНА ИЗ CsI ==========
    G4double csiThickness = 0.5 * mm;
    G4double csiSizeX = 10.0 * cm;
    G4double csiSizeY = 5.0 * cm;

    // Создаём материал CsI
    G4Material* csiMat = new G4Material("CsI", 4.51 * g / cm3, 2);
    csiMat->AddElement(nist->FindOrBuildElement("Cs"), 1);
    csiMat->AddElement(nist->FindOrBuildElement("I"), 1);

    // Добавляем оптические свойства
    G4MaterialPropertiesTable* csiMPT = new G4MaterialPropertiesTable();

    const G4int nEnergies = 6;
    G4double photonEnergies[nEnergies] = { 1.91 * eV, 2.07 * eV, 2.27 * eV, 2.48 * eV, 2.76 * eV, 3.10 * eV };
    G4double refractiveIndex[nEnergies] = { 1.78, 1.79, 1.80, 1.81, 1.82, 1.83 };
    csiMPT->AddProperty("RINDEX", photonEnergies, refractiveIndex, nEnergies);

    csiMPT->AddConstProperty("SCINTILLATIONYIELD", 54000.0 / MeV);
    csiMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1000.0 * ns);
    csiMPT->AddConstProperty("RESOLUTIONSCALE", 1.0);

    const G4int nScint = 7;
    G4double scintEnergies[nScint] = { 2.07 * eV, 2.18 * eV, 2.30 * eV, 2.43 * eV, 2.58 * eV, 2.76 * eV, 2.95 * eV };
    G4double scintIntensity[nScint] = { 0.05, 0.2, 0.5, 0.8, 1.0, 0.4, 0.1 };
    csiMPT->AddProperty("SCINTILLATIONCOMPONENT1", scintEnergies, scintIntensity, nScint);

    // Реалистичные значения для CsI
    const G4int nAbs = 2;
    G4double absEnergies[] = { 2.0 * eV, 3.1 * eV };
    G4double absLength[] = { 3.0 * cm, 3.0 * cm };  // ~3 см для сцинтилляционного света
    csiMPT->AddProperty("ABSLENGTH", absEnergies, absLength, nAbs);

    csiMat->SetMaterialPropertiesTable(csiMPT);

    // Создаём геометрию CsI
    G4Box* solidCsI = new G4Box("solidCsI", 0.5 * csiSizeX, 0.5 * csiSizeY, 0.5 * csiThickness);
    logicCsI = new G4LogicalVolume(solidCsI, csiMat, "logicCsI");

    // Позиция CsI (центр сцинтиллятора)
    G4double csiPosZ = 0.165 * m;
    G4VPhysicalVolume* physCsI = new G4PVPlacement(0, G4ThreeVector(0. * m, -0.025 * m, csiPosZ),
        logicCsI, "physCsI", logicWorld, false, 2, checkOverlaps);

    // Визуализация CsI
    G4VisAttributes* csiVisAtt = new G4VisAttributes(G4Color(0.0, 1.0, 0.0, 0.6));
    csiVisAtt->SetForceSolid(true);
    logicCsI->SetVisAttributes(csiVisAtt);

    // ========== КРЕМНИЕВЫЙ ДЕТЕКТОР (ВПЛОТНУЮ К CsI) ==========
    G4Material* siMat = nist->FindOrBuildMaterial("G4_Si");

    // Оптические свойства для кремния
    G4MaterialPropertiesTable* siMPT = new G4MaterialPropertiesTable();

    const G4int nSiEnergies = 3;
    G4double siEnergies[] = { 1.5 * eV, 2.5 * eV, 3.5 * eV };
    G4double siRindex[] = { 3.5, 4.0, 5.0 };        // Показатель преломления кремния
    G4double siAbsLength[] = { 500 * um, 50 * um, 10 * um }; // Длина поглощения

    siMPT->AddProperty("RINDEX", siEnergies, siRindex, nSiEnergies);
    siMPT->AddProperty("ABSLENGTH", siEnergies, siAbsLength, nSiEnergies);

    // Квантовая эффективность кремния
    G4double siEfficiency[] = { 0.5, 0.8, 0.9 };
    siMPT->AddProperty("EFFICIENCY", siEnergies, siEfficiency, nSiEnergies);

    siMat->SetMaterialPropertiesTable(siMPT);

    // Создаем кремниевый детектор (такого же размера как CsI)
    G4double detectorSizeX = 10.0 * cm;
    G4double detectorSizeY = 5.0 * cm;
    G4double detectorThickness = 0.5 * mm;

    G4Box* solidDetector = new G4Box("solidDetector",
        0.5 * detectorSizeX,
        0.5 * detectorSizeY,
        0.5 * detectorThickness);
    logicDetector = new G4LogicalVolume(solidDetector, siMat, "logicDetector");

    // ВАЖНО: Размещаем детектор БЕЗ ЗАЗОРА
    // Задняя поверхность CsI находится на Z = csiPosZ + csiThickness/2
    // Передняя поверхность детектора должна быть на этой же позиции
    // Центр детектора = задняя поверхность CsI + detectorThickness/2
    G4double detectorPosZ = csiPosZ + (csiThickness / 2.0) + (detectorThickness / 2.0);

    G4VPhysicalVolume* physDetector = new G4PVPlacement(0,
        G4ThreeVector(0. * m, -0.025 * m, detectorPosZ),
        logicDetector, "physDetector", logicWorld, false, 1, checkOverlaps);

    // Визуализация детектора
    G4VisAttributes* siVisAtt = new G4VisAttributes(G4Color(0.0, 0.0, 1.0, 0.6));
    siVisAtt->SetForceSolid(true);
    logicDetector->SetVisAttributes(siVisAtt);

    // ========== ОПТИЧЕСКАЯ ПОВЕРХНОСТЬ МЕЖДУ CsI И Si ==========
// Создаем MaterialPropertiesTable для поверхности
    G4MaterialPropertiesTable* surfaceMPT = new G4MaterialPropertiesTable();

    const G4int nInterfEnergies = 3;
    G4double interfEnergies[] = { 1.5 * eV, 2.5 * eV, 3.5 * eV };
    G4double reflectivity[] = { 0.05, 0.05, 0.05 };   // 5% отражение
    G4double transmittance[] = { 0.95, 0.95, 0.95 };  // 95% пропускание

    // Добавляем свойства в MaterialPropertiesTable (а не напрямую в G4OpticalSurface)
    surfaceMPT->AddProperty("REFLECTIVITY", interfEnergies, reflectivity, nInterfEnergies);
    surfaceMPT->AddProperty("TRANSMITTANCE", interfEnergies, transmittance, nInterfEnergies);

    // Создаем оптическую поверхность
    G4OpticalSurface* csSiInterface = new G4OpticalSurface("CsI_Si_interface");
    csSiInterface->SetType(dielectric_dielectric);
    csSiInterface->SetModel(unified);
    csSiInterface->SetFinish(polished);

    // Присоединяем таблицу свойств к поверхности
    csSiInterface->SetMaterialPropertiesTable(surfaceMPT);

    // Применяем поверхность к границе между CsI и детектором
    new G4LogicalBorderSurface("CsI_Si_border", physCsI, physDetector, csSiInterface);

    return physWorld;
}

void PMDetectorConstruction::ConstructSDandField()
{
    PMSensitiveDetector* sensDet = new PMSensitiveDetector("SensitveDetector");


    if (logicDetector) {
        logicDetector->SetSensitiveDetector(sensDet);
        G4cout << "Main detector set as sensitive detector" << G4endl;
    }
    else {
        G4cerr << "WARNING: logicDetector is null!" << G4endl;
    }

    G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);
}