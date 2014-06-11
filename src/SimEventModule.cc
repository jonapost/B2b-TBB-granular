#include <stdio.h>

#include "SimEventModule.hh"
#include "WorkerSlot.hh"

#include "G4RunManagerKernel.hh"
#include "tbbMasterRunManager.hh"

#include "G4Event.hh"

#include "Event.hh"
#include "ExperimentRunManager.hh"

#include "CLHEP/Random/Random.h"


// Option 2) below
// Requires change in G4RunManager::G4RunManager() constructor
// The line:
//   kernel = new G4RunManagerKernel();
// must be replaced with:
// START-Replacement
// kernel = G4RunManagerKernel::GetRunManagerKernel();
// if( kernel = 0) {
//   kernel = new G4RunManagerKernel();
// }
// END-Replacement

static SimEventModule* ptrInstanceSEM= 0;

// Thread local pointers
G4ThreadLocal WorkerSlot*          SimEventModule::t_localSlot= 0;
G4ThreadLocal tbbWorkerRunManager* SimEventModule::t_localRM=0 ;

SimEventModule* SimEventModule::GetInstance()
{
    return ptrInstanceSEM;
}

// Must be instantianed in the main program -- the tasks require it to do the work

SimEventModule::SimEventModule(unsigned int iNumSimEvents) :
  m_kernels(),
  m_NumSimultaneous_Events(iNumSimEvents)
  // , m_MasterRunManager(0)
{
    if( ptrInstanceSEM ){
      // There can be only one
      G4cerr << " ERROR> : Cannot create more than one SimEventModule" << G4endl;
      exit(1);
    }else{
      ptrInstanceSEM = this;
    }
    // Make sure that core Geant4 is initialised
    //  -- all that is required before creating Worker Tasks

    G4RunManagerKernel* masterRunMgrKrnl=0;
    ExperimentRunManager* expRunMgr=0;
  
    // There can be only one G4RunManagerKernel class in each thread.
  
    // Here we create a TBB Master Run Manager - trying to reuse the RKM instance
    //     between the  TBB Master Run Manager and the Experiment's 'RunManager'
#if 1
    // There is a choice:
    //  Either
    // 1) the 'master' TBB Run Manager is created first, and the Kernel is
    //   reused for the experiment Run Manager (likelier to work with G4 unchanged,
    //    since the experiment's RunManager is not derived from G4RunManager).
    // tbbMasterRunManager* masterRM=
    CreateMasterRunManager();
     // m_MasterRunManager = masterRM;
    masterRunMgrKrnl= G4RunManagerKernel::GetRunManagerKernel();
    expRunMgr= new ExperimentRunManager( masterRunMgrKrnl );
#else
    //  Or
    //  2) the Experiment Run Manager is created first, and we adapt here
    //   trying to reuse the existing RunManagerKernel (hard, needs G4 revision!?)
    expRunMgr= new ExperimentRunManager( masterRunMgrKrnl );
    masterRunMgrKrnl= G4RunManagerKernel::GetRunManagerKernel();
    CreateMasterRunManager(masterRunMgrKrnl);
#endif
    // RegisterMandatoriesAndInitialize();

    // Create the Working Context for the Tasks
    //
    m_kernels.reserve(iNumSimEvents);
    for(unsigned int i=0; i<iNumSimEvents; ++i) {
        m_kernels.push_back( new WorkerSlot(i) );
    }

    //do all the needed initialization 
    // ...
}

SimEventModule::~SimEventModule()
{
    for(unsigned int i=0; i<m_NumSimultaneous_Events; ++i) {
       delete m_kernels[i];
       m_kernels[i]= 0;
    }
    delete WorkerSlot::GetMasterRunManager();
    WorkerSlot::SetMasterRunManager(0);
}

//   Choice: Experiment RunManager must use an existing
//     RunManagerKernel if one already exists 

tbbMasterRunManager* SimEventModule::CreateMasterRunManager() 
{
  // Create the Master 'RunManager', to hold the pointers to the
  // mandatory elements for the Run:
  //    Detector Construction
  //    Physics List
  //    User Actions ( changes needed )

  // ISSUE: in the current Geant4 (10.0), each thread may only own
  //       one G4RunManagerKernel object.
  //      Due to this it is not possible to have both a Master and a
  //       worker RunManager in the same thread.
  tbbMasterRunManager* masterRunMgr= 0;

  if( 1 ){
    masterRunMgr = new tbbMasterRunManager();
    // Note: Potentially this will be treated as a bucket for the pointers only
    //  - if the real 'Initialize()' call is done elsewhere
    WorkerSlot::SetMasterRunManager(masterRunMgr);
  }
#ifdef MASTER_IN_SEPARATE_THREAD
  // MasterRMspawner::CreateThreadForMasterRM();
#endif  

  return masterRunMgr;
}

