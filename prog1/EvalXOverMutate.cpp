#include <map>
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
#define MUTATE_RATE    50    // optimal 50%

using namespace std;

const char city_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/*
 * Evaluates each trip (or chromosome) and sort them out
 * CHANGE: instead of coordinates, cached distance matrix will be passed
 */
void evaluate( Trip trip[CHROMOSOMES], double distance_matrix[CITIES][CITIES] ) {
	for (auto i=0; )
}

/*
 * Generates new TOP_X offsprings from TOP_X parents.
 * Note that the i-th and (i+1)-th offsprings are created from the i-th and (i+1)-th parents
 */
void crossover( Trip parents[TOP_X], Trip offsprings[TOP_X], int coordinates[CITIES][2] ) {

}

/* 
 * Mutate a pair of genes in each offspring.
 */
void mutate( Trip offsprings[TOP_X] ) {

}

/**
 * populate complement map during initial setup
*/
void set_complement_map(map<char, char> &complement_map) {
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
void set_distance_matrix(int coordinates[CITIES][2], double distance_matrix[CITIES][CITIES]) {
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
				auto distance_x = x2 - coordinates[j][0];
				auto distance_y = y2 - coordinates[j][1];
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