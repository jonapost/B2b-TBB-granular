#include "G4RunManagerKernel.hh"
#include "ExperimentRunManager.hh"

ExperimentRunManager::ExperimentRunManager( G4RunManagerKernel* rmk)   
: m_RunManagerKernel( rmk )
{
   if( !m_RunManagerKernel ) {
      m_RunManagerKernel = new G4RunManagerKernel();
   }
}