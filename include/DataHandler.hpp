#ifndef FTK_DATAHANDLER_HPP
#define FTK_DATAHANDLER_HPP

#include <iostream>      
#include <fstream>
#include <sstream>       
#include <vector>               
#include <string>                        
#include <chrono>   
#include <queue>
#include <unordered_map>
#include <cstdint>
#include <random>
#include <algorithm>
#include <utility>
#include <functional>
#include <tuple>





class Cluster {
    public:
        uint16_t row, col, sensor;
    
        inline Cluster(uint16_t row, uint16_t col, uint16_t sensor)
            : row(row), col(col), sensor(sensor) {}
    
        inline Cluster() = default;
    
        inline bool operator==(const Cluster& other) const {
            return row == other.row && col == other.col && sensor == other.sensor;
        }
    
        inline bool operator<(const Cluster& other) const {
            if (sensor != other.sensor) return sensor < other.sensor;
            if (row != other.row) return row < other.row;
            return col < other.col;
        }
    
        friend inline std::ostream& operator<<(std::ostream& os, const Cluster& c) {
            os << c.sensor << ',' << c.row << ',' << c.col;
            return os;
        }
    };

    template<typename T1, typename T2,typename T3>
std::ostream& operator<<(std::ostream& os, const std::tuple<T1, T2, T3>& p) {
    return os  << std::get<0>(p) << "," << std::get<1>(p) << "," << std::get<2>(p);
}


template <>
struct std::hash<Cluster> {
    std::size_t operator()(const Cluster& c) const {
        return (static_cast<std::size_t>(c.sensor) << 32) |
                (static_cast<std::size_t>(c.col) << 16) |
                (static_cast<std::size_t>(c.row));
    }
};

inline void hash_combine(std::size_t& seed, std::size_t hash) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
    template <>
    struct hash<std::tuple<Cluster, Cluster, Cluster>> {
        std::size_t operator()(const std::tuple<Cluster, Cluster, Cluster>& t) const {
            std::size_t seed = 0;
            hash_combine(seed, std::hash<Cluster>{}(std::get<0>(t)));
            hash_combine(seed, std::hash<Cluster>{}(std::get<1>(t)));
            hash_combine(seed, std::hash<Cluster>{}(std::get<2>(t)));
            return seed;
        }
    };
}

template <typename T>
using Clusters_Map = std::unordered_map<Cluster,T>;

template <typename T>
using Tracks_Map = std::unordered_map<std::tuple<Cluster,Cluster,Cluster>,T>;

class TracksHandler{
private:
    std::random_device rd;
    std::mt19937 g;
    
public:
    size_t n_events;
    std::vector<Cluster> frames_0_clusters;
    std::vector<Cluster> frames_1_clusters;
    std::vector<Cluster> frames_2_clusters;

    std::vector<std::tuple<Cluster,Cluster,Cluster>> data;
    std::ifstream file;
    size_t collapsed_pixels;

