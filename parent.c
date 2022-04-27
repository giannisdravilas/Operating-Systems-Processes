#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <semaphore.h>

#include <time.h>
#include <fcntl.h>

#include <sys/shm.h>
#include <sys/wait.h>

#include "shared_memory.h"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

// Function to count lines of a file
int file_lines(FILE* fp){
    int lines = 0;
    while(EOF != (fscanf(fp, "%*[^\n]"), fscanf(fp, "%*c")))
        lines++;
    return lines;
}

void child(int nlines, int N, int i, sharedMemory semlock, void* requestProducer, void* answerProducer, void* answerConsumer);

int main(int argc, char *argv[]){

    // Save info from command line arguments
    char* filename;
    filename = argv[1];
    int K = atoi(argv[2]);
    int N = atoi(argv[3]);

    // Initialize 3 named semaphores
    sem_t* requestProducer = sem_open("requestProducer", O_CREAT | O_EXCL, SEM_PERMS, 0);

    if(requestProducer == SEM_FAILED){
        perror("sem_open(0) failed");
        exit(EXIT_FAILURE);
    }

    sem_t* answerProducer = sem_open("answerProducer", O_CREAT | O_EXCL, SEM_PERMS, 0);

    if(answerProducer == SEM_FAILED){
        perror("sem_open(1) failed");
        exit(EXIT_FAILURE);
    }

    sem_t* answerConsumer = sem_open("answerConsumer", O_CREAT | O_EXCL, SEM_PERMS, 0);

    if(answerConsumer == SEM_FAILED){
        perror("sem_open(2) failed");
        exit(EXIT_FAILURE);
    }

    // Open file given from user and count lines
    FILE* fp = fopen(filename, "r");
    int nlines = file_lines(fp);
    fclose(fp);

    int shmid;
    sharedMemory semlock;

    // Create memory segment
    if((shmid = shmget(IPC_PRIVATE, sizeof(sharedMemory), (S_IRUSR|S_IWUSR))) == -1){
        perror("Failed to create shared memory segment");
        return 1;
    }

    // Attach memory segment
    if((semlock = (sharedMemory)shmat(shmid, NULL, 0)) == (void*)-1){
        perror("Failed to attach memory segment");
        return 1;
    }

    // Children array
    pid_t pids[K];

    // Initialize K kids
    int i = 0;
    for(i = 0; i < K; i++){
        if((pids[i] = fork()) < 0){ // Fork new process
            perror("Failed to create process");
            return 1;
        }
        if(pids[i] == 0){          // If it is child process
            child(nlines, N, i, semlock, requestProducer, answerProducer, answerConsumer);
            exit(0);
        }
        // When last child is initiated, request as producer from the consumer
        if(i == K-1){
            if(sem_post(requestProducer) < 0){
                perror("sem_post failed on parent");
                exit(EXIT_FAILURE);
            }     
        }
    }

    int children_finished = 0;
    while(children_finished < K){

        // Wait for answer from consumer
        if(sem_wait(answerConsumer) < 0){
            perror("sem_wait failed on parent");
            exit(EXIT_FAILURE);
        }

        semlock->completed = 0;     // At start the child has not completed all of its processes
        int desired_line = semlock->line;   // Get desired line from the consumer

        // Open file and search for that line, copy it to shared memory's buffer
        FILE* fp1 = fopen(filename, "r");
        char line[MAXSIZE + 2]; // 100+2 characters to also save '\n' and '\0' at the end of the line
        int j = 0;
        while(fgets(line, sizeof(line), fp1)){
            j++;
            if(j == desired_line){
                strcpy(semlock->buffer, line);   
            }
        }
        fclose(fp1);

        // Consumer receives producer's answer
        if(sem_post(answerProducer) < 0){
            perror("sem_post failed on parent");
            exit(EXIT_FAILURE);
        }

        // Wait for consumer to read the answer and answer to the producer
        if(sem_wait(answerConsumer) < 0){
            perror("sem_wait failed on parent");
            exit(EXIT_FAILURE);
        }

        // If consumer informs that it has completed K processes, one more child has finished, we set shared memory's
        // completed flag to 0 again, waiting for the next 1
        if(semlock->completed == 1){
            children_finished++;
            semlock->completed = 0;
        }

        // Another child process can begin
        if(sem_post(requestProducer) < 0){
            perror("sem_post failed on parent");
            exit(EXIT_FAILURE);
        }
    }

    int status;

    // Collect children that have finished
    for(int i = 0; i < K; i++){
        wait(&status);
    }

    // Detach shared memory
    if(shmdt((void*)semlock) == -1){
       perror("Failed to destroy shared memory segment");
       return 1;
   }

    // Close and unlink semaphores
    if(sem_close(requestProducer) < 0){
        perror("sem_close(0) failed");
        exit(EXIT_FAILURE);
    }
    if(sem_unlink("requestProducer") < 0){
        perror("sem_unlink(0) failed");
        exit(EXIT_FAILURE);
    }

    if(sem_close(answerProducer) < 0){
        perror("sem_close(1) failed");
        exit(EXIT_FAILURE);
    }
    if(sem_unlink("answerProducer") < 0){
        perror("sem_unlink(1) failed");
        exit(EXIT_FAILURE);
    }

    if(sem_close(answerConsumer) < 0){
        perror("sem_close(2) failed");
        exit(EXIT_FAILURE);
    }
    if(sem_unlink("answerConsumer") < 0){
        perror("sem_unlink(2) failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
