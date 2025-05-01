#include <iostream>
#include <random>
#include <chrono>
#include<unordered_map>
#include <fstream>
#include <sstream>
#include <numeric>
#include<atomic>
#include<mutex>
#include<deque>


#include"xxhash64.h"
#include"FTK.hpp"
#include"DataHandler.hpp"
#include"pattern.hpp"
#include"ThreadPool.hpp"



void leggi_tutti_i_cluster(const std::string& filename) {
    DataHandler dataHandler(filename, 100);
    ThreadPool pool(POOL_SIZE);
    FTK ftk(100000000, dataHandler, pool, 0.999, 150000);

    while (ftk.time_update()) {
        // if (ftk.time == 10000) {
        //     break;
        // }
        std::cout << "Time step: " << ftk.time <<"    Current iterations: "<<ftk.current_iterations<< std::endl;
    }

    saveOccorrenzeToCSV(ftk, "C:/Users/braua/Documents/TesiMagistrale/VELO/topk.txt", ftk.get_total_iterations());
}
void tracks_simulation(const std::string& filename) {
    DataHandler dataHandler(filename, 100);
    ThreadPool pool(POOL_SIZE);
    FTK ftk(100000000, dataHandler, pool, 0.999, 150000);

    while (ftk.time_update()) {
        if (ftk.time == 10000) {
            break;
        }
        std::cout << "Time step: " << ftk.time <<"    Current iterations: "<<ftk.current_iterations<< std::endl;
    }

    saveOccorrenzeToCSV(ftk, "/home/abrau/code/full_tracks.txt", ftk.get_total_iterations());
}

int main() {
    // sliding_window("C:/Users/braua/Documents/TesiMagistrale/VELO/clusters_output.txt", 300);
    tracks_simulation("/home/abrau/code/clusters_output.txt");
    return 0;
}