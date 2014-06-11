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

//   Simple class to dispatch 'slots' - a number between 0 and 'MaxSlots'
//    which is determined from previous requests
//

#ifndef SLOT_Dispatcher_hh
#define SLOT_Dispatcher_hh 1

#include <tbb/concurrent_queue.h>
#include "tls.hh"  // Defines G4ThreadLocal

class SlotDispatcher
{
public: 
  static SlotDispatcher* GetInstance(); 
  int          ObtainSlot();  // Potential return -1 for failure 
  void         RelinquishSlot(unsigned int mySlotId);

  unsigned int GetTotalNumber();
  unsigned int GetNumberOfFreeSlots();

  // The following operations are not thread-safe
  void         AddSlots(unsigned int numAdditionalSlots);
  bool         ExpandSlots(unsigned int newNumberOfSlots);
                    // Grow the size to newNumberOfSlots

  static void  UnitTest();
  
private:
  SlotDispatcher() {};
  ~SlotDispatcher() {};
private:
  static tbb::concurrent_bounded_queue<unsigned int> m_theAvailableSlots;
  
  static G4ThreadLocal  int m_currentSlotId;
};
#endif
