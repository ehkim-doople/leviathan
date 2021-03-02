# leviathan
C++ Big Data Solution


common : a collection of library of data structures with improved speed
         -  Very fast memory POOL, 
         -  Synchronization-based data struture collection : spinLock & atomic & Critical Section(mutex) & SWLock 
         -  Large file handling library
         -  quick sort
         -  hash map, double hash map
         -  Date time processing library
         -  multi OS support Library (70% application rate)
         -  a collection of OS-dependent system libraries


network : Framework for building servers very quickly
        verification competed : iocp/redis
        todo : 
          - Provides a more organized interface
          - provide epoll-based framework
          - provide buffer according to http contents size analysis

LogAnalyzer : Large File Analyzer To verify server and engine performance, accuracy, and integrity
        todo : 
          - Provides more concise and convenient user setting function
          - Add more various analysis functions
