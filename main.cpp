#include <iostream>
#include <chrono>
#include <pthread.h>
#include <numa.h>
#include <thread>
#include <vector>
#include "tbb/tbb.h"

using namespace std;

#include "Tree.h"

const uint64_t COUNT = 10;

void loadKey(TID tid, Key &key) {
    // Store the key of the tuple into the key vector
    // Implementation is database specific
    key.setKeyLen(sizeof(tid));
    reinterpret_cast<uint64_t *>(&key[0])[0] = __builtin_bswap64(tid);
}

void singlethreaded(char **argv,size_t numa_thread,vector<int64_t> &durations) {
    // bind numa
    //获取当前线程的id
    pthread_t thread_id = pthread_self();
    // 获取 NUMA 节点 numa_thread 的 CPU 集合
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < numa_num_configured_cpus(); i++) {
        if (numa_node_of_cpu(i) == numa_thread) {
            CPU_SET(i, &cpuset);
        }
    }

    // 将当前线程绑定到 NUMA 节点 0 上
    int ret = pthread_setaffinity_np(thread_id, sizeof(cpuset), &cpuset);
    if (ret != 0) {
        std::cerr << "Failed to set thread affinity: " << ret << std::endl;
    }



    uint64_t n = std::atoll(argv[1]);
    uint64_t *keys = new uint64_t[n];

    // Generate keys
    for (uint64_t i = 0; i < n; i++)
        // dense, sorted
        keys[i] = i + 1;
    if (atoi(argv[2]) == 1)
        // dense, random
        std::random_shuffle(keys, keys + n);
    if (atoi(argv[2]) == 2)
        // "pseudo-sparse" (the most-significant leaf bit gets lost)
        for (uint64_t i = 0; i < n; i++)
            keys[i] = (static_cast<uint64_t>(rand()) << 32) | static_cast<uint64_t>(rand());

    
    ART_OLC::Tree tree(loadKey);
    //ART_ROWEX::Tree tree(loadKey);

    

    // Build tree
    {
        int64_t build_times=0;
        // for (size_t i = 0; i < COUNT; i++){
            auto starttime = std::chrono::system_clock::now();
            auto t = tree.getThreadInfo();
            
            for (uint64_t i = 0; i < n; i++) {
                Key key;
                loadKey(keys[i], key);
                tree.insert(key, keys[i], t);
            }
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            build_times+=duration.count();
        // }
        durations.push_back(build_times);
        // printf("%ld\n",duration.count());
        printf("insert,%ld,%f\n", n, (n * 1.0) / duration.count());
    }

    {
        // Lookup
        int64_t lookup_times=0;
        // for (size_t i = 0; i < COUNT; i++){
            auto starttime = std::chrono::system_clock::now();
            auto t = tree.getThreadInfo();
            for (uint64_t i = 0; i < n; i++) {
                Key key;
                loadKey(keys[i], key);
                auto val = tree.lookup(key, t);
                if (val != keys[i]) {
                    std::cout << "wrong key read: " << val << " expected:" << keys[i] << std::endl;
                    throw;
                }
            }
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            lookup_times+=duration.count();
        // }
        durations.push_back(lookup_times);
        printf("lookup,%ld,%f\n", n, (n * 1.0) / duration.count());
    }

    {
        
        int64_t update_times=0;
        // for (size_t i = 0; i < COUNT; i++){
             auto starttime = std::chrono::system_clock::now();
            auto t = tree.getThreadInfo();
            for (uint64_t i = 0; i < n; i++) {
                Key key;
                loadKey(keys[i], key);
                tree.remove(key, keys[i], t);
            }
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - starttime);
            update_times+=duration.count();
        // }
        durations.push_back(update_times);
        printf("remove,%ld,%f\n", n, (n * 1.0) / duration.count());
    }
    delete[] keys;
    // return durations;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: %s n 0|1|2\nn: number of keys\n0: sorted keys\n1: dense keys\n2: sparse keys\n", argv[0]);
        return 1;
    }
    vector<int64_t> durations;
    vector<int64_t> durations1;


   
    std::thread t1(singlethreaded, argv,atoi(argv[3]) ,std::ref(durations));
    t1.join();
    printf("insert time :%ld\n",durations[0]);
    printf("lookup time :%ld\n",durations[1]);
    printf("remove time :%ld\n",durations[2]);

        
    // std::thread t2(singlethreaded, argv,4,std::ref(durations1));
    // t2.join();
    // printf("insert time on same numa node:%ld\n",durations1[0]);
    // printf("lookup time on same numa node:%ld\n",durations1[1]);
    // printf("remove time on same numa node:%ld\n",durations1[2]);

    return 0;
}
