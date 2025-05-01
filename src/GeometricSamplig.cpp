#include "GeometricSampling.hpp"

RandomGeometric::RandomGeometric()
    : generator(std::random_device{}()), distribution(0.5) {}

RandomGeometric::RandomGeometric(double p)
    : generator(std::random_device{}()), distribution(p) {}

int RandomGeometric::generate() {
    return distribution(generator);
}

void RandomGeometric::setProbability(double p) {
    distribution = std::geometric_distribution<>(p);
}

void RandomGeometric::setSeed(unsigned int seed) {
    generator.seed(seed);
}
