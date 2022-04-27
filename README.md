# Operating-Systems-Processes
💻 Cooperation between a parent process and several child processes in C using a shared memory segment and semaphores

## General
The parent process is initiated using three arguments given by the user, a filename, the number of child processes to be used and the number of transactions each child process is involved to. Afterwards, three semaphores, a shared memory segment and the given number of child processes are created.

The child procsesses act like a client in a Client-server model, asking the parent process for random lines of the file specified by the user, which the parent process returns to the child processes. This is achieved by using a shared memory segment, in which the child processes write the desired line number. The number is then read by the parent process, which also returns the content of the line to a buffer of the shared memory segment. This whole process is controlled by the three semaphores described before.

The processes are terminated when the desired number of transactions has been completed among all child procsesses.

## Technical details

A bug in valgrind-3.13.0 shows 3 errors for uninitialized values by sem_open() functions when running the current program. In valgrind-3.15.0 this problem is fixed.

## Make and run

```
make
./parent X K N
```
where X is the name of the input file, K is the number of child processes to be created and N is the number of transactions each child process is involved to.
