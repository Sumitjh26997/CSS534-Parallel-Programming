#include <vector>
#include <iostream>  // cout
#include <stdlib.h>  // rand
#include <math.h>    // sqrt, pow
#include <omp.h>     // OpenMP
#include <string.h>  // memset
#include "Timer.h"
#include "Trip.h"

#define CHROMOSOMES    50000 // 50000 different trips
#define CITIES         36    // 36 cities = ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
#define TOP_X          25000 // top optimal 25%
#define MUTATE_RATE    20    // optimal 50%

using namespace std;

const string city_map = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const string inverse_city_map = "9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA";

/*
 * function to compare fitness and pass as reference to qsort
 */
int compare(const void* a, const void* b) {
    // Convert the generic pointers to Trip pointers.
		Trip* trip_a = (Trip*)a;
    Trip* trip_b = (Trip*)b;

		// Compare the fitness (distance) of trip_a and trip_b.
    // The result determines the order of the trips in the sorted array.
    return (trip_a->fitness - trip_b->fitness);
}

/*
 * Evaluates each trip (or chromosome) and sort them out
 * CHANGE: instead of coordinates, cached distance matrix will be passed. Additional params index map and generation count has also been passd
 */
void evaluate( Trip trip[CHROMOSOMES], double distance_matrix[CITIES][CITIES], int generation ) {
	// Check if this is the first generation to determine where to start the iteration.
	int itr_start = ( generation == 0 ) ? 0 : TOP_X;
	// Use OpenMP for parallel processing, sharing relevant data among threads.
	#pragma omp parallel for firstprivate(itr_start) shared( trip, distance_matrix )
	for ( int i = itr_start ; i < CHROMOSOMES ; i++ ) {
		int index_to = city_map.find(trip[i].itinerary[0]);
		int index_from = city_map.find(trip[i].itinerary[1]);
		// Calculate the total distance (fitness) for the current itinerary.
		trip[i].fitness = distance_matrix[index_to][index_from];
		for ( int j = 1; j < CITIES - 1; j++ ) {
			index_to = city_map.find(trip[i].itinerary[j]);
			index_from = city_map.find(trip[i].itinerary[j+1]);
			trip[i].fitness += distance_matrix[index_to][index_from];
		}
	}

	// Sort the trips in descending order of fitness using quicksort (qsort).
	qsort( trip, CHROMOSOMES, sizeof( Trip ), compare );
}

/*
 * Generates new TOP_X offsprings from TOP_X parents.
 * Note that the i-th and (i+1)-th offsprings are created from the i-th and (i+1)-th parents
 * CHANGE: instead of coordinates, cached distance matrix will be passed
 */
void crossover( Trip parents[TOP_X], Trip offsprings[TOP_X], double distance_matrix[CITIES][CITIES] ) {
	// Create a map to keep track of visited cities during crossover.
	

	// Use OpenMP for parallel processing, sharing relevant data among threads.
	#pragma omp parallel for shared( parents, offsprings, distance_matrix ) 
 	for (int i = 0; i < TOP_X; i++)
	{
		vector <bool> visited(128, false);
		// cout<<"visited1: "<< visited<< endl;
		// Use OpenMP for parallel processing, sharing relevant data among threads.
		string parent1 = parents[i].itinerary;
		string parent2 = parents[i+1].itinerary;

		// Create two child arrays to store offspring itineraries.
		char child1[CITIES + 1];
		char child2[CITIES + 1];

		// Initialize the first city in child1 and mark it as visited.
		child1[0] = parent1[0];
		visited[parent1[0]] = true;

		// Iterate through the cities in the itinerary.
		for (int j = 1; j < CITIES; j++) {
			char prev_child = child1[j-1];

			// Find the indices of the previous city in both parents.
			int parent1_index = parent1.find(prev_child);
			int parent2_index = parent2.find(prev_child);

			// Calculate the indices of the next cities in both parents, wrapping around if necessary.
			int parent1_next_index = (parent1_index + 1) % CITIES;
			int parent2_next_index = (parent2_index + 1) % CITIES;

			// Check if both next cities are visited.
			if ( visited[parent1[parent1_next_index]] && visited[parent2[parent2_next_index]] ) {
				// If both are visited, select a random unvisited city.
				int rand_num = rand() % CITIES;
				char next_city = parent1[rand_num];
				
				// loop over till next unvisited city is found in parent1
				while ( visited[next_city] ) {
					rand_num = (rand_num + 1) % CITIES;
					next_city = parent1[rand_num];
				}
				
				child1[j] = next_city;
			} else if ( visited[parent1[parent1_next_index]] ) {
				// If only the next city in parent1 is visited, select city from parent2.
				child1[j] = parent2[parent2_next_index];
			} else if ( visited[parent2[parent2_next_index]] ) {
				// If only the next city in parent2 is visited, select city from parent1
				child1[j] = parent1[parent1_next_index];
			} else {
				// If both next cities are unvisited, choose next closest city
				int index_from = city_map.find(prev_child);
				int parent1_index_to = city_map.find(parent1[parent1_next_index]);
				int parent2_index_to = city_map.find(parent2[parent2_next_index]);

				if ( distance_matrix[index_from][parent1_index_to] <= distance_matrix[index_from][parent2_index_to] ) {
					child1[j] = parent1[parent1_next_index];
				} else {
					child1[j] = parent2[parent2_next_index];
				}
			}
			
			// Mark the selected city as visited.
			visited[child1[j]] = true;
		}

		// Create child2 by mapping complement of child1.
		for (int k = 0; k < CITIES; k++){
			int index = city_map.find(child1[k]);
			child2[k] = inverse_city_map[index];
		}
			
		
		// Copy the child itineraries to the offspring array.
		strcpy(offsprings[i].itinerary, child1);
		strcpy(offsprings[i+1].itinerary, child2);
		// Clear the visited cities map for the next iteration.
		visited.clear();
	}
}

