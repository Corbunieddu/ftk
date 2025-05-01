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
#include <functional>
#include <algorithm>
#include <utility>
#include <functional>




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

    template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2>& p) {
    return os  << p.first << "," << p.second;
}


template <>
struct std::hash<Cluster> {
    std::size_t operator()(const Cluster& c) const {
        return (static_cast<std::size_t>(c.sensor) << 32) |
                (static_cast<std::size_t>(c.col) << 16) |
                (static_cast<std::size_t>(c.row));
    }
};

namespace std {
    template <>
    struct hash<std::pair<Cluster, Cluster>> {
        std::size_t operator()(const std::pair<Cluster, Cluster>& p) const {
            std::size_t h1 = std::hash<Cluster>{}(p.first);
            std::size_t h2 = std::hash<Cluster>{}(p.second);
            return h1 ^ (h2 << 1);  // o qualsiasi combinazione robusta
        }
    };
}

template <typename T>
using Clusters_Map = std::unordered_map<Cluster,T>;

template <typename T>
using Tracks_Map = std::unordered_map<std::pair<Cluster,Cluster>,T>;

class TracksHandler{
private:
    std::random_device rd;
    std::mt19937 g;
    
public:
    size_t n_events;
    std::vector<Cluster> clusters;
    std::vector<std::pair<Cluster,Cluster>> data;
    std::ifstream file;

    TracksHandler(const std::string& path_file_clusters, size_t n_events)
        : n_events(n_events) , file(path_file_clusters) { 
        g.seed(rd()); // Inizializza il generatore di numeri casuali
        if (!file.is_open()) {
            std::cerr << "Errore nell'aprire il file: " << path_file_clusters << std::endl;
            throw std::runtime_error("File non trovato");
        }
    }
    bool time_step(){
        // Carica sul vettore i clusters successivi liberando prima il vettore
        // Se non ci sono più cluster da leggere ritorna false

        clusters.clear();
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
            while (std::getline(ss, cluster_str, ';')) {
                std::stringstream cluster_ss(cluster_str);
                uint16_t row, col, sensor;
                char comma;
                if (cluster_ss >> sensor >> comma >> row >> comma >> col) {

                    Cluster cluster(row, col, sensor);
                    clusters.emplace_back(cluster);
                }
            }
            for(auto clu1 : clusters){
                for(auto clu2 : clusters){
                    if ( (int)(clu1.sensor/4) - (int)(clu2.sensor/4) == 1 ){
                        data.push_back(std::make_pair(clu1, clu2));
                    }
                }
            }
            clusters.clear();
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
    
        ClustersHandler(const std::string& path_file_clusters, size_t n_events)
            : n_events(n_events) , file(path_file_clusters) { 
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
    
                        Cluster cluster(row, col, sensor);
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