/*
 *  main.cpp
 *  PSIM
 *
 *  Created by Sam Uddin on 3/2/15.
 *  Copyright (c) 2015 Sam Uddin. All rights reserved.
 */

#include <iostream>
#include <unistd.h>
#include <string>
#include <functional>
#include "psim.cpp"

// Driver program. Implementation of Prim's algorithm using the C++ PSim API:

int main(int argc, const char * argv[]) {
    
    PSim comm(3, SWITCH);
    sleep(1);

    //printf("mul: %d\n", mul(mul(4, 6), 3));
    
    if(comm.rank == 0) {
        //printf("*** Hello world from rank %d @ pid %d. ***\n", comm.rank, getpid());
        comm.send(1, 123456789);
    }
    else if(comm.rank == 1) {
        int msg = comm.recv(0);
        printf("@process %d (pid %d) => message received from process 0: %d\n", comm.rank, getpid(), msg);
    }
    
    return 0;
}
