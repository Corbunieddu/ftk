#ifndef FTK_HPP
#define FTK_HPP


#include <iostream>
#include <random>
#include <chrono>
#include<unordered_map>
#include <numeric>
#include<atomic>
#include<mutex>
#include<deque>

#include"CostantSettings.hpp"
#include"ThreadPool.hpp"
#include"pattern.hpp"
#include"IndexedMinheap.hpp"
#include"DataHandler.hpp"
#include"GeometricSampling.hpp"
#include"fast_vector.hpp"
#include"xxhash64.h"



class FTK{
    private:

        RandomGeometric random_generator;
        DataHandler & handler;
        ThreadPool &pool;
        std::condition_variable cv;
        std::mutex mtx;
        std::atomic<int> task_mancanti;
        std::vector<size_t> topc_sum_level;

    public:
        std::vector<std::pair<uint64_t, int>> occorrenze;
        int DimensioneFinestra;
        int time = 0;
        size_t k;
        float total_iterations = 0;
        size_t current_iterations;
        Patterns_Map<int> Reference;
        std::vector<std::deque<FastVector<std::pair<MultiLevel,int>>>> circular_topc;
        std::vector<IndexedMinHeap<MultiLevel>> topc;
        std::vector<ReferenceHandler> referenceHandler;
        std::deque<size_t> deque_iterations;

        inline FTK(int DimensioneFinestra, DataHandler& handler, ThreadPool& pool, double p, size_t k)
            : handler(handler), pool(pool), task_mancanti(0), DimensioneFinestra(DimensioneFinestra), k(k){
            random_generator = RandomGeometric(1 - p);
            topc.resize(POOL_SIZE);
            circular_topc.resize(POOL_SIZE);
            referenceHandler.resize(POOL_SIZE);
        }

        // Permetti il movimento se necessario
        inline FTK(FTK&&) = default;                // Abilita il costruttore di spostamento
        inline FTK& operator=(FTK&&) = default;     // Abilita l'operatore di assegnazione per spostamento



        inline bool time_update(){
            if ( handler.time_step() == false ) return false;
            {
            time++;
            current_iterations = handler.data.size();
            deque_iterations.push_back(current_iterations);
            if (deque_iterations.size() > DimensioneFinestra) {
                deque_iterations.pop_front();
            }
            total_iterations += current_iterations;
            std::unique_lock<std::mutex> lock(mtx);
            task_mancanti.store(POOL_SIZE);

            for (int task = 0; task < POOL_SIZE; task++){
                std::mutex mtx;
                pool.enqueue([this, task, &mtx]{
                    auto &buffer_binari = handler.data;

                    Patterns_Map<MultiLevel> current_topc_task;

                    for (int idx = task; idx < current_iterations; idx += POOL_SIZE){
                        auto & box = buffer_binari[idx];

                        MultiLevel levels;
                        int sum = 0;
                        for ( size_t _i = 0; _i < DIM_MULTI_LEVEL; ++_i){
                            int value = 1 + random_generator.generate();
                            levels.levels[_i] = value;
                            levels.sum_levels += value;
                        }

                        current_topc_task[box].aggiorna(levels);
                        
                    }
                    auto &topc_task = topc[task];
                    auto &referenceHandler_task = referenceHandler[task];
                    auto &circular_topc_task = circular_topc[task];

                    circular_topc_task.push_back({});
                    auto & last = circular_topc_task.back();

                    
                    for (auto it = current_topc_task.begin(); it != current_topc_task.end(); it++){
                        size_t index = referenceHandler_task.insert_element(it->first);
                        last.push_back(std::make_pair(it->second, index));
                    }
                    std::sort(last.begin(), last.end(), std::greater<std::pair<MultiLevel,int>>());
                    
                    if (last.size() > k) {
                        auto size = last.size();
                        for (size_t ind = k; ind < size; ind++) referenceHandler_task.delete_element_byindex(last[ind].second);
                        last.resize(k);
                    }
                    topc_task.inizializza(referenceHandler_task.size(), k);
                    for ( int _ind = last.size() - 1; _ind >= 0; _ind--){
                        topc_task.aggiorna(last[_ind].second,last[_ind].first);
                    }
                    
                    if ( circular_topc_task.size() > DimensioneFinestra ) {
                        auto &front = circular_topc_task.front();
                        size_t size = front.size();
                        for ( size_t ind = 0; ind < size; ++ind){
                            referenceHandler_task.delete_element_byindex(front[ind].second);
                        }
                        circular_topc_task.pop_front();
                    }
                    auto begin = circular_topc_task.rbegin();
                    if (begin != circular_topc_task.rend()){
                        begin = std::next(begin);
                        auto circular_end = circular_topc_task.rend();
                        for (auto it = begin; it != circular_end; it++){
                            auto it2 = it->begin();
                            auto end = it->end();
                            while ( it2 != end ){
                                bool all_over = topc_task.aggiorna(it2->second,it2->first);
                                if (all_over){
                                    referenceHandler_task.delete_element_byindex(it2->second);
                                    it->fastErase(it2);
                                    end = it->end();
                                }
                                else {
                                    ++it2;
                                }
                            }
                            it->shrink_to_fit();
                        }
                    }

                    // Rimuovi i vector vuoti dalla deque
                    circular_topc_task.erase(
                        std::remove_if(circular_topc_task.begin(), circular_topc_task.end(),
                                    [](const FastVector<std::pair<MultiLevel, int>>& v) { return v.empty(); }),
                        circular_topc_task.end());

                    
                    task_mancanti.fetch_sub(1, std::memory_order_relaxed);
                    if (task_mancanti.load() == 0) {
                        cv.notify_all();

                    }
            });
        }
        cv.wait(lock, [&] { return task_mancanti.load() == 0; });
        }
        return true;
    }
    inline Patterns_Map<std::pair<int,int>> get_topc(){
        Patterns_Map<std::pair<int,int>> map;
        for ( size_t task = 0; task < POOL_SIZE; task++){
            auto &heap = topc[task].heap;
            size_t size = heap.size();
            for ( size_t i = 0; i < size; ++i){
                int sum = heap[i].first;
                size_t index = heap[i].second;
                auto box = referenceHandler[task].get_element(index);
                //std::cout<<index<<" "<<box.to_string()<<" "<<topc[task].indexed_map[index].second<<std::endl;
                std::pair<int,int> &pair = map[box];
                pair.first += sum;
                pair.second += DIM_MULTI_LEVEL;
            }
        }
        return map;
    }
    
    inline float get_total_iterations(){
        float total_iterations = 0;
        for (auto &window : deque_iterations){
            total_iterations += window;
        }
        return total_iterations;
    }
};



inline void saveOccorrenzeToCSV(FTK &ftk, const std::string& filename, float iterations) {
    auto occorrenze = ftk.get_topc();
    // Scrittura del file in UTF-8
    std::ofstream file(filename);
    file.imbue(std::locale::classic());

    file<<"# Total_Iterations; POOL_SIZE; DIM_MULTI_LEVEL\n";
    int time = ftk.time;
    if (time > ftk.DimensioneFinestra) time = ftk.DimensioneFinestra;
    file<<"#"<<iterations<<";"<<POOL_SIZE<<";"<<DIM_MULTI_LEVEL<<";\n";

    file<<"Pattern;";
    file<<"Sum_level;";
    file<<"Count_level\n";

    for (const auto& pair : occorrenze) {
        file << pair.first << ";" << pair.second.first << ";" << pair.second.second << "\n";
    }

    file.close();
}

#endif //FTK_HPP

