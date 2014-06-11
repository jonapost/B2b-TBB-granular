#ifndef EVENT_HH
#define EVENT_HH 1

// Stand-in for experiment class that carries
//   - event id information
//   - id of 'worker slot' - ie the thread to be used to simulate it
//   - seed to be used in event

class Event{
  public:
    Event(int aId, int cId) { m_id= aId; m_concurrent_index= cId; }
    int id()             { return m_id; }
    int concurrentIndex(){ return m_concurrent_index; }
     //  long seed() { return m_seed; }
    void GetSeeds(long *seed1, int *seed2) { *seed1= m_seed1; *seed2= m_seed2; }
  private:
    int  m_id;
    int  m_concurrent_index;
    long m_seed1;
    int   m_seed2;
};
#endif // EVENT_HH
