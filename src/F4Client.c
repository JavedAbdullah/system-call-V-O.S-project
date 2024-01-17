/************************************
*VR471543
*Javed Abdullah
*2/01/2024
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "../inc/shared_memory.h"
#include "../inc/semaphore.h"
#include "../inc/errExit.h"

#define attendi_client1 0 //serviranno per capire al server, quando arrivano i client
#define attendi_client2 1
#define just_play_client1 2 //seviranno per sincornizzare i client tra di loro
#define just_play_client2 3
#define sblocca_server 4 //serviranno per sincronizzare i client con il server
#define controlla_partita_server 5


//rendo variabili globali per necessita nella fase implementativa
int rows = 5, columns = 5; // queste informazioni verranno prese dalla memoria condivisa





//stampa campo da gioco:
void stampa_campo_da_gioco(char (*gameBoard)[columns]){

    printf("~ sciegli la colonna: ~\n");
    for (int i = 0; i < rows; ++i) {
        printf(" ~  ");
    }
    printf("\n");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            //gameBoard[i][j] = 'H';
            printf("[%c] ", gameBoard[i][j]);
        }
        printf("\n");
    }
    //stampo per numero di colonne
    for (int i = 0; i < rows; ++i) {
        printf(" ~  ");
    }
    printf("\n");
    for (int i = 0; i < rows; ++i) {
        printf(" %i  ",i+1);
    }
    printf("\n");

}







int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(semkey, 6, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set
    union semun arg;
    unsigned short values[] = {0, 0, 0, 0, 0, 0};
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
        errExit("semctl SETALL failed");

    return semid;
}

int main() {

    //chiave della memoria condivisa
    //key_t shmKey = 123;
    key_t shmKey = ftok(".", 'm');
    if ( shmKey == -1)
        errExit(" shared_memory ftok failed");
    //chaive del set di semafori
    //key_t semkey = 456;
    key_t semkey = ftok(".", 's');
    if ( semkey== -1)
        errExit("semaphore ftok failed");



    // la size che andro ad allocare
    // allocate a shared memory segment
    printf("<client> allocating a shared memory segment...\n");
    int shmidServer = alloc_shared_memory(shmKey, sizeof(rows*columns));

    // attach the shared memory segment
    printf("<client> attaching the shared memory segment...\n");
    struct SharedData  *sharedData  = (struct SharedData *)get_shared_memory(shmidServer, 0);

    // create a semaphore set
    printf("<client> creating a semaphore set...\n");
    int semid = create_sem_set(semkey);


    rows = sharedData->rows;
    columns = sharedData->columns;

    char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
    //per accedere:  gameBoard[i][j]



    //struct playersInfo (accedo allo spazio allocato dal server)
    key_t semkeyPlayers = ftok(".", 'c');
    if (semkeyPlayers == -1)
        errExit("ftok failed");
    int shmidForPlayers = alloc_shared_memory(semkeyPlayers , sizeof(struct PlayersInfo));
    struct PlayersInfo  *playersInfo = (struct PlayersInfo*)get_shared_memory(shmidForPlayers, 0);

    playersInfo->player_counter++;

    if(!(playersInfo->player_counter == 2)){
        //saro dentro client 1, faccio andare avanti il server
        printf("mi collego da client 1\n");
        semOp(semid, attendi_client1, 1);
        printf("attendo che il server mi dica che e' arrivato il client 2\n");
        //semOp(semid, attendi_client1, -1);
    }

    if(playersInfo->player_counter == 2){
        //saro dentro client 2, faccio andare avabti il server
        printf("mi collego da client 2\n");
        semOp(semid, attendi_client2, 1);
    }
    //let's play
    char my_token;
    char my_name; //li mettero il suo nome scelto
    if(playersInfo->player_counter == 2){
        my_token = playersInfo->token2;
    }else{
        my_token = playersInfo->token1;
    }
    printf("\n===========Cominciamo=============\n");

    for (int i = 0; i < 5; ++i) { //lo faccio girare solo per 5 volte, ma dovrebbe essere while(true)
        if(playersInfo->player_counter == 2){
            semOp(semid, just_play_client2, -1);//mi blocco affinche client 1 possa fare la sua mossa
        }
        //stampo il campo di gioco
        stampa_campo_da_gioco(gameBoard);
        int mossa_cliente;
        printf("inserisci una colonna dove inserire: %c \n",my_token);
        scanf("%d",&mossa_cliente);//devo fare il controllo che quello che inserisco sia giusto
        //move

        //prima di continuare, comunico al server che ho fatto la mossa
        semOp(semid, sblocca_server, 1);//sbolocco il server, affinche possa fare i controlli
        semOp(semid, controlla_partita_server, -1);//mi blocco affinche il server possa controllare

        if(playersInfo->player_counter == 2){
            semOp(semid, just_play_client1, 1); //sblocco il client 1
        }else{
            semOp(semid, just_play_client2, 1); //sblocco client 2
            semOp(semid, just_play_client1, -1);//blocco il client 1
        }


    }



    return 0;
}