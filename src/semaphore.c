#include <sys/sem.h>
#include <stddef.h>
#include "semaphore.h"
#include "../inc/errExit.h"
#include <errno.h>
void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};


//    if (semop(semid, &sop, 1) == -1)
//        errExit("semop failed");
    int ret;
    do {
        ret =semop(semid, &sop, 1) ;
    } while (ret == -1 && errno == EINTR);



}

void elimina_semafori(int semid){
    // remove the created semaphore set
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        errExit("semctl IPC_RMID failed");

}

