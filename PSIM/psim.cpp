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
#include <math.h>

//topology lambdas to test connectedness. signiture: (int, int, int) => bool
//parameter p is unused for most topology lambdas

static std::function<bool(int, int, int)> BUS = [](int i, int j, int p) { return true; };
static std::function<bool(int, int, int)> SWITCH = [](int i, int j, int p) { return true; };

static std::function<bool(int, int, int)> MESH1 = [](int i, int j, int p)
                                                    {
                                                        return ((i-j)^2) == 1;
                                                    };

static std::function<bool(int, int, int)> TORUS1 = [](int i, int j, int p)
                                                    {
                                                        return ((i-j+p)%p) == 1 || ((j-i+p)%p) == 1;
                                                    };

static std::function<bool(int, int, int)> MESH2 = [](int i, int j, int p)
                                                    {
                                                        int q = static_cast<int>(sqrtf(p) + 0.1f);
                                                        int a = (i%q - j%q)^2;
                                                        int b = (i/q - j/q)^2;
                                                        return (a == 1 && b == 0) || (a == 0 && b == 1);
                                                    };

static std::function<bool(int, int, int)> TORUS2 = [](int i, int j, int p)
                                                    {
                                                        int q = static_cast<int>(sqrtf(p) + 0.1f);
                                                        int a = (i%q - j%q + q) % q;
                                                        int b = (i/q - j/q + q) % q;
                                                        int c = (j%q - i%q + q) % q;
                                                        int d = (j/q - i/q + q) % q;
                                                        return ((a == 0 && b == 1) || (a == 1 && b == 0)) ||
                                                               ((c == 0 && d == 1) || (c == 1 && d == 0));
                                                    };

static std::function<bool(int, int, int)> TREE = [](int i, int j, int p)
                                                    {
                                                        return i == static_cast<int>((j-1)/2) ||
                                                               j == static_cast<int>((i-1)/2);
                                                    };

//binop lambdas for reduce/accumulate with signiture: (int, int) => int

static std::function<int(int, int)> sum = std::plus<int>();
static std::function<int(int, int)> mul = std::multiplies<int>();
static std::function<int(int, int)> max = [](int a, int b) { return ((a > b) ? a : b); };
static std::function<int(int, int)> min = [](int a, int b) { return ((a > b) ? b : a); };


//a struct containing a pair of file descriptors. fd[0]: read , fd[1]: write
struct pipeFD {
    int fd[2];
};

class PSim {
public:
    
    //constructor taking # of processors and a functor reference to a topology lambda 
    PSim(int p, std::function<bool(int, int, int)>& topo) {
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
    
    /*
     * METHODS:
     */
    
    /*
     * send integer data to process j
     */
    void _send(int j, int data) {
        char sendbuf[10];
        sprintf(sendbuf, "%d", data);
        write((this->pipe_arr[this->rank][j]).fd[1], sendbuf, 10);
    }
    
    void send(int j, int data) {
        if(this->topology(this->rank, j, this->nprocs)) {
            _send(j, data);
        }
        else {
            //log topology violation
        }
    }
    
    /*
     * receive integer data from process j
     */
    int _recv(int j) {
        char recvbuf[10];
        read((this->pipe_arr[j][this->rank]).fd[0], recvbuf, 10);
        return atoi(recvbuf);
    }
    
    int recv(int j) {
        if(this->topology(this->rank, j, this->nprocs)) {
            return _recv(j);
        }
        else {
            //log topology violation
            return -1;
        }
    }
    

    /*
     * PUBLIC MEMBERS:
     */
    int nprocs;
    std::function<bool(int, int, int)> topology; //hold the lambda functor for this network's topology
    int rank;
    pipeFD pipe_arr[3][3]; //should be **pipe_arr

    
};
