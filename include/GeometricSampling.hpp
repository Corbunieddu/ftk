#ifndef FTK_GEOMETRICSAMPLING_HPP
#define FTK_GEOMETRICSAMPLING_HPP

#include <random>

class RandomGeometric {
private:
    std::mt19937 generator;                     // Generatore di numeri casuali
    std::geometric_distribution<> distribution; // Distribuzione geometrica

public:
    RandomGeometric();
    explicit RandomGeometric(double p);

    int generate();
    void setProbability(double p);
    void setSeed(unsigned int seed);
};

#endif // GeometricSampling_hpp