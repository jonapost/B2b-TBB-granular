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
// $Id: exampleB2b-TBB-granular.cc 80806 2014-05-12 15:01:12Z japost $
//
/// \file exampleB2b.cc
/// \brief Main program of the B2b example

#include "B2bDetectorConstruction.hh"
#include "B2ActionInitialization.hh"

//G4-TBB interfaces
#include "tbbUserWorkerInitialization.hh"
#include "tbbMasterRunManager.hh"
#include "G4Threading.hh"

#include "G4UImanager.hh"
#include "FTFP_BERT.hh"
#include "G4StepLimiterPhysics.hh"

#include "Randomize.hh"

#ifdef G4VIS_USE
#include "G4VisExecutive.hh"
#endif

#ifdef G4UI_USE
#include "G4UIExecutive.hh"
#endif

//TBB includes
#include <tbb/task_scheduler_init.h>
#include <tbb/task.h>

#include "SimEventModule.hh"
#include "tbbMasterRunManager.hh"

//This function is very simple: it just start tbb work.
//This is done in a seperate thread, because for G4 the
//master cannot live in the same thread where workers are
//Starting tbb work in a thread guarantees that no workers
//are created where the master lives (the main thread)
//Clearly a separate solution is to create and configure master
//in a separate thread. But this is much simpler.
G4ThreadFunReturnType startWork(G4ThreadFunArgType arg)
{
    tbb::task_list* tasks = static_cast<tbb::task_list*>(arg);
    //We assume at least one /run/beamOn was executed, thus the tasklist is now filled,
    //lets start TBB
    try {
        std::cout<<"Now calling 'tbb::task::spawn_work_and_wait' "<<std::endl;
        tbb::task::spawn_root_and_wait( *tasks );
    } catch(std::exception& e) {
        std::cerr<<"Error occurred. Error info is:\""<<e.what()<<"\""<<std::endl;
    }
    return static_cast<G4ThreadFunReturnType>(0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
  // Choose the Random engine

  G4Random::setTheEngine(new CLHEP::RanecuEngine);
  
  unsigned int numCoresAvailable=  G4Threading::G4GetNumberOfCores();
  unsigned int numberOfCoresToUse= (numCoresAvailable > 1 ) ? 2 : 1 ;
  //=== TBB engine initialization
  // tbb::task_scheduler_init init( numberOfCoresToUse );
  // tbb::task_list tasks;
  
  // Simple example for using TBB scheduler:
  //  From http://www.ibm.com/developerworks/aix/library/au-intelthreadbuilding/index.html?ca=dat
  // task_scheduler_init init(task_scheduler_init::automatic);
  // first_task& f1 = *new(tbb::task::allocate_root()) first_task( );
  // tbb::task::spawn_root_and_wait(f1);
  
  // tbbMasterRunManager* runManager = new tbbMasterRunManager;
  
  SimEventModule* simEventMdl= new SimEventModule( numberOfCoresToUse );
  // Must be instantiated in the main program -- the tasks require it to do the work
  
  // simEventMdl= SimEventModule->GetInstance();

  tbbMasterRunManager*   runManager = simEventMdl->GetMasterRunManager();

  //Set TBB specific data to run-manager, 1 event per tbb::task (e.g. Nevents == N tasks)
  //Note that a /run/beamOn command will just create tasks and add them to tasks
  runManager->SetNumberEventsPerTask(1); //Not needed since 1 is default
  // runManager->SetTaskList(&tasks);
  
  //Set user-initialization that specify threading model, in this case TBB.
  //This overwrote default that used pthreads

  runManager->SetUserInitialization(new tbbUserWorkerInitialization );
    
  //==== Geant4 specific stuff, from now up to END-G4 comment is copy from MT example
  // Set mandatory initialization classes

  runManager->SetUserInitialization(new B2bDetectorConstruction());

  G4VModularPhysicsList* physicsList = new FTFP_BERT;
  physicsList->RegisterPhysics(new G4StepLimiterPhysics());
  runManager->SetUserInitialization(physicsList);
    
  // Set user action classes
  runManager->SetUserInitialization(new B2ActionInitialization());

  simEventMdl->Initialize();
  // Will Initialize the G4 kernel ie call runManager->Initialize();
  
  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if (argc!=1)   // batch mode
    {
      G4String command = "/control/execute ";
      G4String fileName = argv[1];
      UImanager->ApplyCommand(command+fileName);
    }
  else
    {  // interactive mode : define UI session
#if 1
      G4int nEvents= 50; 
      runManager->BeamOn(nEvents); 
#else     
   #ifdef G4UI_USE      
      G4UIExecutive* ui = new G4UIExecutive(argc, argv);
      UImanager->ApplyCommand("/control/execute init.mac");
      if (ui->IsGUI())
         UImanager->ApplyCommand("/control/execute gui.mac");
      ui->SessionStart();
      delete ui;
   #endif
#endif
    }
 //END-G4

#if 1
  runManager->CreateTasks();
  runManager->CreateAndStartWorkers();

#else
    G4Thread aThread;
    G4THREADCREATE(&aThread,startWork,static_cast<G4ThreadFunArgType>(&tasks));
    
    //Wait for work to be finised
    G4THREADJOIN(aThread);
#endif
    // delete runManager;
    delete simEventMdl;  // This should delete the runManager

    return 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
