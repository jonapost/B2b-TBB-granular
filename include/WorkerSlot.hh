//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
#ifndef WORKERSLOT_HH
#define WORKERSLOT_HH

// #include <tbb/task.h>
// #include <tbb/concurrent_queue.h>
#include "G4Types.hh"

#include "G4Event.hh"

#include "tbbMasterRunManager.hh"
#include "tbbWorkerRunManager.hh"

class G4Run;

class WorkerSlot {  // : public tbb::task {
public:
  WorkerSlot(G4int anId, G4int nEvts = 1 );   
  virtual ~WorkerSlot();

  void SimulateOneEvent(G4Event &event);
    // Take 'event' after primary generation, and simulate its tracks

  void SimulateFullEventLoop( G4int numEvents );
    // Generate and Simulate numEvents

public:
  // Enabling methods

  // static WorkerSlot* ReserveWorkerSlot(unsigned int slotId);
    // Check that the slot 'slotId' is free, and reserve it
    // Return the pointer to it.
  
  void UseRNGEngine(CLHEP::HepRandomEngine* ptrRngEngine,
                    G4bool                  rngInitialized);
  void CreateLocalRNG_Engine();  // Create an engine

public:  // Was protected - changed to allow SimEventModule to use these methods
  // Sub-tasks - which will be used to construct the above methods
  void Initialize();
  void PrepareForBeam();

  void ProcessEventSimple( G4Event &event );  // No Event Generation
  // ProcessMultipleEvents();
  void CleanUpTask();
  void TerminateRun(); // Release run resources - there will be no more work

  // Optional methods - ie needed for simple example - may not be used
  // void GeneratePrimaries( G4Event &event );   // Use Generator Action
  G4Event* GeneratePrimaries( int eventId );

public:  // Minor Utility methods
  // Ensure that this thread is the sole one using the task
  //   - and that it is the unique task on the Current thread
  inline void MarkUsed();
  inline void MarkFree();
  inline void CheckUsed() { CheckOccupation(true); }
  inline void CheckFree() { CheckOccupation(false); }
  
  bool CheckOccupation(bool) const;  // Else complain and return false
  void SetOccupation(bool); 
  
  CLHEP::HepRandomEngine* CloneRNGEngine(
            const CLHEP::HepRandomEngine* existingRNG) const;
  void ConstructRNGEngine(const CLHEP::HepRandomEngine* masterRng );
  void InitialiseRNGEngine(long Seed1, int Seed2);

public:   // Methods required currently - Set must be called by external
  static void SetMasterRunManager(tbbMasterRunManager* masterRM)
  { m_MasterRunManager= masterRM; }
  static tbbMasterRunManager* GetMasterRunManager()
  { return m_MasterRunManager; }

private:
  // G4int  m_nEvents;  // Not needed ?
  G4int  m_thisID;
  // tbb::concurrent_queue<const G4Run*>* output;
  bool m_beamOnCondition;

  // bool m_createdRNG_Engine;
  // bool m_initializedRNG_Engine;

  bool m_eventGenerated;

  CLHEP::HepRandomEngine *mp_RngEngine;
  bool m_rngCreated;
  bool m_rngInitialised;

  bool m_taskOccupied;

  static tbbMasterRunManager* m_MasterRunManager;
  static G4ThreadLocal WorkerSlot*          t_localSlot;
  static G4ThreadLocal tbbWorkerRunManager* t_localRM;
   // The same for any task on this thread - only one may use it at a time
  static G4ThreadLocal G4int                t_thread_occupied;
};

void WorkerSlot::MarkUsed() { CheckOccupation(false); SetOccupation(true); }
void WorkerSlot::MarkFree() { CheckOccupation(true);  SetOccupation(false); }


  
#endif //WorkerSlot_HH
