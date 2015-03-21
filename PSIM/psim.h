//
//  psim.h
//  PSIM
//
//  Created by Sam Uddin
//

#ifndef PSIM_psim_h
#define PSIM_psim_h

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

//------------------------------------------------------------------------------------------------

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
static std::function<Edge(Edge, Edge)> edgemax = [](Edge a, Edge b) {return ((a.weight > b.weight) ? a : b); };

//------------------------------------------------------------------------------------------------

/*
 *  A struct containing a pair of file descriptors. fd[0]: read , fd[1]: write
 *  Each cell in the p x p matrix of pipes will contain a pipeFD struct
 */
struct pipeFD {
    int fd[2];
};

class PSim {
public:
    
    PSim(int p, std::function<bool(int, int, int)>& topo);
    ~PSim();
    
    void _send(int j, int data);
    void _send_vector(int j, std::vector<int> data);
    void _send_Edge(int j, Edge data);
    void send(int j, int data);
    int _recv(int j);
    std::vector<int> _recv_vector(int j);
    Edge _recv_Edge(int j);
    int recv(int j);
    
    int one2all_broadcast(int source, int value);
    Edge one2all_broadcast_E(int source, Edge value);
    std::vector<int> all2all_broadcast(int value);
    std::vector<int> one2all_scatter(int source, std::vector<int> data);
    std::vector<int> all2one_collect(int destination, int data);
    int all2one_reduce(int destination, int value, std::function<int(int, int)>& binop);
    Edge all2one_reduce_E(int destination, Edge value, std::function<Edge(Edge, Edge)>& binop);
    int all2all_reduce(int value, std::function<int(int, int)>& binop);
    Edge all2all_reduce_E(Edge value, std::function<Edge(Edge, Edge)>& binop);
    void barrier();
    
    
    
    
    //PUBLIC MEMBERS:
    int nprocs;
    std::function<bool(int, int, int)> topology; //hold the lambda functor for this network's topology
    int rank;
    pipeFD **pipe_arr;
};



#endif
