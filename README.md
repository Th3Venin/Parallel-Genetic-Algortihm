# Parallel-Genetic-Algortihm

Simulation of a genetic algorithm implemented in C, parallelized using PThreads and synchronization mechanisms. 

The project consists in evaluating multiple potential solutions to a problem, being represented as a generation of individuals. The strongest individuals are selected by using a fitness function, formed of the following processes: selection, crossover and mutation. The genetic information (array of bits) is modified by the fitness algorithm so that the final solution only contains the best individuals (the final generation). The fitness function was parallelized so that the program is scalable with the number of cores used.

Input files for testing:
https://github.com/APD-UPB/APD/tree/master/teme/tema1
