/*
 *  psim.cpp
 *  PSIM
 *  Created by Sam Uddin
 *
 *  Ported from Massimo DiPierro's psim.py
 */

#include "psim.h"

/*
 * CONSTRUCTOR
 * params:  int p: # of processors
 *          std::function<bool(int, int, int)>& topo: a functor reference to a topology lambda
 */
PSim::PSim(int p, std::function<bool(int, int, int)>& topo) {
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
PSim::~PSim() {
    for(int i = 0; i < this->nprocs; i++) {
        delete [] pipe_arr[i];
    }
    delete [] pipe_arr;
}

//------------------------------------------------------------------------------------------------
/*
 * CLASS METHODS:
 */

/*
 * Send integer data to process j. (TODO: Templates/generics)
 */

//Serialize plain int data and send to process j
void PSim::_send(int j, int data) {
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw((this->pipe_arr[this->rank][j]).fd[1], boost::iostreams::never_close_handle);
    std::ostream os(&sbw);
    boost::archive::text_oarchive oa(os);
    oa << data;
}

//Serialize vector<int> and send to process j
void PSim::_send_vector(int j, std::vector<int> data) {
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw((this->pipe_arr[this->rank][j]).fd[1], boost::iostreams::never_close_handle);
    std::ostream os(&sbw);
    boost::archive::text_oarchive oa(os);
    oa << data;
}

//Serialize Edge and send to process j
void PSim::_send_Edge(int j, Edge data) {
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw((this->pipe_arr[this->rank][j]).fd[1], boost::iostreams::never_close_handle);
    std::ostream os(&sbw);
    boost::archive::text_oarchive oa(os);
    oa << data;
}

void PSim::send(int j, int data) {
    if(this->topology(this->rank, j, this->nprocs)) {
        _send(j, data);
    }
    else {
        //throw topology violation
    }
}
//------------------------------------------------------------------------------------------------

/*
 *  Receive integer data from process j. (TODO: Templates/generics)
 *
 */

//De-serialize plain int data from process j
int PSim::_recv(int j) {
    int tmp;
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr((this->pipe_arr[j][this->rank]).fd[0], boost::iostreams::never_close_handle);
    std::istream is(&sbr);
    boost::archive::text_iarchive ia(is);
    ia >> tmp;
    return tmp;
}

//De-serialize vector<int> from process j
std::vector<int> PSim::_recv_vector(int j) {
    std::vector<int> outvect;
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr((this->pipe_arr[j][this->rank]).fd[0], boost::iostreams::never_close_handle);
    std::istream is(&sbr);
    boost::archive::text_iarchive ia(is);
    ia >> outvect;
    return outvect;
}

//De-serialize Edge from process j
Edge PSim::_recv_Edge(int j) {
    Edge outEdge;
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr((this->pipe_arr[j][this->rank]).fd[0], boost::iostreams::never_close_handle);
    std::istream is(&sbr);
    boost::archive::text_iarchive ia(is);
    ia >> outEdge;
    return outEdge;
}

int PSim::recv(int j) {
    if(this->topology(this->rank, j, this->nprocs)) {
        return _recv(j);
    }
    else {
        //throw topology violation
        return -1;
    }
}
//------------------------------------------------------------------------------------------------

/*
 *  GLOBAL COMMUNICATION PATTERNS:
 */

/*
 *  Broadcast int 'value' to all processes from process 'source'
 */
int PSim::one2all_broadcast(int source, int value) {
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
 *  Broadcast EDGE 'value' to all processes from process 'source'
 *  TODO: omit and implement generics/Templates
 */
Edge PSim::one2all_broadcast_E(int source, Edge value) {
    if(this->rank == source) {
        for(int i = 0; i < this->nprocs; i++) {
            if(!(i == source)) {
                this->_send_Edge(i, value);
            }
        }
    }
    else {
        value = this->_recv_Edge(source);
    }
    return value;
}

/*
 *  Broadcast int 'value' to all processes and returns a vector of each process's value
 */
std::vector<int> PSim::all2all_broadcast(int value) {
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
std::vector<int> PSim::one2all_scatter(int source, std::vector<int> data) {
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
std::vector<int> PSim::all2one_collect(int destination, int data) {
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
int PSim::all2one_reduce(int destination, int value, std::function<int(int, int)>& binop) {
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
 *  Reduction of each process's EDGE WEIGHT using (Edge, Edge) => Edge
 *  the functor 'binop'. The result is stored is process 'destination.'
 */
Edge PSim::all2one_reduce_E(int destination, Edge value, std::function<Edge(Edge, Edge)>& binop) {
    //Send if this process isn't the dest
    if (this->rank != destination) {
        this->_send_Edge(destination, value);
    }
    std::vector<Edge> v;
    Edge result;
    if(this->rank == destination) {
        for(int i = 0; i < this->nprocs; i++) {
            if (i == destination) {
                v.push_back(value);
            }
            else{
                Edge tmp = this->_recv_Edge(i);
                v.push_back(tmp);
            }
        }
        result = std::accumulate(std::begin(v)+1, std::end(v), *std::begin(v), binop);
    }
    else {
        result = Edge();
    }
    return result;
}


/*
 *  All to all reduction returning the reduction of each process's 'value' using
 *  the functor 'binop'. The result is broadcast to every process from process 0.
 */
int PSim::all2all_reduce(int value, std::function<int(int, int)>& binop) {
    int reduction = all2one_reduce(0, value, binop);
    int all_reduction = one2all_broadcast(0, reduction);
    return all_reduction;
}


/*
 *  All to all reduction returning the reduction of each process's Edge weighting using
 *  the functor 'binop'. The result is broadcast to every process from process 0.
 */
Edge PSim::all2all_reduce_E(Edge value, std::function<Edge(Edge, Edge)>& binop) {
    Edge reduction = all2one_reduce_E(0, value, binop);
    Edge all_reduction = one2all_broadcast_E(0, reduction);
    return all_reduction;
}


/*
 *  Barrier
 */
void PSim::barrier() {
    all2all_broadcast(0);
}





