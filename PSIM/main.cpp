/*
 *  main.cpp
 *  PSIM
 *  Created by Sam Uddin on 3/2/15.
 *
 *  Driver program. Various tests of the C++ PSim API
 */

#include <iostream>
#include <unistd.h>
#include <string>
#include <functional>
#include "psim.cpp"

//TEST MACROs
#define TOPOLOGY 1
#define BCAST 0
#define PRIM_SEQUENTIAL 0
#define PRIM_PARALLEL 0

static void topology_test() {
    static std::function<bool(int, int, int)> f;
    int i, j, p;
    
    f = SWITCH;
    i = 0; j = 235; p = 23487;
    printf("Topology: SWITCH, i = %d, j = %d, p = %d.\nAre i and j connected => %d\n\n", i, j, p, f(i, j, p));
    
    f = MESH2;
    i = 14; j = 9; p = 16;
    printf("Topology: MESH2, i = %d, j = %d, p = %d.\nAre i and j connected => %d\n\n", i, j, p, f(i, j, p));
    
    i = 14; j = 10; p = 16;
    printf("Topology: MESH2, i = %d, j = %d, p = %d.\nAre i and j connected => %d\n\n", i, j, p, f(i, j, p));
    
    f = TORUS1;
    i = 0; j = 4; p = 5;
    printf("Topology: TORUS1, i = %d, j = %d, p = %d.\nAre i and j connected => %d\n\n", i, j, p, f(i, j, p));
}

static void bcast_test() {
    int p = 8;
    PSim comm(p, SWITCH);
    sleep(1);
    int msg = (comm.rank == 0) ? 112358 : 0;
    printf("@process %d (pid %d) => message PRE-BROADCAST is: %d\n", comm.rank, getpid(), msg);
    sleep(1);
    msg = comm.one2all_broadcast(0, msg);
    sleep(1);
    printf("@process %d (pid %d) => message POST-BROADCAST is: %d\n", comm.rank, getpid(), msg);
}



/*
 *  MAIN()
 */

int main(int argc, const char * argv[]) {
    
#if(TOPOLOGY)
    topology_test();
#endif
    
#if(BCAST)
    bcast_test();
#endif
    
#if(PRIM_SEQUENTIAL)
    
#endif
    
#if(PRIM_PARALLEL)
    
#endif
    
    
    return 0;
}


