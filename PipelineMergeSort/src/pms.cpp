/*
 * Pipeline Merge Sort - OpenMPI implementation
 *
 * Author: Jan Bednarik
 *
 */

#include <mpi.h>
#include <iostream>
#include <fstream>

// DEBUG - time measurement 
#include <mpi.h>

using namespace std;

#define TAG 0
#define DEBUG_ID 100

int main(int argc, char *argv[]) {
    int numprocs;   // # CPU
    int myid;       // Rank of this CPU
    MPI_Status stat;

    // Initialize MPI
    MPI_Init(&argc,&argv);                          // Initializae MPI 
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // # of CPUs running
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);           // ID of this CPU

    // buffer for input numbers
    int numbersCount = 1 << (numprocs - 1);
    int numbers[numbersCount];  

    /* --- CPU_0 --- */
    if(myid == 0) {
        // Reading file
        char input[] = "numbers";
        int number;
        int i = 0;
        fstream fin;
        fin.open(input, ios::in);                   

        while(fin.good()){
            number = fin.get();
            if(!fin.good()) break;            
            numbers[i++] = number;            
        }
        fin.close();           

        // Print out numbers (first line of the output)
        for(int i = 0; i < numbersCount; ++i)
            cout << numbers[i] << " ";
        cout << endl;

        // DEBUG - time measurement            
        // double tFrom_wtime = MPI::Wtime();

        // Send numbers to CPU 1
        for(int i = 0; i < numbersCount; ++i) {
            MPI_Send(&numbers[i], 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);  
        }        

        // DEBUG - time measurement            
        // char aux;
        // MPI_Recv(&aux, 1, MPI_CHAR, numprocs - 1, TAG, MPI_COMM_WORLD, &stat);                        
        // double tTo_wtime = MPI::Wtime();                
        // cout << "CPU time used (wtime): " << 1000.0 * (tTo_wtime - tFrom_wtime) << " ms" << endl;    
    }

    /* --- All CPUs except the first one --- */    
    if(myid > 0) {
        int bufLen = 1 << (myid - 1);
        // Input double buffers - top ([][0..bufLen-1]) and bottom ([][bufLen..2*bufLen-1])
        int buf[2][bufLen << 1];                
        // Compare Indices        
        int icT = 0;
        int icB = bufLen;
        // Double buffer Compare index (0/1)        
        int icDB = 0;
        // Flag indicating whether the CPU should start comparing values from top and bottom buffer
        bool compare = false;

        // auxiliary variables to evaluate double buffer index        
        int pow2rankPlus1 = bufLen << 2;
        int pow2rank      = bufLen << 1;    

        for(int clk = 0; clk < numbersCount + bufLen; ++clk) {
            if(clk == bufLen) compare = true;        

            if(clk < numbersCount) {
                int tmp;                
                MPI_Recv(&tmp, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat); 
                
                // write            
                buf[int((clk % pow2rankPlus1) >= pow2rank)][clk % (2 * bufLen)] = tmp;            
            }

            // compare and send the lower value to next CPU
            if(compare) {
                int lower;

                // switch to the other buffer (we use double buffer)
                if(icT >= bufLen && icB >= 2 * bufLen) {
                    icDB = 1 - icDB;
                    icT  = 0;
                    icB  = bufLen;
                }
                    
                // compare the numbers in top and bottom buffer
                if(icT < bufLen && icB < (bufLen << 1)) 
                    lower = (buf[icDB][icT] < buf[icDB][icB]) ? buf[icDB][icT++] : buf[icDB][icB++];                    
                else
                    lower = (icT >= bufLen) ? buf[icDB][icB++] : buf[icDB][icT++];

                if(myid < numprocs - 1) 
                    MPI_Send(&lower, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);                
                else                    
                    // Last CPU - print out the result            
                    cout << lower << endl;                                 
            }
        }

        // DEBUG - time measurement
        // if(myid == numprocs - 1) {
        //     char aux = 0;
        //     MPI_Send(&aux, 1, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
        // }
    }

    // Finalize MPI
    MPI_Finalize(); 
    return 0;
}
