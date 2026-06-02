#include "PMPrimaryGenerator.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "Randomize.hh"
#include "G4RunManager.hh"
#include "G4AutoLock.hh"
#include <iostream>

std::atomic<G4int>  PMPrimaryGenerator::fGlobalPixelX(0);
std::atomic<G4int>  PMPrimaryGenerator::fGlobalPixelY(0);
std::atomic<G4int>  PMPrimaryGenerator::fParticlesEmittedInCurrentPixel(0);
std::atomic<G4bool> PMPrimaryGenerator::fIsFinished(false);

// Параметры сканирования — задаются через макрос командами:
//   /gun/gridSize <N>           → N×N пикселей (разрешение)
//   /gun/particlesPerPixel <N>  → квантов на пиксель (статистика)
// Итого событий = gridSize * gridSize * particlesPerPixel
// Значение beamOn должно быть >= этого числа (рекомендуется с запасом 2×)
G4int PMPrimaryGenerator::fParticlesPerPixel = 3;
G4int PMPrimaryGenerator::fGridSize          = 100;

G4double energy = 0.;
G4Mutex pixelMutex;

void PMPrimaryGenerator::ResetGrid()
{
    G4AutoLock lock(&pixelMutex);
    fGlobalPixelX                  = 0;
    fGlobalPixelY                  = 0;
    fParticlesEmittedInCurrentPixel = 0;
    fIsFinished                    = false;
}

PMPrimaryGenerator::PMPrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1);

    // Тип частицы: гамма-квант
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle("gamma");

    G4ThreeVector pos(0., 0., 0.);
    G4ThreeVector mom(0., 0., 1.);

    fParticleGun->SetParticlePosition(pos);
    fParticleGun->SetParticleMomentumDirection(mom);
    fParticleGun->SetParticleEnergy(30. * keV);
    fParticleGun->SetParticleDefinition(particle);
    // UI-мессенджер для /gun/gridSize и /gun/particlesPerPixel
    // регистрируется в PMRunAction (master-поток), а не здесь.
}

PMPrimaryGenerator::~PMPrimaryGenerator()
{
    delete fParticleGun;
}

G4int PMPrimaryGenerator::GetCurrentPixelX()
{
    return fGlobalPixelX.load();
}

G4int PMPrimaryGenerator::GetCurrentPixelY()
{
    return fGlobalPixelY.load();
}

void PMPrimaryGenerator::SetSourcePosition(G4double x, G4double y)
{
    G4ThreeVector pos(x, y, 0.);
    fParticleGun->SetParticlePosition(pos);
}

// ������� ��� ��������� ������ ���� ��������� �� �������
// ��������� � �������� �� SensitiveDetector
G4double GetDetectorBinCenter(int index, G4double size, int numBins)
{
    G4double step = (2.0 * size) / numBins;  // step = 10 �� / 25 = 0.4 ��
    G4double position = -size + (index + 0.5) * step;
    return position;
}

void PMPrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
    // ������� �������� �����
    if (fIsFinished.load()) {
        return;
    }

    G4int currentX, currentY;
    G4bool shouldGenerate = false;
    G4bool needAbort = false;

    // ����������� ������ ��� ���������� �������
    {
        G4AutoLock lock(&pixelMutex);

        if (fIsFinished.load()) return;

        currentX = fGlobalPixelX.load();
        currentY = fGlobalPixelY.load();
        G4int particlesInPixel = fParticlesEmittedInCurrentPixel.load();

        if (particlesInPixel >= fParticlesPerPixel) {
            // Переходим к следующему пикселю
            fParticlesEmittedInCurrentPixel = 0;
            currentX++;

            if (currentX >= fGridSize) {
                currentX = 0;
                currentY++;

                if (currentY >= fGridSize) {
                    fIsFinished = true;
                    G4cout << "[Progress] 100% — все пиксели обработаны." << G4endl;
                    needAbort = true;
                    lock.unlock();
                    G4RunManager::GetRunManager()->AbortRun();
                    return;
                }
            }

            fGlobalPixelX = currentX;
            fGlobalPixelY = currentY;

            // Прогресс-индикатор: вывод каждые 10%
            const G4int totalPixels = fGridSize * fGridSize;
            const G4int progressStep = totalPixels / 10;  // каждые 10%
            G4int pixelsDone = currentY * fGridSize + currentX;
            if (progressStep > 0 && pixelsDone % progressStep == 0 && pixelsDone > 0) {
                G4int percent = (pixelsDone * 100) / totalPixels;
                G4cout << "[Progress] " << percent << "% ("
                       << pixelsDone << "/" << totalPixels << " пикселей)" << G4endl;
            }
        }

        // ����������� �������
        fParticlesEmittedInCurrentPixel++;
        shouldGenerate = true;
    }

    // ���������� ������� ������ ���� �� ���������
    if (shouldGenerate && !fIsFinished.load()) {
        // ���������� �� �� ����� �������, ��� � � SensitiveDetector
        const G4double range = 5.0/100 * cm;   
        const G4int numBins = fGridSize;           // 25 �����

        // ��������� ����� ���� ���������
        G4double x = GetDetectorBinCenter(currentX, range, numBins);
        G4double y = GetDetectorBinCenter(currentY, range, numBins);

        SetSourcePosition(x, y);
        energy = fParticleGun->GetParticleEnergy();

        // ���������� �������
        fParticleGun->GeneratePrimaryVertex(anEvent);

    }
}