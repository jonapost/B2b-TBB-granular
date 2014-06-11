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
//
// Class description:
//
//    This class implements the worker model run manager for TBB based
//    application.
//    It is instantiated by tbbUserWorkerInitialization and used by
//    tbbMasterRunManager.
//    See G4WorkerRunManager for documentation of methods relative to
//    base class. Only class specific methods are documented here.
//
// Equivalent in traditional MT:
//    G4WorkerRunManager
//
// History:
//    Oct 31st, 2013 A. Dotti - First Implementation

#include "tbbMasterRunManager.hh"
#include "tbbTask.hh"

#include <tbb/task_scheduler_init.h>
#include <tbb/task.h>

using tbb::task;

tbbMasterRunManager::tbbMasterRunManager() :
    G4MTRunManager(),
   // fTaskList(static_cast<tbb::task_list*>(0)),
    fNumEvtsPerTask(1)
{
  // tbb::task_scheduler_init init( numberOfCoresToUse );

  // fpTasksList = new tbb::task_list();
}

tbbMasterRunManager::~tbbMasterRunManager()
{
}

void tbbMasterRunManager::TerminateWorkers()
{
    //For TBB based example this should be empty
}


void tbbMasterRunManager::CreateTasks(int numEvents)
{
  //Instead of pthread based workers, create tbbTask
  G4int ntasks = numEvents/fNumEvtsPerTask;
  G4int remainEv = 0;
  if( fNumEvtsPerTask > 1 ) {
    remainEv = numEvents % fNumEvtsPerTask;
  }
  
  G4cout << "tbbMasterRunManager::CreateTasks: " << G4endl
         << "  Creating " << ntasks << " tasks, "
         << " with " << fNumEvtsPerTask << " events per task. "
         << G4endl;
  G4int nt;
  for (  nt = 0 ; nt < ntasks ; ++nt )
  {
     G4int evts= fNumEvtsPerTask;
     CreateOneTask(nt,evts);
  }
  if( remainEv != 0 ) {
     G4cout << "  Creating 1 (extra) task for remaining "
            << remainEv << " events. " << G4endl;
     CreateOneTask(++nt, remainEv );
  }
  // if ( nt == ntasks - 1 ) evts+=remainEv;

}

void tbbMasterRunManager::CreateOneTask(G4int id,G4int evts)
{
  tbbTask& task = * new(tbb::task::allocate_root())
                      tbbTask( id , NULL , evts ); //Add output for merging
  G4cout << " Created task with id=" << id
         << " with " << evts << " events. " << G4endl;
  fTaskList.push_back( task );
}

// void tbbMasterRunManager::StartWork() // int numTasks)

// CreateAndStartWorkers
void tbbMasterRunManager::CreateAndStartWorkers()
{
  // ??
  // tbb::task::spawn_root_and_wait(); // ??
  // tbb::task::set_ref_count(3); // 2 (1 per child task) + 1 (for the wait)
  // tbb::task::spawn_and_wait_for_all(fTaskList);

  tbb::task::spawn_root_and_wait(fTaskList);
  // Creates the workers and waits for it to end ...
}

void tbbMasterRunManager::RunTermination()
{
    // Reduce results ....
    G4MTRunManager::RunTermination();
}