void SimEventModule::RegisterMandatoriesAndInitialize()
{
    // This method replaces the 'main()' method of a Geant4 example
    //  It brings all the elements already created and registered 
    //  with the existing customised CMS Run Manager to a 
    //  custom Master RunManager (potentially TBBMasterRunManager)
    //  from which the Worker RunManagers can obtain them.
        

    // 1. Obtain the mandatory Experiment elements from CMS Run Manager
    //    a.) Detector Construction
    //    b.) Physics List
    //    c.) User Actions ===> Changed (see below)
    // G4MTRunManagerKernel*
    tbbMasterRunManager*
       masterRunManager = WorkerSlot::GetMasterRunManager();

    //  Code HERE to copy pointers into the tbb Master RM

    // 2. Specialised elements - existed but are done differently
    //    c.) User Action Initialization ( all 'action' objects: step, track, ... )
    //         These *must* be created once for each thread
    //         -> They used to be registered individually with RunManager,
    //            Refactored: the new class must create one instance of each one,
    
    //  Code HERE to register the User-Action-Initialization class with the RM

    // 3. New elements - specialised first for MT, further for tasks/TBB
    //  These will be handled by the G4 MT/TBB classes - see
    //                                   ExpRunControl::InitializeG4WorkerState

    // 4. Initialize the Geant4 'kernel' on the 'Master'
    masterRunManager->Initialize();
}


void SimEventModule::Process(G4Event& event, CLHEP::HepRandomEngine* ptrRngEngine, int slotId)
{
  
  WorkerSlot* mySlot;
  mySlot= ReserveWorkerSlot(slotId); // Add threadId ); ?
  mySlot= m_kernels[slotId];
  
  t_localSlot= mySlot;
  
  bool rngInitialized= true;
  
  // The state of the RNG on this thread or slotId must be set
  mySlot->UseRNGEngine( ptrRngEngine, rngInitialized);
  mySlot->ProcessEventSimple(event);
}



G4Event*  SimEventModule::GeneratePrimaries( int eventId )
{
   G4Event* pEventG4=0;
   // Dummy implementation of experiment's method to fill the event
   pEventG4 = t_localSlot->GeneratePrimaries( eventId );
   //
   return pEventG4;
}


// SimEventModule::CreateLocalRNG_Engine()


#include "SlotDispatcher.hh"

void SimEventModule::DispatchGenerateAndProcess(int eventId, int runId)  // Event& e
{
  
  // Prepare the G4Event currentEvent, using Event e
  // G4Event currentEvent(e.id());
  // fillCurrentEvent(currentEvent, e);
  
  G4int slotId = -1;
  
  if( slotId < 0 ){
    // Slot was not set by framework - in this case we manage them
    slotId= SlotDispatcher::GetInstance()->ObtainSlot();
    if( (slotId < 0) || (slotId>(int)m_NumSimultaneous_Events-1) ) {
      G4Exception("SimEventModule::Process(Event&)", "1", FatalException, "Erroneous Slot Id returned by WorkerSlot.");
    }
  }
  WorkerSlot* mySlot= m_kernels[slotId];
   
  G4Event *anEvent= mySlot->GeneratePrimaries(eventId);
  
  long seed1; int seed2int;
  ChooseSeeds(eventId, runId, &seed1, &seed2int);
  //each concurrent Event has been provided with its own seeds

  
  //LATER: each concurrent Event will its own index, so we can use that to find an
  //unused Kernel for this thread
  
  // Initialize Random Number Engine
  mySlot->CreateLocalRNG_Engine();
  CLHEP::HepRandomEngine* ptrRngEngine= G4Random::getTheEngine();
  
  ptrRngEngine->setSeeds(&seed1, seed2int);
  
  Process(*anEvent, ptrRngEngine, slotId);
  
  // Free the current Slot
  SlotDispatcher::GetInstance()->RelinquishSlot((unsigned int)slotId);
  t_localSlot= 0;
  // Call to Return Slot;
  
  // Do we hold onto the local RunManager ?   For now Yes!
  // t_localRM= 0;
}

void SimEventModule::ChooseSeeds(int eventId, int runId, long *seed1, int *seed2int)
{
  // Arbitrary choice of seeds - for toy runs
  // Choosing primes to reduce chance of collisions between runs
  //
  long baseSeed1=  278075881679; // 10987654321 th prime number
  long multSeed1E=  22507410677; //   987654321 th prime number
  long multSeed1R=   2543568463; //   123456789 th prime number
  *seed1=  multSeed1R * runId + multSeed1E * eventId + baseSeed1;
  
  // 22507410677 is the hypotenuse of a primitive Pythagorean triple:
  // 22507410677^2 = 4576861652^2+22037147565^2
  //
  // Nearest primes to 22037147565 are 22037147563 and 22037147579
  long baseSeed2=  22037147563;
  // Nearest primes to 4576861652 are 4576861651  |  4576861657
  long multSeed2R=  4576861652;
  long multSeed2E=   224284387;  // 12345678 th prime number

  long seed2Long=  multSeed2R * runId + multSeed2E * eventId + baseSeed2;
  *seed2int = seed2Long % 1<<31;
}

WorkerSlot* SimEventModule::ReserveWorkerSlot(unsigned int id)
{
  WorkerSlot*  workSlot= 0;
  
  if( id < m_kernels.size() )
  {
    workSlot= m_kernels.at(id);
    assert( workSlot != 0 );
    if( workSlot ) {
      workSlot->MarkUsed();
    }
    // if( ! workSlot->Free() ) { worker=0;}
  }
  // t_localSlot= workSlot;
  return workSlot;
}


