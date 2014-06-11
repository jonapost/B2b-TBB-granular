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
//  Author: J. Apostolakis (CERN)
//
// First version:  Dec 2013
// Revised:  May 2014
//

#include "WorkerSlot.hh"
#include "tbbWorkerRunManager.hh"
#include "G4Threading.hh"
#include "G4String.hh"
#include "G4WorkerRunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4UserWorkerThreadInitialization.hh"
#include "G4WorkerThread.hh"
#include "G4MTRunManagerKernel.hh"
#include "G4AutoLock.hh"
#include "G4UserWorkerInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4Event.hh"

#include "tbbWorkerRunManager.hh"
#include "tbbMasterRunManager.hh"

G4ThreadLocal tbbWorkerRunManager* WorkerSlot::t_localRM = 0;
// G4ThreadLocal G4bool               WorkerSlot::t_RMoccupied = false;
tbbMasterRunManager*  WorkerSlot::m_MasterRunManager= 0;

//Equivalent to G4MTRunManagerKernel::StartThread

#define COUNT_WORKERS 1 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

WorkerSlot::WorkerSlot(G4int anId,
                       G4int nevts)
                     // tbb::concurrent_queue<const G4Run*>* out,
: m_thisID(anId),
  m_beamOnCondition(false),
  m_eventGenerated(false),
  mp_RngEngine(0),
  m_rngCreated(false),
  m_rngInitialised(false),
  m_taskOccupied(false)
{
  // Old way to identify the Worker - use counter
  // New way - obtain it from the Controling Framework
  // m_thisID = g_WorkerCount.fetch_and_increment();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

WorkerSlot::~WorkerSlot()
{
}

#ifdef COUNT_WORKERS
#include <tbb/atomic.h>
namespace {
    tbb::atomic<unsigned int> g_WorkersInUseCount;
}
#endif

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::SimulateOneEvent( G4Event &event)
{
   // Input condition: 'event' must contain output of the Event Generator
   m_eventGenerated = true;

   // G4int nEvents= 1;

   Initialize();
   PrepareForBeam();

   if ( m_beamOnCondition ) {
      // eventManager->ProcessEvent(currentEvent);
      this->ProcessEventSimple(event); 
      CleanUpTask();
      // Includes:  m_eventGenerated= false;
   }else{
      G4cerr << "ERROR in WorkerSlot::SimulateOneEvent: "
             << " Unable to set BeamOn condition." << G4endl;
   }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::SimulateFullEventLoop( G4int nEvents )
{        
   // Input condition: The Event Generator has not been called
   m_eventGenerated = false;

   Initialize();
   PrepareForBeam();
 
   // Simulate the event    
   assert(t_localRM!=0);
   if ( m_beamOnCondition ) {
      // Simple implementation -- Not tried yet
      //   t_localRM->DoEventLoop( m_nEvents );

      // Implementation calling ProcessEventSimple
      G4int i_event= 0; 
     while(  i_event < nEvents ) //  && eventLoopOnGoing )
     {
         // Adapted from 
         //  G4WorkerRunManager::DoEventLoop() and
         //        G4RunManager::ProcessOneEvent(int i_event)

         G4Event *pCurrentEvent = t_localRM->GenerateEvent(i_event);

         ProcessEventSimple( *pCurrentEvent);
         // eventManager->ProcessOneEvent(currentEvent);

         // Store Data from event, call RecordEvent
         // RecordEvent(*pCurrentEvent);
         // AnalyzeEvent(*pCurrentEvent);
       
         // Message Scoring Manager - if relevant
         // t_localRM->UpdateScoring();

         // Deal with standard optional arguments of Event Loop method - if used
         // if(pCurrentEvent->GetEventID()<n_select_msg)
         //   G4UImanager::GetUIpointer()->ApplyCommand(msgText);
         i_event++;
      }

      CleanUpTask();
   }else{
      G4cerr << "ERROR in WorkerSlot::SimulateMultipleEvent: "
             << " Unable to set BeamOn condition." << G4endl;
   }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::UseRNGEngine(CLHEP::HepRandomEngine* ptrRngEngine,
                              G4bool                  rngInitialized) 
{ 
   mp_RngEngine= ptrRngEngine;
   G4bool        rngExists = (ptrRngEngine != 0);
   m_rngInitialised= rngInitialized && rngExists;
  
   m_rngCreated= rngExists;  // May change to signify if 'newed'
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::CreateLocalRNG_Engine()     
{
  tbbMasterRunManager* masterRM= // t_localRM->GetMasterRunManager();
       m_MasterRunManager;
  
   //RNG Engine is created by "cloning" the master one.
   const CLHEP::HepRandomEngine* masterEngine =
                 masterRM->getMasterRandomEngine();
   if( masterEngine != 0 ){
     masterRM->GetUserWorkerThreadInitialization()
                   ->SetupRNGEngine(masterEngine);
   }else{
     G4Exception("WorkerSlot::CreateLocalRNG_Engine", "WorkerSlot-031", 
        FatalException, "Master Run Manager has no RNG Engine");
   }
   m_rngCreated= true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::ProcessEventSimple( G4Event &event)
{
   // Method of G4RunManager:
   //   virtual void ProcessOneEvent(G4int i_event);

#ifdef COUNT_WORKERS
  g_WorkersInUseCount++;
#endif
  
   t_localRM->tbbWorkerRunManager::ProcessAnEvent(event);
  
#ifdef COUNT_WORKERS
  g_WorkersInUseCount--;
#endif
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Event* WorkerSlot::GeneratePrimaries( int eventId ) // G4Event &event;
{
#ifdef COUNT_WORKERS
  g_WorkersInUseCount++;
#endif
  G4Event* pEvent= 0;
  const G4VUserPrimaryGeneratorAction *userPrimaryGeneratorAction=
    t_localRM->GetUserPrimaryGeneratorAction();
  if( userPrimaryGeneratorAction ){
    // userPrimaryGeneratorAction->GeneratePrimaries(event);
    
    pEvent= t_localRM->GenerateEvent(eventId);
    // Note: this call will re-seed the RNG engine ... in MT
  
    
  }else{
    G4Exception("WorkerSlot::GenerateEventSimple", "WorkerSlot-032", 
      FatalException, "No User Primary Generator Action registered"); 
  }
#ifdef COUNT_WORKERS
  g_WorkersInUseCount--;
#endif
  return pEvent;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4Event.hh"

// Copy the primaries from the new event to ours ... ?
// G4bool Event_Copy(G4Event *left, const G4Event &right);
// Event_Copy( &event, *pEvent);

G4bool Event_Copy(G4Event *left, const G4Event &right)
{
  left->SetEventID(right.GetEventID());
 
 
  // Ver. 1.) PrimaryVertex is a linked list - we just needs its head!
  // left->AddPrimaryVertex( right.GetPrimaryVertex() );
  // left->SetNumberOfPrimaryVertices(right.GetNumberOfPrimaryVertex());


  // Ver 2.) Just takes over the chain/list of primaries.
  // Traverses down the tree - does not copy, just adds existing next one!
  G4PrimaryVertex* primaryVertex= right.GetPrimaryVertex();
  while( primaryVertex != 0){
    left->AddPrimaryVertex(primaryVertex);
    primaryVertex= primaryVertex->GetNext();
  }
  
  // G4int numberOfPrimaryVertex;
  // Swap HitsCollection:  G4HCofThisEvent* HC;
  // Swap DigiCollection:  G4DCofThisEvent* DC;
  // TrajectoryContainer:  G4TrajectoryContainer * trajectoryContainer;
  // G4bool eventAborted;
  // UserEventInformation (optional)
  // G4VUserEventInformation* userInfo;
  // Initial random number engine status before primary particle generation
  // G4String* randomNumberStatus;
  // G4bool validRandomNumberStatus;
  // Initial random number engine status before event processing
  // G4String* randomNumberStatusForProcessing;
  // G4bool validRandomNumberStatusForProcessing;
  // Flag to keep the event until the end of run
  // G4bool keepTheEvent;

  left-> SetHCofThisEvent((G4HCofThisEvent*) right.GetHCofThisEvent());
  // right.SetHCofThisEvent((G4HCofThisEvent*) 0);

  left-> SetDCofThisEvent((G4DCofThisEvent*) right.GetDCofThisEvent());
  // right.SetDCofThisEvent((G4DCofThisEvent*) 0);
  
  // left-> SetDCofThisEvent(G4DCofThisEvent*);
  // left-> SetTrajectoryContainer(G4TrajectoryContainer*value);
  // left-> SetEventAborted();
  // left-> SetRandomNumberStatus(G4String& st);
  // left-> SetRandomNumberStatusForProcessing(G4String& st);
  // left-> KeepTheEvent(G4bool vl=true);
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//Equivalent to G4MTRunManagerKernel::StartThread

void WorkerSlot::Initialize()
{
   // Responsibilities: Setup per-thread objects
   //   i) 'dumb' ones to use a per-thread work area 
   //  ii) compatible ones to use the workspace corresponding to our index
   // iii) call User Worker Initialization - initializes other elements

   // From tbb::task* WorkerSlot::execute()
   // *************************************

   // In tbb a task is run "somewhere", on one of a pool of threads.
   // There is no control over which thread.
   // The thread is created, controlled and is 'switched' to our task 
   // (or another) by the TBB runtime system. 
   // IMPORTANT Assumption> The task will completed on the current 
   //   thread before the thread is used for anything else, or at least
   //   before the thread is used for another of our tasks.
   // 
   // In the current version to avoid this we initialize only
   //   i) the first time we run on each thread, and
   //  ii) at the start of a new run on each thread (reinitialization).
   // We are confident that this works because TBB co-works with TLS.
   // In addition this ensures that minimal changes needed to G4 code base.

   // Get exclusive use of this task - cross check
   // MarkUsed();
   CheckUsed();

   //Is this task running for the first time?
   // OPEN Issue> How to re-initialize between runs ??   TODO
   if (! t_localRM ) {
      //It's a new thread, basically repeat what is being done in 
      //G4MTRunManagerKernel::StarThread adapted for the TBB context.
      //Important differences: do not process data or wait for master to
      //communicate what to do, it will not happen!
      G4MTRunManager* masterRM = G4MTRunManager::GetMasterRunManager();
      //============================
      //Step-0: Thread ID
      //============================
      //Initliazie per-thread stream-m_output
      //The following line is needed before we actually do I/O initialization
      //because the constructor of UI manager resets the I/O destination.

      G4Threading::G4SetThreadId( m_thisID );
      G4UImanager::GetUIpointer()->SetUpForAThread( m_thisID );

      //============================
      //Step-1: Random number engine
      //============================
      if( !m_rngCreated)
      {
         ConstructRNGEngine(masterRM->getMasterRandomEngine());
      }
      //============================
      //Step-2: Initialize worker thread
      //============================
      if(masterRM->GetUserWorkerInitialization())
         masterRM->GetUserWorkerInitialization()->WorkerInitialize();
      if(masterRM->GetUserActionInitialization()) 
      {
          G4VSteppingVerbose* sv =
          masterRM->GetUserActionInitialization()->InitializeSteppingVerbose();
          if (sv) G4VSteppingVerbose::SetInstance(sv);
      }
      //Now initialize worker part of shared objects (geometry/physics)
      G4WorkerThread::BuildGeometryAndPhysicsVector();
      t_localRM = static_cast<tbbWorkerRunManager*>(
               masterRM->GetUserWorkerThreadInitialization()
                       ->CreateWorkerRunManager()
                                                   );
      G4cout << " Local RunManager created. Address is " << t_localRM << G4endl;
      //t_localRM->SetWorkerThread(wThreadContext);
      //    G4AutoLock wrmm(&workerRMMutex);
      //    G4MTRunManagerKernel::workerRMvector->push_back(t_localRM); 
      //             <<<<?????? ANDREA TBB
      //    wrmm.unlock();

      //================================
      //Step-3: Setup worker run manager
      //================================
      // Set the detector and physics list for worker thread. Share with master
      G4VUserDetectorConstruction* detectorCtion = 
           const_cast<G4VUserDetectorConstruction*>(
               masterRM->GetUserDetectorConstruction());
      
      t_localRM->G4RunManager::SetUserInitialization(detectorCtion);

      G4VUserPhysicsList* physicslist = 
        const_cast<G4VUserPhysicsList*>( masterRM->GetUserPhysicsList() );
      
      t_localRM->SetUserInitialization(physicslist);
       
      //================================
      //Step-4: Initialize worker run manager
      //================================
      if(masterRM->GetUserActionInitialization())
      {   
         masterRM->GetNonConstUserActionInitialization()->Build(); 
      }
      if(masterRM->GetUserWorkerInitialization())
      {
         masterRM->GetUserWorkerInitialization()->WorkerStart(); 
      }
      t_localRM->Initialize();  
   }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::PrepareForBeam() // Was ( G4int nEvents ) - do not need nEvents
{
   G4MTRunManager* masterRM = G4MTRunManager::GetMasterRunManager();

   // Execute UI commands stored in the masther UI manager
   // 
   // Problem: if there was more than one run...
   //          the commands of previous runs (lost) would
   //          be relevant - for a thread which did not
   //          participate in the first run.  
  
   std::vector<G4String> cmds = masterRM->GetCommandStack();
   G4UImanager* uimgr = G4UImanager::GetUIpointer(); //TLS instance
   std::vector<G4String>::const_iterator it = cmds.begin();
   for(;it!=cmds.end();it++) {
      uimgr->ApplyCommand(*it); 
   }
   //Start this run
   G4String macroFile = masterRM->GetSelectMacro();
   G4int numSelect = masterRM->GetNumberOfSelectEvents();
   G4int nEvents= 0;
   if ( macroFile == "" || macroFile == " " )
   {
      t_localRM->BeamOn(nEvents,0,numSelect);
   }else{
      t_localRM->BeamOn(nEvents,macroFile,numSelect);
   }
   //======= NEW TBB SPECIFIC ===========
   //Step-5: Initialize and start run
   //====================================
   // This is basically BeamOn 
   m_beamOnCondition = t_localRM->ConfirmBeamOnCondition();
   if (m_beamOnCondition) {
      t_localRM->SetNumberOfEventsToBeProcessed( nEvents );
      t_localRM->ConstructScoringWorlds(); 
      t_localRM->RunInitialization();
      //Register this G4Run in m_output
      //Note: the idea is that we are going to accumulate everything in 
      //G4Run or derived class. We let the kernel know this is the object
      //where the m_output is accumulated for the tbb::tasks that run on 
      //this thread.
      // if ( m_output ) {}
      // TODO : First implementation of m_output
   }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::CleanUpTask()
{
   m_eventGenerated= false; 
   // m_rngCreated= false;
   m_rngInitialised= false;
 
   // m_taskOccupied= false;
   MarkFree(); 

   // G4RunManager::TerminateOneEvent()
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::TerminateRun()
{
   // It is responsibility of the framework when to call this when 
   //   all the work is finished.
   //
   // If it is called, it must:
   //   - check whether output of worker was collected (future) - else report
   //   - release resources utilised by this worker slot
   //   
   t_localRM->RunTermination(); 

   //Can be done only if thread will never be re-used by any task
   //===============================
   //Step-6: Terminate worker thread(s)
   //===============================
  // if(masterRM->GetUserWorkerInitialization())
  // { masterRM->GetUserWorkerInitialization()->WorkerStop(); }

  // wrmm.lock();
  // std::vector<G4WorkerRunManager*>::iterator itrWrm =workerRMvector->begin();
  // for(;itrWrm!=workerRMvector->end();itrWrm++)
  // {
  //   if((*itrWrm)==wrm)
  //   {
  //     workerRMvector->erase(itrWrm);
  //     break;
  //   }
  // }
  // wrmm.unlock();
    
  // wThreadContext->DestroyGeometryAndPhysicsVector();
  // wThreadContext = 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// The following methods could be made robust (protected)
//   or deleted.
// They are meant as a simple sanity check - of exclusion which is
//  enforced through other means.

bool WorkerSlot::CheckOccupation(bool expectedValue) const
{
   // Else complain and return false
   bool agree = ( m_taskOccupied != expectedValue );
   if( !agree )
   {
      G4cerr << " ERROR> WorkerSlot::CheckOccupation failed. " << G4endl
             << "        Expected value= "  << expectedValue 
             << "   Current state= "  << m_taskOccupied << G4endl;
     // exit(1);
   }
   return agree;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::SetOccupation(bool val)
{
  // Else complain and return false
  if( m_taskOccupied == val )
  {
    G4cerr << " ERROR> WorkerSlot::SetOccupation failed. " << G4endl
    << "        Already have occupied = "  << m_taskOccupied
    << "   Cannot set again to = "  << val << G4endl;
    exit(1);
  }
  m_taskOccupied= val;
}

// WorkerSlot* SimEventModule::ReserveWorkerSlot(unsigned int id) const

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WorkerSlot::ConstructRNGEngine(const CLHEP::HepRandomEngine* masterRng )
{
  CLHEP::HepRandomEngine *rngEngine;
  
  if( masterRng != 0){
    rngEngine= CloneRNGEngine(masterRng);
  }else{
    rngEngine= new CLHEP::MTwistEngine;
    G4ExceptionDescription msg;
    msg << "No master HepRandomEngine provided.  Chosen MTwistEngine"
    << G4endl;
    G4Exception("WorkerSlot::SetupRNGEngine()","WrkSlot101",FatalException,msg);
  }
  
  if( rngEngine != 0 ) {
    G4Random::setTheEngine( rngEngine );
  }else{
    G4ExceptionDescription msg;
    msg<< " Unable to complete setup of RNG Engine - " << G4endl;
    G4Exception("WorkerSlot::ConstructRNGEngine()",
                "WrkSlot901",FatalException,msg);
  }
  
  G4Random::setTheEngine( rngEngine );
  m_rngCreated= true;
}

namespace {
  G4Mutex rngWorkerCreateMutex = G4MUTEX_INITIALIZER;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

CLHEP::HepRandomEngine*
WorkerSlot::CloneRNGEngine(const CLHEP::HepRandomEngine* existingRNG) const
{
  //No default available, let's create the instance of random stuff
  //A Call to this just forces the creation to defaults
  G4Random::getTheEngine();
  //Poor man's solution to check which RNG Engine is used in master thread
  CLHEP::HepRandomEngine* retRNG= 0;
  G4bool   recognised= false;
  
  // Need to make these calls thread safe
  G4AutoLock l(&rngWorkerCreateMutex);
  if ( dynamic_cast<const CLHEP::HepJamesRandom*>(existingRNG) ) {
    retRNG= new CLHEP::HepJamesRandom;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::RanecuEngine*>(existingRNG) ) {
    retRNG= new CLHEP::RanecuEngine;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::Ranlux64Engine*>(existingRNG) ) {
    retRNG= new CLHEP::Ranlux64Engine;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::MTwistEngine*>(existingRNG) ) {
    retRNG= new CLHEP::MTwistEngine;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::DualRand*>(existingRNG) ) {
    retRNG= new CLHEP::DualRand;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::RanluxEngine*>(existingRNG) ) {
    retRNG= new CLHEP::RanluxEngine;
    recognised= true;
  }
  if ( dynamic_cast<const CLHEP::RanshiEngine*>(existingRNG) ) {
    retRNG= new CLHEP::RanshiEngine;
    recognised= true;
  }
  if( !recognised ) {
    // Look/request a new method, such as cloneEngine() to clone it
    G4ExceptionDescription msg;
    msg<< " Unknown type of RNG Engine - " << G4endl
    << " Can cope only with HepJamesRandom, Ranecu, Ranlux64, MTwistEngine, "
    << " DualRand, Ranlux or Ranshi." << G4endl
    << " Cannot clone this type of RNG engine, as required for this thread"
    << G4endl
    << " Aborting " << G4endl;
    G4Exception("WorkerSlot::CloneRNGEngine()",
                "WrkSlot902",JustWarning,msg);
  }else{
    if( retRNG == 0 ) {
      G4ExceptionDescription msg;

      msg<< " Unable to clone known RNG Engine - " << G4endl;
      G4Exception("WorkerSlot::CloneRNGEngine()",
                  "WrkSlot102",FatalException,msg);
    }
  }
  return retRNG;
}


