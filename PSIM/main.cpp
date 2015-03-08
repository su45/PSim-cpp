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
#define TOPOLOGY 0
#define BCAST 0
#define REDUCE 1
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
    PSim comm(8, SWITCH);
    int msg = (comm.rank == 0) ? 112358 : 0;
    printf("@process %d (pid %d) => message PRE-BROADCAST is: %d\n", comm.rank, getpid(), msg);
    sleep(2);
    msg = comm.one2all_broadcast(0, msg);
    printf("@process %d (pid %d) => message POST-BROADCAST is: %d\n", comm.rank, getpid(), msg);
}

static void reduce_test() {
    PSim comm(5, SWITCH);
    int red_sum = comm.all2one_reduce(0, comm.rank, sum);
    printf("@process %d (pid %d) => reduction result of sum(rank 0 ... rank 5) is: %d\n", comm.rank, getpid(), red_sum);
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
    
#if(REDUCE)
    reduce_test();
#endif
    
#if(PRIM_SEQUENTIAL)
    
#endif
    
#if(PRIM_PARALLEL)
    
#endif
    
    
    return 0;
}


