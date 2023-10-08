#include <map>
#include <fstream>
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
#define MUTATE_RATE    10    // optimal 50%

using namespace std;


const char city_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/*
 * function to compare fitness and pass as reference to qsort
 */
int compare(const void* a, const void* b) {
    Trip* trip_a = (Trip*)a;
    Trip* trip_b = (Trip*)b;
    return (trip_a->fitness - trip_b->fitness);
}

/*
 * Evaluates each trip (or chromosome) and sort them out
 * CHANGE: instead of coordinates, cached distance matrix will be passed
 */
void evaluate( Trip trip[CHROMOSOMES], double distance_matrix[CITIES][CITIES], map<char, int> index_map ) {
	#pragma omp parallel for shared( trip )
	for ( int i = 0; i < CHROMOSOMES ; i++ ) {
		double distance = distance_matrix[index_map[trip[i].itinerary[0]]][index_map[trip[i].itinerary[1]]];
		for ( int j = 1; j < CITIES - 1; j++ ) {
			distance += distance_matrix[index_map[trip[i].itinerary[j]]][index_map[trip[i].itinerary[j+1]]];
		}
		trip[i].fitness = distance;
	}

	// sort the trips in descending order of fitness
	qsort( trip, CHROMOSOMES, sizeof( Trip ), compare );
}

/*
 * Generates new TOP_X offsprings from TOP_X parents.
 * Note that the i-th and (i+1)-th offsprings are created from the i-th and (i+1)-th parents
 * CHANGE: instead of coordinates, cached distance matrix will be passed
 */
void crossover( Trip parents[TOP_X], Trip offsprings[TOP_X], double distance_matrix[CITIES][CITIES], map<char, char> complement_map, map<char, int> index_map ) {
	map<char, bool> visited;

	#pragma omp parallel for shared( parents, offsprings, visited )
 	for (int i = 0; i < TOP_X; i++)
	{
		string parent1 = parents[i].itinerary;
		string parent2 = parents[i+1].itinerary;

		char child1[CITIES + 1] = " ";
		char child2[CITIES + 1] = " ";

		child1[0] = parent1[0];
		visited[parent1[0]] = true;

		for (int j = 1; j < CITIES; j++) {
			char prev_child = child1[j-1];

			int parent1_index = parent1.find(prev_child);
			int parent2_index = parent2.find(prev_child);

			int parent1_next_index = (parent1_index + 1) % CITIES;
			int parent2_next_index = (parent2_index + 1) % CITIES;

			if ( visited[parent1[parent1_next_index]] && visited[parent2[parent2_next_index]] ) {
				int rand_num = rand() % CITIES;
				char next_city = parent1[rand_num];
				
				while ( visited[next_city] ) {
					rand_num = (rand_num + 1) % CITIES;
					next_city = parent1[rand_num];
				}
				
				child1[j] = next_city;
			} else if ( visited[parent1[parent1_next_index]] ) {
				child1[j] = parent2[parent2_next_index];
			} else if ( visited[parent2[parent2_next_index]] ) {
				child1[j] = parent1[parent1_next_index];
			} else {
				int index_from = index_map[prev_child];
				int parent1_index_to = index_map[parent1[parent1_next_index]];
				int parent2_index_to = index_map[parent2[parent2_next_index]];

				if ( distance_matrix[index_from][parent1_index_to] <= distance_matrix[index_from][parent2_index_to] ) {
					child1[j] = parent1[parent1_next_index];
				} else {
					child1[j] = parent2[parent2_next_index];
				}
			}
			
			visited[child1[j]] = true;
		}

		for (int k = 0; k < CITIES; k++) 
			child2[k] = complement_map[child1[k]];
		
		strcpy(offsprings[i].itinerary, child1);
		strcpy(offsprings[i+1].itinerary, child2);
		visited.clear();
	}
}

/* 
 * Mutate a pair of genes in each offspring.
 */
void mutate( Trip offsprings[TOP_X], double distance_matrix[CITIES][CITIES], map<char, int> index_map ) {
	for (int i = 0; i < TOP_X; i++) {
		int rand_num = rand() % 100;
		if ( rand_num < MUTATE_RATE ) {
			int rand_num1 = rand() % CITIES;
			int rand_num2 = rand() % CITIES;
			
			char temp[CITIES];
			double temp_distance = 0.0;
			double offspring_distance = 0.0;

			strcpy(temp, offsprings[i].itinerary);
			temp[rand_num1] = offsprings[i].itinerary[rand_num2];
			temp[rand_num2] = offsprings[i].itinerary[rand_num1];
			for(int j = 0; j < CITIES - 1; j++) {
				int temp_index_from = index_map[temp[j]];
				int temp_index_to = index_map[temp[j+1]];
				temp_distance += distance_matrix[temp_index_from][temp_index_to];

				int index_from = index_map[offsprings[i].itinerary[j]];
				int index_to = index_map[offsprings[i].itinerary[j+1]];
				offspring_distance += distance_matrix[index_from][index_to];
			}	
			
			if ( temp_distance < offspring_distance ) {
				strcpy(offsprings[i].itinerary, temp);
			}		
		}
	}
}


/**
 * populate complement map during initial setup
*/
void set_complement_map( map<char, char> &complement_map ) {
	#pragma omp parallel for shared(complement_map)
	for(int i = 0; i < CITIES ; i++) {
		complement_map[city_map[i]] = city_map[CITIES - i - 1];
	}

	if ( DEBUG ) {
		for ( auto i : complement_map ) 
        cout << i.first << "-" << i.second << endl; 
	}
}

/**
 * evaluate and store distance matrix for faster retrieval later
*/
void set_distance_matrix( int coordinates[CITIES][2], double distance_matrix[CITIES][CITIES] ) {
	#pragma omp parallel for shared(distance_matrix)
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
				double distance = sqrt( ( distance_x*distance_x ) + ( distance_y*distance_y ) );
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

void set_index_map( map<char, int> &index_map ) {
	#pragma omp parallel for shared(index_map)
	for( int i = 0; i < CITIES ; i++ ) {
		index_map[city_map[i]] = i;
	}

	if ( DEBUG ) {
		for ( auto i : index_map ) 
				cout << i.first << "-" << i.second << endl; 
	}
}