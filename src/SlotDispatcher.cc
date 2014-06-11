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
#include "SlotDispatcher.hh"

#include <tbb/atomic.h>
namespace {
   tbb::atomic<unsigned int> a_SlotCounter;
}

#include <assert.h>
// #include <stdio.h>
#include <iostream.h>
#include "tls.hh"

G4ThreadLocal  int SlotDispatcher::m_currentSlotId= -1;
tbb::concurrent_bounded_queue<unsigned int> SlotDispatcher::m_theAvailableSlots;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SlotDispatcher* SlotDispatcher::GetInstance()
{
  static SlotDispatcher *SD=0;
  if( SD == 0) {
    a_SlotCounter= 0u;
    SD= new SlotDispatcher();
    
    SlotDispatcher::UnitTest();
  }
  return SD;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

bool SlotDispatcher::ExpandSlots(unsigned int newNumSlots)
{
  if ( newNumSlots <= a_SlotCounter ) { return false; }
  
  for( unsigned int i=a_SlotCounter; i<newNumSlots; ++i)
  {
    m_theAvailableSlots.push(a_SlotCounter);
    a_SlotCounter++;
  }
  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SlotDispatcher::AddSlots(unsigned int numAdditionalSlots)
{
   ExpandSlots( a_SlotCounter + numAdditionalSlots );
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int SlotDispatcher::ObtainSlot()
{
  unsigned int slotId;
  m_theAvailableSlots.pop(slotId);  // Wait for free slot (if needed)
  m_currentSlotId= slotId;

  std::cout << "Dispatcher>  Obtained slot " << slotId << std::endl;
  return slotId;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SlotDispatcher::RelinquishSlot(unsigned int mySlotId)
{
  if( (int) mySlotId != m_currentSlotId ){
     std::cout << " ERROR in SlotDispatcher::RelinquishSlot. "
      << " Missmatch in request to return " << mySlotId
      << " but current thread holds slot " << m_currentSlotId << std::endl;
  }
  std::cout << "Dispatcher> Relinquished slot " << mySlotId << std::endl;
  m_theAvailableSlots.push(m_currentSlotId);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

unsigned int SlotDispatcher::GetTotalNumber() { return a_SlotCounter; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

unsigned int SlotDispatcher::GetNumberOfFreeSlots()
{ return m_theAvailableSlots.size(); }; 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Check implementation
void SlotDispatcher::UnitTest()
{
  SlotDispatcher *sd= SlotDispatcher::GetInstance();

  unsigned int numSlots= 1;

  for( numSlots = 1; numSlots < 15; numSlots++)
  {
    sd->ExpandSlots( numSlots); 
    assert( sd->GetTotalNumber() == numSlots );

    int retSlot= sd->ObtainSlot();

    if( retSlot < 0 )
       std::cout << " ERROR in test : Obtained negative slot. " << std::endl;
    else{
      unsigned int mySlot = (unsigned int) retSlot;
      if( mySlot > numSlots-1 ){
         std::cout << " ERROR in test : Obtained large slot number = "
                << mySlot << " compared with number of Slots = " << numSlots
                << std::endl;
      }else{
         std::cout<< " OK - obtained slot " << mySlot << std::endl;
         sd->RelinquishSlot(mySlot);
      }
    }
  }
}