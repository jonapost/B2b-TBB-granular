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
#include <tbb/task.h>

using tbb::task;

tbbMasterRunManager::tbbMasterRunManager() :
    G4MTRunManager(),
   // fTaskList(static_cast<tbb::task_list*>(0)),
    nEvtsPerTask(1)
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


void tbbMasterRunManager::CreateTasks()
{
  //Instead of pthread based workers, create tbbTask
  G4int ntasks = numberOfEventToBeProcessed/nEvtsPerTask;
  G4int remn = numberOfEventToBeProcessed % nEvtsPerTask;
  for ( G4int nt = 0 ; nt < ntasks ; ++nt )
  {
      G4int evts= nEvtsPerTask;
      if ( nt == ntasks - 1 ) evts+=remn;
      CreateOneTask(nt,evts);
  }
  // tbb::
  set_ref_count(ntasks+1); // (1 per child task) + 1 (for the wait)
  spawn_and_wait_for_all(fTaskList);
}

void tbbMasterRunManager::CreateOneTask(G4int id,G4int evts)
{
    tbbTask& task = * new(tbb::task::allocate_root())
                      tbbTask( id , NULL , evts ); //Add output for merging
    fTaskList.push_back( task );
}

// CreateAndStartWorkers
void tbbMasterRunManager::StartWork() // int numTasks)
{
  // ??
  // tbb::task::spawn_root_and_wait(); // ??
  tbb::set_ref_count(3); // 2 (1 per child task) + 1 (for the wait)
  tbb::spawn_and_wait_for_all(list1);
  // Creates the workers and waits for it to end ... 
}

void tbbMasterRunManager::RunTermination()
{
    // Reduce results ....
    G4MTRunManager::RunTermination();
}

