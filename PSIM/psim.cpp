//
//  psim.cpp
//  PSIM
//
//  Created by Sam Uddin on 3/2/15.
//  Copyright (c) 2015 Sam Uddin. All rights reserved.
//

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <unistd.h>

enum topology_enum {
    BUS,
    SWITCH,
    MESH1,
    TORUS1,
    MESH2,
    TORUS2,
    TREE
};

//a struct containing a pair of file descriptors
struct pipeFD {
    int fd[2];
};

class PSim {
public:
    
    //constructor
    PSim(int p, topology_enum topo) {
        nprocs = p;
        topology = topo;
        
        //dynamically allocate pipe_arr HERE (p x p matrix)
        
        //create a pair of file descriptors and pack a pipeFD struct into each cell (replace '3' with 'p')
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                pipeFD temp;
                pipe(temp.fd);
                pipe_arr[i][j] = temp;
            }
        }
        this->rank = 0;
        printf("STARTING: process rank %d @ pid %d.\n", this->rank, getpid());
        //fork n-1 processes
        pid_t pid;
        for(int i = 1; i < p; i++) {
            if((pid = fork()) == 0) {
                this->rank = i;
                printf("STARTING: process rank %d @ pid %d.\n", this->rank, getpid());
                break;
            }
        }
    }
    
    //destructor
    ~PSim() {
        //free the pipe_arr
        //printf("ENDING: process rank %d @ pid %d.\n", this->rank, getpid());
    }
    
    //class methods:
    
    //send integer data to process j
    void send(int j, int data) {
        char sendbuf[10];
        sprintf(sendbuf, "%d", data);
        write((this->pipe_arr[this->rank][j]).fd[1], sendbuf, 10);
    }
    
    //receive integer data from process j
    int recv(int j) {
        char recvbuf[10];
        read((this->pipe_arr[j][this->rank]).fd[0], recvbuf, 10);
        return atoi(recvbuf);
    }
    
    

    //public members:
    
    int nprocs;
    topology_enum topology;
    int rank;
    char buffer[100];
    //TEST WITH A 3x3 MATRIX FIRST TO MAKE SURE PIPING WORKS (Dynamically create later):
    pipeFD pipe_arr[3][3];
    //pipeFD** pipe_array;
    
};

//binops/lambdas for reduce/accumulate with signiture: (int, int) => int 
static std::function<int(int, int)> sum = std::plus<int>();
static std::function<int(int, int)> mul = std::multiplies<int>();
static std::function<int(int, int)> max = [](int a, int b) { return ((a > b) ? a : b); };
static std::function<int(int, int)> min = [](int a, int b) { return ((a > b) ? b : a); };



