//
//  primsAlgorithm.h
//  PSIM
//
//  Created by Sam Uddin on 3/19/15.
//  Copyright (c) 2015 Sam Uddin. All rights reserved.
//

#ifndef __PSIM__primsAlgorithm__
#define __PSIM__primsAlgorithm__

#include <stdio.h>
#include <unistd.h>
#include <unordered_set>
#include <set>
#include <vector>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
//#include "psim.cpp"


struct Edge {
    int e[2];
    int weight;
    
    //Default constructor
    Edge() {
        e[0] = 0;
        e[1] = 0;
        weight = 0;
    }
    
    //Overload constructor
    Edge(int u, int v, int w) {
        e[0] = u;
        e[1] = v;
        weight = w;
    }
    
    //Overloaded equality operator. Tests for cummutativity.
    bool operator==(const Edge& a) const {
        return (
                (e[0] == a.e[0] && e[1] == a.e[1]) ||
                (e[0] == a.e[1] && e[1] == a.e[0])
                );
    }
    
    friend std::ostream& operator<<(std::ostream & os, const Edge& ed) {
        os << ed.e[0] << " " << ed.e[1] << " (" << ed.weight << ") " << std::endl;
        return os;
    }
    
    void set(int u, int v, int w) {
        e[0] = u;
        e[1] = v;
        weight = w;
    }
    
    //Make Edge serializable via Boost
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & e;
        ar & weight;
    }
};

//Hash function for Edge objects (for use is the unordered set)
struct HashEdge {
    size_t operator()(const Edge &edge) const{
        return std::hash<int>()(edge.e[0]) + std::hash<int>()(edge.e[1]) ;
    }
};




enum PrimEnum{
    SEQUENTIAL,
    PARALLEL
};


class Prim {
public:
    
    Prim(const char* filename, PrimEnum);
    ~Prim();
    void run();
    
    PrimEnum type;
    int nVerts;
    int nEdges;
    int **adjMatrix;
    std::set<int> X;
    std::unordered_set<Edge, HashEdge> T;
    
private:
    void run_sequential();
    void run_parallel();
    
};

#endif /* defined(__PSIM__primsAlgorithm__) */
