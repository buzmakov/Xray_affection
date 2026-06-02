#ifndef PMPRIMARYGENERATOR_HH
#define PMPRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"
#include "G4Threading.hh"
#include <atomic>

class PMPrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    PMPrimaryGenerator();
    ~PMPrimaryGenerator();

    virtual void GeneratePrimaries(G4Event *);

    void SetSourcePosition(G4double x, G4double y);

    // Параметры сканирования — настраиваются через макрос:
    //   /gun/gridSize <N>            (N×N пикселей, по умолчанию 100)
    //   /gun/particlesPerPixel <N>   (квантов на пиксель, по умолчанию 3)
    static G4int fParticlesPerPixel;
    static G4int fGridSize;

    // Сброс счётчиков перед каждым новым Run (вызывается из RunAction)
    static void ResetGrid();

private:
    G4ParticleGun *fParticleGun;

    static std::atomic<G4int>  fGlobalPixelX;
    static std::atomic<G4int>  fGlobalPixelY;
    static std::atomic<G4int>  fParticlesEmittedInCurrentPixel;
    static std::atomic<G4bool> fIsFinished;

    G4int GetCurrentPixelX();
    G4int GetCurrentPixelY();
};

extern G4double energy;

#endif