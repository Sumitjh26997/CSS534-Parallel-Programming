#include <iostream>
#include "Timer.h"
#include <stdlib.h>   // atoi
#include <math.h>
#include "mpi.h"
#define BUFSIZE 100

int default_size = 100;  // the default system size
int defaultCellWidth = 8;
double c = 1.0;      // wave speed
double dt = 0.1;     // time quantum
double dd = 2.0;     // change in system

using namespace std;



int main( int argc, char *argv[] ) {
	int my_rank;              // rank of process
  int mpi_size;                 // #processes
  // verify arguments
  if ( argc != 4 ) {
    cerr << "usage: Wave2D size max_time interval" << endl;
    return -1;
  }
  int size = atoi( argv[1] );
  int max_time = atoi( argv[2] );
  int interval  = atoi( argv[3] );

	MPI_Init( &argc, &argv ); // Start up MPI
	MPI_Comm_rank( MPI_COMM_WORLD, &my_rank ); // Find out process rank
	MPI_Comm_size( MPI_COMM_WORLD, &mpi_size );    // Find out # processes

  if ( size < 100 || max_time < 3 || interval < 0 ) {
    cerr << "usage: Wave2D size max_time interval" << endl;
    cerr << "       where size >= 100 && time >= 3 && interval >= 0" << endl;
    return -1;

	

// simulate wave diffusion from time = 2
for (int t = 2; t < max_time; t++) {
	int p = t % 3;
	int q = (t + 2) % 3;
	int r = (t + 1) % 3;

	for (int i = 1; i < size - 1; i++) {
		for (int j = 1; j < size - 1; j++) {
			

			

			// simulate wave diffusion from time = 2
			for (int t = 2; t < max_time; t++) {
				int p = t % 3;
				int q = (t + 2) % 3;
				int r = (t + 1) % 3;

				// rank 0 sends stripes of z[q] to all other ranks
				if (my_rank == 0) {
					for (int i = 1; i < mpi_size; i++) {
						int start_index = i * stripe_size;
						int end_index = start_index + stripe_size;
						MPI_Send(&z[q][start_index][0], stripe_size * size, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
					}
				} else {
					// all other ranks receive stripes of z[q] from rank 0
					MPI_Recv(&z[q][my_rank * stripe_size][0], stripe_size * size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}

				for (int i = 1; i < size - 1; i++) {
					for (int j = 1; j < size - 1; j++) {
						z[p][i][j] = 2.0 * z[q][i][j] - z[r][i][j] + pow(c, 2) * pow(dt / dd, 2)
							* (z[q][i + 1][j] + z[q][i - 1][j] + z[q][i][j + 1] + z[q][i][j - 1]
								- 4.0 * z[q][i][j]);
					}
				}

				if (interval != 0 && t % interval == 0) {
					cout << t << endl;
				}

				// rank 0 receives stripes of z[p] from all other ranks and prints z[p]
				if (my_rank == 0) {
					for (int i = 1; i < mpi_size; i++) {
						int start_index = i * stripe_size;
						int end_index = start_index + stripe_size;
						MPI_Recv(&z[p][start_index][0], stripe_size * size, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					}

					for (int i = 0; i < size; i++) {
						for (int j = 0; j < size; j++) {
							cout << z[p][i][j] << " ";
						}
						cout << endl;
					}
					cout << endl;
				} else {
					MPI_Send(&z[p][my_rank * stripe_size][0], stripe_size * size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
				}
			}

