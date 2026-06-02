#ifndef PMRUNACTION_HH
#define PMRUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4GenericMessenger.hh"

class PMRunAction : public G4UserRunAction
{
public:
    PMRunAction();
    ~PMRunAction();

    virtual void BeginOfRunAction(const G4Run *);
    virtual void EndOfRunAction(const G4Run *);

private:
    // Мессенджер для команд /gun/gridSize и /gun/particlesPerPixel
    // Создаётся только на master-потоке (через BuildForMaster)
    static G4GenericMessenger* fGridMessenger;
};

#endif