/* 
 * Mutate a pair of genes in each offspring.
 */
void mutate( Trip offsprings[TOP_X], double distance_matrix[CITIES][CITIES] ) {
	for (int i = 0; i < TOP_X; i++) {
		// Generate a random number between 0 and 99.
		int rand_num = rand() % 100;
		if ( rand_num < MUTATE_RATE ) {
			// Create a temporary itinerary to hold the mutated version.
			char mutated[CITIES + 1];
			strcpy(mutated, offsprings[i].itinerary);
			
			// Generate two random indices for swapping cities in the itinerary.			
			int	rand_num1 = rand() % CITIES;
			int	rand_num2 = rand() % CITIES;

			// Swap the cities at the randomly selected indices in the temporary itinerary.
			mutated[rand_num1] = offsprings[i].itinerary[rand_num2];
			mutated[rand_num2] = offsprings[i].itinerary[rand_num1];

			// Initialize variables to calculate distances for the temporary and original itineraries.
			int mut_index_from = city_map.find(mutated[0]);
			int mut_index_to = city_map.find(mutated[1]);
			double mutated_distance = distance_matrix[mut_index_from][mut_index_to];

			int index_from = city_map.find(offsprings[i].itinerary[0]);
			int index_to = city_map.find(offsprings[i].itinerary[1]);
			double offspring_distance = distance_matrix[index_from][index_to];;
			
			// Calculate the fitness for the mutated and original itinerary.
			for( int j = 1; j < CITIES - 1; j++ ) {
				mut_index_from = city_map.find(mutated[j]);
				mut_index_to = city_map.find(mutated[j+1]);
				mutated_distance += distance_matrix[mut_index_from][mut_index_to];

				index_from = city_map.find(offsprings[i].itinerary[j]);
				index_to = city_map.find(offsprings[i].itinerary[j+1]);
				offspring_distance += distance_matrix[index_from][index_to];
			}

			// If the mutated itinerary is better (has a shorter distance) than the original itinerary, replace the original with the mutated itinerary.
			if ( mutated_distance < offspring_distance ) {
				strcpy( offsprings[i].itinerary, mutated );
			}
		}
	}
}

/**
 * evaluate and store distance matrix for faster retrieval later
*/
void set_distance_matrix( int coordinates[CITIES][2], double distance_matrix[CITIES][CITIES] ) {
	#pragma omp parallel for
	for( int i = 0; i < CITIES; i++ ) {
		for( int j = i; j < CITIES; j++ ) {
			// get (x,y) coordinates of city i 
			int x2 = coordinates[i][0];
			int y2 = coordinates[i][1];

			// if same city is selected set distance to 0 
			if ( i == j ) {
				distance_matrix[i][j] = 0.0;
			} else {
				// calculate euclidean distance with city j and store it in distance matrix as distance from i->j and j->i
				double distance_x = x2 - coordinates[j][0];
				double distance_y = y2 - coordinates[j][1];
				double distance = sqrt( ( distance_x * distance_x ) + ( distance_y * distance_y ) );
				distance_matrix[i][j] = distance_matrix[j][i] = distance;
			}
		}
	}

	if ( DEBUG ) {
		for( int i = 0; i < CITIES; i++ ) {
			for( int j = 0; j < CITIES; j++ ) {
				cout << distance_matrix[i][j] << "\t";
			}
			cout << endl;
		}
	}
}
