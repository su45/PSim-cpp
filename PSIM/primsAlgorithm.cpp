//
//  primsAlgorithm.cpp
//  PSIM
//
//  Created by Sam Uddin on 3/19/15.
//  Copyright (c) 2015 Sam Uddin. All rights reserved.
//

#include "primsAlgorithm.h"




/*
 *  Constructor taking in the (absolute path) filename of an 
 *  undirected weighted graph and an enum for sequential or parallel.
 */
Prim::Prim(const char* filename, PrimEnum typeIn) {
    this->type = typeIn;
    
    //open and read file of weighted undirected graph
    std::ifstream infs(filename);
    std::cout << "Prim's Algorithm -- " << "Undirected Weighted Graph\n";
    std::cout << ((this->type == PrimEnum::SEQUENTIAL) ? "SEQUENTIAL\n" : "PARALLEL\n");
    std::cout << filename << std::endl;
    infs >> this->nVerts;
    infs >> this->nEdges;
    std::cout << "Vertices: " << this->nVerts << "\nEdges: " << this->nEdges << std::endl;
    
    //Dynamically allocate adjMatrix to serve as a nVerts x nVerts adjacency matrix for
    //the weighted undirected graph
    this->adjMatrix = new int*[this->nVerts];
    for(int i = 0; i < this->nVerts; i++) {
        adjMatrix[i] = new int[this->nVerts];
    }
    
    //Zero out the adjaceny matrix
    for(int i = 0; i < this->nVerts; i++) {
        for(int j = 0; j < this->nVerts; j++) {
            adjMatrix[i][j] = 0;
        }
    }
    
    //Load edges and weights from file
    int u, v, weight;
    for (int i = 0; i < this->nEdges; i++) {
        infs >> u;
        infs >> v;
        infs >> weight;
        adjMatrix[u][v] = weight;
        adjMatrix[v][u] = weight;
    }
    infs.close();
    
    //Print a representation of the adjacency matrix
    for(int i = 0; i < this->nVerts; i++) {
        for(int j = 0; j < this->nVerts; j++) {
            std::cout << adjMatrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}


/*
 *  Destructor -- clean up and deallocate adjacency matrix
 *
 */
Prim::~Prim() {
    for(int i = 0; i < this->nVerts; i++){
        delete this->adjMatrix[i];
    }
    delete [] this->adjMatrix;
}

/*
 *  Run Prim's Algorithm and output the Edges comprising the MST.
 *  Will call the version of algorithm specified on instantiation.
 *
 */
void Prim::run() {
    if(this->type == PrimEnum::SEQUENTIAL) {
        this->run_sequential();
    }
    else if(this->type == PrimEnum::PARALLEL) {
        this->run_parallel();
    }
}

void Prim::run_sequential() {
    std::set<int> X;
    //We need an unordered_set<Edge, HashEdge> here with a custom hash function and operator== :
    std::unordered_set<Edge, HashEdge> T;
    
    //start at an arbitrary node -- 0
    X.insert(0);
    
    while (X.size() != this->nVerts) {
        Edge edgy;
        int d = 0;
        
        //For each element x in set X, add Edge(x, k) to crossing if k is NOT in set X
        
        for(std::set<int>::iterator it = X.begin(); it != X.end(); it++) {
            int x = *it;
            for(int k = 0; k < this->nVerts; k++) {
                
                //if k is NOT in set X
                if(X.find(k) == X.end()) {
                    int link = this->adjMatrix[x][k];
                    if(link != 0) {
                        if(d == 0 || link < d) {
                            edgy.set(x, k, this->adjMatrix[x][k]);
                            d = link;
                        }
                    }
                }
            }
        }
        T.insert(edgy);        //Insert Edge edgy into the MST
        X.insert(edgy.e[1]);   //Add the new vertex to set X
    }
    
    //Print the Edges of the MST
    std::cout << "-------------------\n";
    std::cout << "MST edges (weight): \n";
    for(std::unordered_set<Edge, HashEdge>::iterator treeIt = T.begin(); treeIt != T.end(); treeIt++) {
        Edge ed = *treeIt;
        std::cout << ed;
    }
}

void Prim::run_parallel() {
    
}



