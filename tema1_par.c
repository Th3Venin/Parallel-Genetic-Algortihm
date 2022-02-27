#include <stdlib.h>
#include "genetic_algorithm_par.h"

int main(int argc, char *argv[]) {

	pthread_barrier_init(&b,NULL, atoi(argv[3]));
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv)) {
		return 0;
	}

	int i;
	pthread_t tid[atoi(argv[3])];

	//sorted_array = (individual*) calloc(object_count, sizeof(individual));
	individual *current_generation = (individual*)calloc(object_count, sizeof(individual));

	for (i = 0; i < atoi(argv[3]); i++) {
		params *p;
		p = malloc(sizeof(params));
		p->thread_id = i;
		p->num_threads = atoi(argv[3]);
		p->objects = objects;
		p->object_count = object_count;
		p->generations_count = generations_count;
		p->sack_capacity = sack_capacity;
		p->start = i * (double)object_count/atoi(argv[3]);
		p->end = min((i + 1)*(double)object_count/atoi(argv[3]), object_count);
		p->thread_array = (individual*) malloc((p->end - p->start) * sizeof(individual));
		p->current_generation = &current_generation;

		if(pthread_create(&tid[i], NULL, run_genetic_algorithm, (void *)p)){
			return 0;
		}
	}

	// se asteapta thread-urile
	for (i = 0; i < atoi(argv[3]); i++) {
		if(pthread_join(tid[i], NULL)){
			return 0;
		}
	}

	free(objects);

	return 0;
}
