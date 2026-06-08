#ifndef DEBUG_H
#define DEBUG_H

#ifdef __DEBUG_GAME__

    #include <stdio.h>
    #define DEBUG_LOG(...) printf(__VA_ARGS__);

  #else

    #define DEBUG_LOG(x) { ; }

  #endif // !__DEBUG_GAME

#endif // !DEBUG_H
