//
//  Simple class to spawn thread that holds Master Run Manager
//    - this thread must do just enough to initialise the objects with the
//       master RM holds - which will be copied by other worker threads.
//
//  Author: J. Apostolakis
//  First version:   May 2014

#ifndef MasterRMspawner_h
#define MasterRMspawner_h

#include "G4Threading.hh"   // Define G4Thread: Cannot forward declare - it is a typedef
class G4WorkerThread;
class tbbMasterRunManager;

class MasterRMspawner 
{
  public: 
    static MasterRMspawner* GetInstance();
    void CreateThreadForMasterRM();

  public:
    G4Thread* CreateAndStartMaster(G4WorkerThread* wTC);

    void InitializeMasterRM_thread(); 
    tbbMasterRunManager* StopMaster(); 

  private:
    G4Thread* fThreadMasterRM;   //  G4Thread object corresponding to Master RM
    int       fThreadId;         //  Id for Thread (from OS) of the thread of 'MasterRM
};
#endif




