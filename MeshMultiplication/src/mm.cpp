/*
 * Mesh Multiplication - OpenMPI implementation
 *
 * Author: Jan Bednarik
 * Date: 3.5.2015
 *
 */

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <string>
#include <cassert>
#include <algorithm>

using namespace std;

#define TAG 0

// debug
#define DEBUGID 100

// file names
const char * MAT1_FILE_NAME = "mat1";
const char * MAT2_FILE_NAME = "mat2";

/*  Allocates memory for a materix and reads its content from file. 
*   @param fin opened file
*   @param mat matrix - 2D vector of ints
*   @param rows number of matrix rows
*   @param columns number of matrix columns
*/ 
void matRead(fstream &fin, vector<vector<int> > &mat, int *rows, int *columns) {
    string line;
    getline(fin, line);    

    while(getline(fin, line)) {                
        if(line.size() == 0) break; // extra newline at the end of file
        istringstream is(line);
        mat.push_back(vector<int>(istream_iterator<int>(is), istream_iterator<int>())); 
    }

    *rows = mat.size();
    *columns = mat[0].size();
}

/*  Main program function
*
*   Matrices format (mat1 x mat2):
*                
*          mat2        k
*               ---+---+---+---
*              |   |   |   |   |
*            n +---+---+---+---+
*              |   |   |   |   |
*  mat1         ---+---+---+---
*       n
*    --- ---    ---+---+---+--- 
*   |   |   |  |               |
*   +---+---+  +               +
* m |   |   |  |     m x k     |
*   +---+---+  +               +
*   |   |   |  |               |
*    ---+---    ---+---+---+---
*
*/
int main(int argc, char *argv[]) {
    int numprocs;       // # CPU
    int myid;           // Rank of this CPU
    MPI_Status stat;    
    int prod = 0;       // Resulting dot product for the field controled byt this process.
    int m, n, k;        // Matrices dimensions.

    // Initialize MPI
    MPI_Init(&argc,&argv);                          // Initializae MPI 
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // # of CPUs running
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);           // ID of this CPU

    /* --- CPU_0 --- */
    if(myid == 0) {
        int mat1r, mat1c, mat2r, mat2c;

        vector<vector<int> > mat1;
        vector<vector<int> > mat2;

        // read matrices
        fstream fin1, fin2;
        fin1.open(MAT1_FILE_NAME, ios::in); 
        fin2.open(MAT2_FILE_NAME, ios::in);    
        matRead(fin1, mat1, &mat1r, &mat1c); 
        matRead(fin2, mat2, &mat2r, &mat2c);               
        fin1.close(); 
        fin2.close();           

        assert(mat1c == mat2r);

        m = mat1r;
        n = mat1c;
        k = mat2c;

        // DEBUG - time measurement            
        // double tFrom_wtime = MPI::Wtime();

        // Send the number of columns of matrix 1 to each process
        for(int i = 1; i < numprocs; ++i) {
            MPI_Send(&m, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
            MPI_Send(&n, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
            MPI_Send(&k, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
        } 

        // Mesh multiplication algorithm        
        for(int i = 0; i < max(m + n - 1, n + k - 1); ++i) {                        
            // Send values from matrix 1 to all processes.
            if(i < m + n - 1) {
                int c = max(0, n - 1 - i);
                int r = i - n + c + 1;
                
                while(r < m && c < n) {
                    if(r > 0) {
                        int tmp = mat1[r][c];
                        MPI_Send(&tmp, 1, MPI_INT, r * k, TAG, MPI_COMM_WORLD); 
                    }
                    r += 1;                    
                    c += 1;
                }
            }


            // Send values from matrix 2 to all processes.
            if(i < n + k - 1) {                
                int r = max(0, n - 1 - i);
                int c = i - n + r + 1;
                
                while(r < n && c < k) {
                    if(c > 0) {
                        int tmp = mat2[r][c];
                        MPI_Send(&tmp, 1, MPI_INT, c, TAG, MPI_COMM_WORLD); 
                    }
                    r += 1;
                    c += 1;
                }
            }

            // compute product and send values L -> R, T -> B
            if(i < n) {
                int left = mat1[0][n - 1 - i];
                int top  = mat2[n - 1 - i][0];            
                prod += left * top;

                // Send current values from left and top vector to right and bottom processes.
                if(k > 1) MPI_Send(&left, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD); // LEFT -> RIGHT
                if(m > 1) MPI_Send(&top,  1, MPI_INT, k, TAG, MPI_COMM_WORLD); // TOP  -> DOWN
            }
        }

        // Read resulting values from all processes and print out
       cout << m << ":" << k << endl;
       cout << prod;
        for(int proci = 1; proci < numprocs; ++proci) {
            int tmp;                
            MPI_Recv(&tmp, 1, MPI_INT, proci, TAG, MPI_COMM_WORLD, &stat); 
            cout << ((proci % mat2c == 0) ? "\n" : " ") << tmp;                        
        }
       cout << endl;


        // DEBUG - time measurement                    
        // double tTo_wtime = MPI::Wtime();                
        // cout << 1000.0 * (tTo_wtime - tFrom_wtime) << endl;   
    }

    /* --- All CPUs except the first one --- */    
    if(myid > 0) {
        // Receive matrices dimensions frmo process 0.
        MPI_Recv(&m, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
        MPI_Recv(&n, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
        MPI_Recv(&k, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

        // Iteratively compute the dot product of the top and vector left vector, send values to right and bottom.
        int left, top;
        for(int i = 0; i < n; ++i) {
            // CPU [1][0]
            if((m > 1) && (myid == k)) {
                MPI_Recv(&top,  1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
                MPI_Recv(&left, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
            } else {
                MPI_Recv(&left, 1, MPI_INT, (myid == 1 || myid % k == 0) ? 0 : myid - 1, TAG, MPI_COMM_WORLD, &stat);
                MPI_Recv(&top,  1, MPI_INT, (myid <= k) ? 0 : myid - k,                  TAG, MPI_COMM_WORLD, &stat);
            }                    
            prod += left * top;

            if((myid + 1) % k != 0)   MPI_Send(&left, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);    
            if(myid < (numprocs - k)) MPI_Send(&top,  1, MPI_INT, myid + k, TAG, MPI_COMM_WORLD);                
        }
        MPI_Send(&prod, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD);
    }

    // Finalize MPI
    MPI_Finalize(); 
    return 0;
}
