#include "MasterRMspawner.hh"

#include "G4Autolock.hh"

class tbbMasterRunManager;

#if 0 // def MASTER_IN_SEPARATE_THREAD0
namespace {
  G4Mutex spawningThreadForMasterRM = G4MUTEX_INITIALIZER;
}

void MasterRMspawner::CreateThreadForMasterRM()
{
  G4bool    JustCreatedMasterRM= false;

  G4AutoLock l(&spawningThreadForMasterRM);
  static G4bool CreatedMasterRM= false;

  if( !CreatedMasterRM ){
     //Create the thread for Master and remember it
     G4WorkerThread* context = new G4WorkerThread;
     G4Thread* thread = CreateAndStartMaster(context);
     fThreadMasterRM=thread;

     CreatedMasterRM= true;
     JustCreatedMasterRM= true;
  }
}


#ifdef G4MULTITHREADED
G4Thread* MasterRMspawner::CreateAndStartMaster(G4WorkerThread* wTC)
{
  //Note: this method is called by G4MTRunManager, here we are still sequential
  //Create a new thread/worker structure
  G4Thread* master = new G4Thread;  // pthread_t
#ifdef WIN32
  G4THREADCREATE(master, (LPTHREAD_START_ROUTINE)&SimEventModule::InitializeMasterRM_thread , wTC );
#else
  G4THREADCREATE(master, &SimEventModule::InitializeMasterRM_thread , wTC );
#endif
  return worker;
}
#else
G4Thread* MasterRMspawner::CreateAndStartMaster(G4WorkerThread* wTC)
{
  return new G4Thread;
}
#endif

void G4WorkerThread::InitializeMasterRM_thread()
{
  // Create the Master 'RunManager', to hold the pointers to the
  // mandatory elements for the Run:
  //    Detector Construction
  //    Physics List
  //    User Actions ( changes needed )
  
  tbbMasterRunManager* masterRunMgr = new tbbMasterRunManager();
  // Note: Potentially this will be treated as a bucket for the pointers only
  //  - if the real 'Initialize()' call is done elsewhere
  
  WorkerSlot::SetMasterRunManager(masterRunMgr);
  
  return masterRunMgr;
}
#endif

tbbMasterRunManager* MasterRMspawner::StopMaster() 
{
}
