#ifndef FTK_PATTERN_HPP
#define FTK_PATTERN_HPP

#include<DataHandler.hpp>

template <typename T>
using Patterns_Map = Clusters_Map<T>;

// using Pattern = std::pair<Cluster,Cluster>;
using Pattern = Cluster;

using DataHandler = ClustersHandler;

#endif //FTK_PATTERN_HPP