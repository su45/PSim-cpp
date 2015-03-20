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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include "primsAlgorithm.h"

//Topology lambdas to test connectedness. Signiture: (int, int, int) => bool

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

//Commutative binop lambdas for reduce/accumulate with signiture: (int, int) => int

static std::function<int(int, int)> sum = std::plus<int>();
static std::function<int(int, int)> mul = std::multiplies<int>();
static std::function<int(int, int)> max = [](int a, int b) { return ((a > b) ? a : b); };
static std::function<int(int, int)> min = [](int a, int b) { return ((a > b) ? b : a); };

//Binop for comparing Edge objects against each other by weight: (Edge, Edge) => Edge
static std::function<Edge(Edge, Edge)> edgemin = [](Edge a, Edge b) {return ((a.weight > b.weight) ? b : a); };

/* 
 *  A struct containing a pair of file descriptors. fd[0]: read , fd[1]: write
 *  Each cell in the p x p matrix of pipes will contain a pipeFD struct
 */
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
        
        //dynamically allocate pipe_arr (to serve as a p x p matrix of pipes)
        pipe_arr = new pipeFD*[p];
        for(int i = 0; i < p; i++) {
            pipe_arr[i] = new pipeFD[p];
        }
        
        //create a pair of file descriptors via pipe() and pack a pipeFD struct into each cell
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
     * Send integer data to process j. (TODO: Templates/generics)
     */
    
    //Serialize plain int data and send to process j
    void _send(int j, int data) {
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw((this->pipe_arr[this->rank][j]).fd[1], boost::iostreams::never_close_handle);
        std::ostream os(&sbw);
        boost::archive::text_oarchive oa(os);
        oa << data;
    }
    
    //Serialize vector<int> and send to process j
    void _send_vector(int j, std::vector<int> data) {
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw((this->pipe_arr[this->rank][j]).fd[1], boost::iostreams::never_close_handle);
        std::ostream os(&sbw);
        boost::archive::text_oarchive oa(os);
        oa << data;
    }
    
    void send(int j, int data) {
        if(this->topology(this->rank, j, this->nprocs)) {
            _send(j, data);
        }
        else {
            //throw topology violation
        }
    }
    
    /*
     *  Receive integer data from process j. (TODO: Templates/generics)
     */
    
    //De-serialize plain int data from process j
    int _recv(int j) {
        int tmp;
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr((this->pipe_arr[j][this->rank]).fd[0], boost::iostreams::never_close_handle);
        std::istream is(&sbr);
        boost::archive::text_iarchive ia(is);
        ia >> tmp;
        return tmp;
    }
    
    //De-serialize vector<int> from process j
    std::vector<int> _recv_vector(int j) {
        std::vector<int> outvect;
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr((this->pipe_arr[j][this->rank]).fd[0], boost::iostreams::never_close_handle);
        std::istream is(&sbr);
        boost::archive::text_iarchive ia(is);
        ia >> outvect;
        return outvect;
    }
    
    int recv(int j) {
        if(this->topology(this->rank, j, this->nprocs)) {
            return _recv(j);
        }
        else {
            //throw topology violation
            return -1;
        }
    }
    
    /*
     *  GLOBAL COMMUNICATION PATTERNS:
     */
    
    /*
     *  Broadcast int 'value' to all processes from process 'source'
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
     *  Broadcast int 'value' to all processes
     */
    std::vector<int> all2all_broadcast(int value) {
        std::vector<int> vect, ret_vect;
        vect.resize(this->nprocs);
        ret_vect.resize(this->nprocs);
        vect = all2one_collect(0, value);
        if(this->rank == 0) {
            for(int i = 0; i < this->nprocs; i++) {
                _send_vector(i, vect);
            }
        }
        ret_vect = _recv_vector(0);
        return ret_vect;
    }
    
    
    /*
     *  Scatter slices the input vector<int> 'data' at process 'source' and sends a unique
     *  vector<int> to each process
     */
    std::vector<int> one2all_scatter(int source, std::vector<int> data) {
        if(this->rank == source) {
            int h = (int)data.size() / this->nprocs;
            int r = (int)data.size() % this->nprocs;
            if(r != 0) {
                h++;
            }
            for(int i = 0; i < this->nprocs-1; i++) {
                std::vector<int> tv( data.begin() + (i*h), data.begin() +(i*h) + (h) );
                _send_vector(i, tv);
            }
            //the last vector is often shorter than the previous ones, so we manually send the last
            //vector to ensure we aren't overshooting the iterator and sending 0s
            std::vector<int> tvl( data.begin() + ((this->nprocs-1)*h), data.end() );
            _send_vector((this->nprocs-1), tvl);
        }
        std::vector<int> outvect = _recv_vector(source);
        return outvect;
    }
    
    
    /*
     *  Collect gathers values (input int 'data') from each process and stores them in a
     *  vector<int> ordered by process at the process specified by 'destination'
     */
    std::vector<int> all2one_collect(int destination, int data) {
        _send(destination, data);
        std::vector<int> collection_by_rank;
        collection_by_rank.resize(this->nprocs);
        if(this->rank == destination) {
            for(int i = 0; i < this->nprocs; i++) {
                collection_by_rank[i] = _recv(i);
            }
        }
        else {
            collection_by_rank.clear();
        }
        return collection_by_rank;
    }
    
    
    /*
     *  All to one reduction returning the reduction of each process's 'value' using
     *  the functor 'binop'. The result is stored is process 'destination.'
     */
    int all2one_reduce(int destination, int value, std::function<int(int, int)>& binop) {
        this->_send(destination, value);
        std::vector<int> v;
        int result;
        if(this->rank == destination) {
            for(int i = 0; i < this->nprocs; i++) {
                int tmp = this->_recv(i);
                v.push_back(tmp);
            }
            result = std::accumulate(std::begin(v)+1, std::end(v), *std::begin(v), binop);
        }
        else {
            result = 0;
        }
        return result;
    }
    
    
    /*
     *  All to all reduction returning the reduction of each process's 'value' using
     *  the functor 'binop'. The result is broadcast to every process from process 0.
     */
    int all2all_reduce(int value, std::function<int(int, int)>& binop) {
        int reduction = all2one_reduce(0, value, binop);
        int all_reduction = one2all_broadcast(0, reduction);
        return all_reduction;
    }
    
    
    /*
     *  Barrier
     */
    void barrier() {
        all2all_broadcast(0);
    }
    
    
    /*
     * PUBLIC MEMBERS:
     */
    int nprocs;
    std::function<bool(int, int, int)> topology; //hold the lambda functor for this network's topology
    int rank;
    pipeFD **pipe_arr;
    
};
