#ifndef EXPERIMENTRUNMANAGER_HH
#define EXPERIMENTRUNMANAGER_HH 1

class ExperimentRunManager{
   public:
     ExperimentRunManager( G4RunManagerKernel* ); 

   private:
      G4RunManagerKernel*  m_RunManagerKernel;
};
#endif // EXPERIMENTRUNMANAGER_HH
