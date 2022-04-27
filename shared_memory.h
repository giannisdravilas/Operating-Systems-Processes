#define MAXSIZE 100

struct shared_memory{
    int line;   // Desired line
    char buffer[MAXSIZE];   // Content of the desired line
    int completed;  // Child completed or not (0 for not completed, 1 for completed)
};

typedef struct shared_memory* sharedMemory;
