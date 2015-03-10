/*
 *  main.cpp
 *  PSIM
 *  Created by Sam Uddin
 *
 *  Driver program. Various tests of the C++ PSim API
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <functional>
#include "psim.cpp"


//TEST MACROS
#define BOOST_SERIALIZATION 0
#define TOPOLOGY 0
#define BCAST 0
#define ALL_BCAST 0
#define SCATTER 1
#define COLLECT 0
#define REDUCE 0
#define ALL_REDUCE 0
#define PRIM_SEQUENTIAL 0
#define PRIM_PARALLEL 0


static void boost_serialization_test() {
    //Test Boost Serialization over pipes using Boost Archives and Boost File Descriptors
    pid_t pid;
    int fd[2];
    pipe(fd);
    
    //Serialize vector v1 in parent and send via i/o stream over pipe file descriptor to child
    if((pid = fork()) != 0) {
        std::vector<int> v1 = {33, 5, 6543, 540, 23, 537, 345, 234, 4, 65, 946};
        printf("parent @pid:%d => vector v1 to be serialized:\n", getpid());
        
        for(std::vector<int>::iterator it = v1.begin(); it != v1.end(); it++) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
        
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_sink> sbw(fd[1], boost::iostreams::never_close_handle);
        std::ostream os(&sbw);
        boost::archive::text_oarchive oa(os);
        oa << v1;
    }
    //De-Serialize vector sent over stream buffer in child
    else {
        std::vector<int> v2;
        printf("child @pid:%d => vector v1 de-serialized:\n", getpid());
        
        boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> sbr(fd[0], boost::iostreams::never_close_handle);
        std::istream is(&sbr);
        boost::archive::text_iarchive ia(is);
        ia >> v2;
        
        for(std::vector<int>::iterator it = v2.begin(); it != v2.end(); it++) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }
}


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


static void all_bcast_test() {
    printf("All processes send their rank^2 to all other processes:\n\n");
    PSim comm(6, SWITCH);
    std::vector<int> v = comm.all2all_broadcast(pow(comm.rank, 2));
    
    printf("@process %d (pid %d) => My collected values are: ", comm.rank, getpid());
    for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}


static void scatter_test() {
    std::vector<int> v1 = {33, 5, 6543, 540, 23, 537, 345, 234, 4, 65, 946};
    printf("Original vector: ");
    for(std::vector<int>::iterator it = v1.begin(); it != v1.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << std::endl << std::endl;
    PSim comm(4, SWITCH);
    std::vector<int> v = comm.one2all_scatter(0, v1);
    
    printf("@process %d (pid %d) => My unique vector slice is:\n", comm.rank, getpid());
    for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}


static void collect_test() {
    printf("All processes send their rank^3 to process #3:\n\n");
    PSim comm(6, SWITCH);
    std::vector<int> v = comm.all2one_collect(3, pow(comm.rank, 3));
    
    printf("@process %d (pid %d) => My collected vector by rank is: ", comm.rank, getpid());
    for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}


static void reduce_test() {
    PSim comm(5, SWITCH);
    int red_sum = comm.all2one_reduce(0, comm.rank, sum);
    printf("@process %d (pid %d) => reduction result of sum of ranks is: %d\n", comm.rank, getpid(), red_sum);
}


static void reduce_all_test() {
    PSim comm(6, SWITCH);
    int red_sum = comm.all2all_reduce(comm.rank, sum);
    printf("@process %d (pid %d) => reduction result of sum of all ranks is: %d\n", comm.rank, getpid(), red_sum);
}


/*
 *  MAIN()
 */

int main(int argc, const char * argv[]) {
    
#if(BOOST_SERIALIZATION)
    boost_serialization_test();
#endif
    
#if(TOPOLOGY)
    topology_test();
#endif
    
#if(BCAST)
    bcast_test();
#endif

#if(ALL_BCAST)
    all_bcast_test();
#endif
    
#if(SCATTER)
    scatter_test();
#endif
    
#if(COLLECT)
    collect_test();
#endif
    
#if(REDUCE)
    reduce_test();
#endif

#if(ALL_REDUCE)
    reduce_all_test();
#endif
    
#if(PRIM_SEQUENTIAL)
    
#endif
    
#if(PRIM_PARALLEL)
    
#endif
    
    return 0;
}


