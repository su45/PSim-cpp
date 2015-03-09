/*
 *  psim.cpp
 *  PSIM
 *  Created by Sam Uddin
 *
 *  Ported from Massimo DiPierro's psim.py
 */

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <unistd.h>
#include <math.h>
#include <vector>
#include <iterator>
#include <numeric>

//topology lambdas to test connectedness. signiture: (int, int, int) => bool

static std::function<bool(int, int, int)> BUS = [](int i, int j, int p) { return true; };
static std::function<bool(int, int, int)> SWITCH = [](int i, int j, int p) { return true; };

static std::function<bool(int, int, int)> MESH1 = [](int i, int j, int p)
                                                    {
                                                        return pow((i-j), 2) == 1;
                                                    };

static std::function<bool(int, int, int)> TORUS1 = [](int i, int j, int p)
                                                    {
                                                        return ((i-j+p)%p) == 1 || ((j-i+p)%p) == 1;
                                                    };

static std::function<bool(int, int, int)> MESH2 = [](int i, int j, int p)
                                                    {
                                                        int q = static_cast<int>(sqrt(p) + 0.1f);
                                                        int a = pow((i%q - j%q), 2);
                                                        int b = pow((i/q - j/q), 2);
                                                        return (a == 1 && b == 0) || (a == 0 && b == 1);
                                                    };

static std::function<bool(int, int, int)> TORUS2 = [](int i, int j, int p)
                                                    {
                                                        int q = static_cast<int>(sqrt(p) + 0.1f);
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

//commutative binop lambdas for reduce/accumulate with signiture: (int, int) => int

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
    
    /* 
     * CONSTRUCTOR
     * params:  int p: # of processors
     *          std::function<bool(int, int, int)>& topo: a functor reference to a topology lambda
     */
    PSim(int p, std::function<bool(int, int, int)>& topo) {
        nprocs = p;
        topology = topo;
        
        //dynamically allocate pipe_arr (p x p matrix of pipes)
        pipe_arr = new pipeFD*[p];
        for(int i = 0; i < p; i++) {
            pipe_arr[i] = new pipeFD[p];
        }
        
        //create a pair of file descriptors and pack a pipeFD struct into each cell
        for(int i = 0; i < p; i++) {
            for(int j = 0; j < p; j++) {
                pipeFD temp;
                pipe(temp.fd);
                pipe_arr[i][j] = temp;
            }
        }
        this->rank = 0;
        //printf("STARTING: process rank %d @ pid %d.\n", this->rank, getpid());
        //fork n-1 processes
        pid_t pid;
        for(int i = 1; i < p; i++) {
            if((pid = fork()) == 0) {
                this->rank = i;
                //printf("STARTING: process rank %d @ pid %d.\n", this->rank, getpid());
                break;
            }
        }
    }
    
    //DESTRUCTOR
    ~PSim() {
        for(int i = 0; i < this->nprocs; i++) {
            delete [] pipe_arr[i];
        }
        delete [] pipe_arr;
    }
    
    /*
     * CLASS METHODS:
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
     * Broadcast
     */
    int one2all_broadcast(int source, int value) {
        if(this->rank == source) {
            for(int i = 0; i < this->nprocs; i++) {
                if(!(i == source)) {
                    this->_send(i, value);
                }
            }
        }
        else {
            value = this->_recv(source);
        }
        return value;
    }
    
    /*
     *  Scatter and Collect
     */
    std::vector<int> one2all_scatter(int source, std::vector<int> data) {
        if(this->rank == source) {
            int h = (int)data.size() / this->nprocs;
            int r = (int)data.size() % this->nprocs;
            
            if(r != 0) {
                h++;
            }
            
            
            
        }
        
        return std::vector<int>(0);
    }
    
    
    /*
     *  Reduce
     */
    int all2one_reduce(int destination, int value, std::function<int(int, int)>& binop) {
        this->_send(destination, value);
        std::vector<int> v;
        int result;
        if(this->rank == destination) {
            for(int i = 0; i < this->nprocs; i++) {
                v.push_back(this->_recv(i));
            }
            //pause to ensure that values from all other processes are received at destination. Kind of hacky.
            sleep(2);
            result = std::accumulate(std::begin(v)+1, std::end(v), *std::begin(v), binop);
        }
        else {
            result = 0;
        }
        return result;
    }
    
    

    /*
     * PUBLIC MEMBERS:
     */
    int nprocs;
    std::function<bool(int, int, int)> topology; //hold the lambda functor for this network's topology
    int rank;
    pipeFD **pipe_arr;
    
};
