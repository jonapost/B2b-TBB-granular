#include <stdio.h>

#ifndef SIMEVENTMODULE_HH
#define SIMEVENTMODULE_HH 1

#include "Event.hh"
#include "G4Event.hh"

#include "WorkerSlot.hh"
class tbbMasterRunManager;
class tbbWorkerRunManager;
class WorkerSlot;

// This object was called 'GeantModule' by Chris Jones
//  Its missions are:
//    - to set up the simulation
//    - to keep a list of Worker-type instances ('WorkerTask' below)
//    - to pass the work of a task to a free  Worker-type instance (its identity
//         can be specified)
//  Each Worker-type instance must
//    - have the ability to simulate a Geant4 event
//    - has the use of an *independent* work area
//      (i.e. can create a working area or own a 'workspace'.)
//  Term: Independent = non-interfering with others
//
// Role: I see SimEventModule as a proxy for the Master Run Manager
//   a) It collects information from the framework (or elsewhere, such as main)
//    and sets up all other objects required to start/coordinate a run.
//   b) It is called within a framework Task to process an event - 
//      passing off to the relevant Sim Worker Task that work.

// Q> What is the type of class with which to replace 'G4RunManagerKernel'
//  in Chris Jones' sample code ? 

//   Chose the name WorkerTask for the 'kernels' which do the work
//    to reduce my confusion.
//   My hunch is that this object is strongly related to  *tbbWorkerRunManager*

class SimEventModule {
  
public:
    SimEventModule(unsigned int iNumSimEvents);
               //  Maximum number of concurrent workers 
   ~SimEventModule();

   // Key Interface
   // =============
   // Methods to be called concurrently
   //   each call will correspond to an open task slot
   //      (amongst m_NumSimultaneous_Events)
   // The framework guarantees that this event has the corresponding
   //   'concurrent index.'
   void Process(G4Event& ev, CLHEP::HepRandomEngine* ptrRngEngine, int slotId);

 
   // Methods which need to be customised for Framework
   // ===================================
   void RegisterMandatoriesAndInitialize();
   // Register all essential G4 operations - a la Run Manager
  
   inline void Build()      { RegisterMandatoriesAndInitialize(); }
   inline void Initialize() { RegisterMandatoriesAndInitialize(); }

   void DispatchGenerateAndProcess(int eventId, int runId);
   // Used to test by setting RNG engine, generating event and processing it
  
public:    // Enabling methods
   static SimEventModule* GetInstance();
   static tbbMasterRunManager* GetMasterRunManager()
        {return WorkerSlot::GetMasterRunManager();}
  
   WorkerSlot* ReserveWorkerSlot(unsigned int id);
    // Ensure that the slot is Free, and mark it as used
  
private:   // Internal methods
   // May be made public - if 'protocol' of use is revised
   tbbMasterRunManager* CreateMasterRunManager();
     // OR (G4RunManagerKernel* runMgrKernel);
     //  Use existing RunManager Kernel (experimental)
  
protected:  // Filler methods, when not used with Experiment framework
    G4Event* GeneratePrimaries( int eventId );
    // fill curEvent, using contents of expEvent
  void ChooseSeeds(int eventId, int runId, long *seed1, int *seed2int );

private:
  std::vector<WorkerSlot *> m_kernels; //one per simultaneous event
  unsigned int m_NumSimultaneous_Events;
  // m_MasterRunManager; // responsibility for it is in WorkerSlot
  
  // Thread local pointers
  static G4ThreadLocal WorkerSlot*          t_localSlot;
  static G4ThreadLocal tbbWorkerRunManager* t_localRM;
};

#endif  // SIMEVENTMODULE_HH
