#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "genetic_algorithm_par.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = 0; i < object_count; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

int min(int a, int b) {
	if (a > b) return b;
	return a;
}

void mergeArrays(individual arr1[], individual arr2[], int n1,
                             int n2, individual arr3[])
{
    int i = 0, j = 0, k = 0;
 
    // Traverse both array
    while (i<n1 && j <n2)
    {
        if (cmpfunc(&arr1[i], &arr2[j]) <= 0)
            arr3[k++] = arr1[i++];
        else
            arr3[k++] = arr2[j++];
    }
 
    while (i < n1)
        arr3[k++] = arr1[i++];
 
    while (j < n2)
        arr3[k++] = arr2[j++];
}

//Thread function
void *run_genetic_algorithm(void *arg)
{
	params p = *(params *)arg;

	int count, cursor;
	individual *next_generation = (individual*) calloc(p.object_count, sizeof(individual));
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = 0; i < p.object_count; ++i) {
		(*p.current_generation)[i].fitness = 0;
		(*p.current_generation)[i].chromosomes = (int*) calloc(p.object_count, sizeof(int));
		(*p.current_generation)[i].chromosomes[i] = 1;
		(*p.current_generation)[i].index = i;
		(*p.current_generation)[i].chromosome_length = p.object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(p.object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = p.object_count;
	}

	individual *temp_array = (individual*) malloc(p.object_count * sizeof(individual));
	sorted_array = (individual*) malloc(p.object_count * sizeof(individual));

	// iterate for each generation
	for (int k = 0; k < p.generations_count; ++k) {
		cursor = 0;

		pthread_barrier_wait(&b);
		if (p.thread_id == 0){
			compute_fitness_function(p.objects, (*p.current_generation), p.object_count, p.sack_capacity);
		}
		pthread_barrier_wait(&b);
		// compute fitness and sort by it
		//qsort(current_generation, p.object_count, sizeof(individual), cmpfunc);

		////////////////////////////////////////////////////////////////////////
		qsort((*p.current_generation) + p.start, p.end - p.start, sizeof(individual), cmpfunc);

		pthread_barrier_wait(&b);

		if (p.thread_id == 0){
			individual *aux;
			int array_size = p.end - p.start;
			memcpy(temp_array, (*p.current_generation), array_size * sizeof(individual));

			for (int i = 1; i < p.num_threads; i++){
				int start = i * (double)p.object_count/p.num_threads;
				int end = min((i + 1)*(double)p.object_count/p.num_threads, p.object_count);
				mergeArrays((*p.current_generation) + start, temp_array, end - start, array_size, sorted_array);
				array_size += end - start;
				
				aux = sorted_array;
				sorted_array = temp_array;
				temp_array = aux;
			}

			memcpy((*p.current_generation), temp_array, sizeof(individual) * p.object_count);
		}

		////////////////////////////////////////////////////////////////////////

		pthread_barrier_wait(&b);
		if (p.thread_id != 0){
			continue;
		}

		// keep first 30% children (elite children selection)
		count = p.object_count * 3 / 10;
		for (int i = 0; i < count; ++i) {
			copy_individual((*p.current_generation) + i, next_generation + i);
		}
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = p.object_count * 2 / 10;
		for (int i = 0; i < count; ++i) {
			copy_individual((*p.current_generation) + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);
		}
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = p.object_count * 2 / 10;
		for (int i = 0; i < count; ++i) {
			copy_individual((*p.current_generation) + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = p.object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual((*p.current_generation) + p.object_count - 1, next_generation + cursor + count - 1);
			count--;
		}

		for (int i = 0; i < count; i += 2) {
			crossover((*p.current_generation) + i, next_generation + cursor + i, k);
		}

		// switch to new generation
		tmp = (*p.current_generation);
		(*p.current_generation) = next_generation;
		next_generation = tmp;

		for (int i = 0; i < p.object_count; ++i) {
			(*p.current_generation)[i].index = i;
		}

		if (k % 5 == 0) {
			print_best_fitness((*p.current_generation));
		}
	}

	pthread_barrier_wait(&b);
		if (p.thread_id == 0){
			compute_fitness_function(p.objects, (*p.current_generation), p.object_count, p.sack_capacity);
		}
		pthread_barrier_wait(&b);
		// compute fitness and sort by it
		//qsort(current_generation, p.object_count, sizeof(individual), cmpfunc);

		////////////////////////////////////////////////////////////////////////
		qsort((*p.current_generation) + p.start, p.end - p.start, sizeof(individual), cmpfunc);

		pthread_barrier_wait(&b);
		if (p.thread_id != 0){
			pthread_exit(NULL);
		}

		if (p.thread_id == 0){
			individual *aux;
			int array_size = p.end - p.start;
			memcpy(temp_array, (*p.current_generation), array_size * sizeof(individual));

			for (int i = 1; i < p.num_threads; i++){
				int start = i * (double)p.object_count/p.num_threads;
				int end = min((i + 1)*(double)p.object_count/p.num_threads, p.object_count);
				mergeArrays((*p.current_generation) + start, temp_array, end - start, array_size, sorted_array);
				array_size += end - start;
				
				aux = sorted_array;
				sorted_array = temp_array;
				temp_array = aux;
			}

			memcpy((*p.current_generation), temp_array, sizeof(individual) * p.object_count);
		}
	print_best_fitness((*p.current_generation));

	// free resources for old generation
	free_generation((*p.current_generation));
	//free_generation(next_generation);

	// free resources
	free((*p.current_generation));
	free(next_generation);

	pthread_exit(NULL);
}