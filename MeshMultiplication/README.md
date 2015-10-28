# Mesh Multiplication

The implementaion of the parallel algorithm Mesh Multiplication using OpenMPI library.

## Dependencies
* OpenMPI (mpic++)
* Python 2.7
* NumPy

## Compile and Run
#### 1) Generate matrices
```
$ cd src
$ python genmat.py -n
```
where *n* is the size of a each matrix (square matrices).
#### 2) Run the algorithm
```
$ cd src
$ chmod +x test.sh
$ ./test.sh
```