    TracksHandler(const std::string& path_file_clusters, size_t n_events, size_t collapsed_pixels = 1)
        : n_events(n_events) , file(path_file_clusters), collapsed_pixels(collapsed_pixels) { 
        g.seed(rd()); // Inizializza il generatore di numeri casuali
        if (!file.is_open()) {
            std::cerr << "Errore nell'aprire il file: " << path_file_clusters << std::endl;
            throw std::runtime_error("File non trovato");
        }
    }
    bool time_step(){
        // Carica sul vettore i clusters successivi liberando prima il vettore
        // Se non ci sono più cluster da leggere ritorna false
        data.clear();
        if (file.eof()) {
            return false; // EOF
        }
        if (!read_tracks()) {
            return false; // Errore di lettura
        }
        return true; // Successo
    }
    bool read_tracks(){
        for(size_t i = 0; i < n_events; ++i) {
            std::string line;
            if (!std::getline(file, line)) {
                return false; // EOF o errore di lettura
            }
            if (line.empty()) {
                i--;
                continue; // Ignora le righe vuote
            }
            std::stringstream ss(line);
            std::string cluster_str;

            int frame = 10;
            while (std::getline(ss, cluster_str, ';')) {
                std::stringstream cluster_ss(cluster_str);
                uint16_t row, col, sensor;
                char comma;
                if (cluster_ss >> sensor >> comma >> row >> comma >> col) {
                    int current_frame = int(int(sensor) / 4);
                    Cluster cluster(row / collapsed_pixels, col / collapsed_pixels, sensor);

                    if (current_frame == frame){
                        frames_0_clusters.push_back(cluster);
                        }
                    else if (current_frame == frame + 2){
                        frames_1_clusters.push_back(cluster);
                    }
                    else if (current_frame == frame + 4){
                        frames_2_clusters.push_back(cluster);
                    }
                }
            }

            for(auto clu1 : frames_0_clusters){
                for(auto clu2 : frames_1_clusters){
                    for (auto clu3 : frames_2_clusters){

                        data.push_back(std::make_tuple(clu1, clu2, clu3));
                    }
                }
            }
            frames_0_clusters.clear();
            frames_1_clusters.clear();
            frames_2_clusters.clear();
        }
        return true; // Successo
    }
};

class ClustersHandler{
    private:
        std::random_device rd;
        std::mt19937 g;
        
    public:
        size_t n_events;
        std::vector<Cluster> data;
        std::ifstream file;
        size_t collapsed_pixels;
    
        ClustersHandler(const std::string& path_file_clusters, size_t n_events, size_t collapsed_pixels = 1)
            : n_events(n_events) , file(path_file_clusters), collapsed_pixels(collapsed_pixels){ 
            g.seed(rd()); // Inizializza il generatore di numeri casuali
            if (!file.is_open()) {
                std::cerr << "Errore nell'aprire il file: " << path_file_clusters << std::endl;
                throw std::runtime_error("File non trovato");
            }
        }
                
        bool read_clusters() {
            // il file è codificato in questo modo:
            // -ogni riga corrisponde ad un evento: ogni evento contiene 3 numeri interi che corrispondo ai cluster e sono separati da una virgola
            // i cluster sono rappresentati da 3 numeri interi: riga, colonna e sensore
            // esempio: 1,2,3. Ogni cluster è separato da un punto e virgola
            //esempio di file:
            // 1,2,3;4,5,6;7,8,9;\n
            // 10,11,12;13,14,15;16,17,18;\n
    
        
            for(size_t i = 0; i < n_events; ++i) {
                std::string line;
                if (!std::getline(file, line)) {
                    return false; // EOF o errore di lettura
                }
                if (line.empty()) {
                    i--;
                    continue; // Ignora le righe vuote
                }
                std::stringstream ss(line);
                std::string cluster_str;
                while (std::getline(ss, cluster_str, ';')) {
                    std::stringstream cluster_ss(cluster_str);
                    uint16_t row, col, sensor;
                    char comma;
                    if (cluster_ss >> sensor >> comma >> row >> comma >> col) {
    
                        Cluster cluster(row / collapsed_pixels, col / collapsed_pixels, sensor);

                        data.emplace_back(cluster);
                    }
                }
            }
            return true; // Successo
        }
        void shuffle_clusters() {
            // Mescola i cluster per evitare che siano ordinati in modo sequenziale
            std::shuffle(data.begin(), data.end(), g);
        }
        bool time_step() {
            // Carica sul vettore i clusters successivi liberando prima il vettore
            // Se non ci sono più cluster da leggere ritorna false
    
            data.clear();
            if (file.eof()) {
                return false; // EOF
            }
            if (!read_clusters()) {
                return false; // Errore di lettura
            }
    
            //shuffle dei cluster
            shuffle_clusters();
            return true; // Successo
        }
    };


#endif //FTK_DATAHANDLER_HPP