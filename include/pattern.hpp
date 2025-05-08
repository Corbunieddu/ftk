#ifndef FTK_PATTERN_HPP
#define FTK_PATTERN_HPP

#include<DataHandler.hpp>

template <typename T>
// using Patterns_Map = Clusters_Map<T>;
using Patterns_Map = Tracks_Map<T>;

using Pattern = std::tuple<Cluster,Cluster,Cluster>;
// using Pattern = Cluster;

// using DataHandler = ClustersHandler;
using DataHandler = TracksHandler;

#endif //FTK_PATTERN_HPP