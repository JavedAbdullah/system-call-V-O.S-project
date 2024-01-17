#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

#include <stdlib.h>

// the Request structure defines a request sent by a client
struct SharedData {
    // Add shared variables or structures here
//    char currentPlayer[100];
////    int rows;
////    int columns;
//    char gameBoard[100][100];
    int rows;
    int columns;
    char gameBoard[1];
};

struct PlayersInfo{
    char client1[100];
    char client2[100];
    char token1;
    char token2;
    int colonna_ultima_mossa;
    int player_counter;
};



// The alloc_shared_memory method creates, if it does not exist, a shared
// memory segment with size bytes and shmKey key.
// It returns the shmid on success, otherwise it terminates the calling process
int alloc_shared_memory(key_t shmKey, size_t size);

// The get_shared_memory attaches a shared memory segment in the logic address space
// of the calling process.
// It returns a pointer to the attached shared memory segment,
// otherwise it terminates the calling process
void *get_shared_memory(int shmid, int shmflg);

// The free_shared_memory detaches a shared memory segment from the logic
// address space of the calling process.
// If it does not succeed, it terminates the calling process
void free_shared_memory(void *ptr_sh);

// The remove_shared_memory removes a shared memory segment
// If it does not succeed, it terminates the calling process
void remove_shared_memory(int shmid);

//chiama i metodi sopra per liberare e rimuove spazio
void rimuovi_e_libera_spazio(void *ptr_sh,int shmid);

#endif