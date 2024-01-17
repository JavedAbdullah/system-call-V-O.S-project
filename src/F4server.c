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
#include <stdbool.h>
#include "string.h"

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
int rows = 5, columns = 5;
char token_client1 = 'x', token_client2 = 'o';




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

//controllero se qualcuno ha vinto (ovviamente 0_0)
//ritorna -1(nessuno ha vinto) 1(vince client1) 2(vince client2) 3(pareggio)
int  controlla_se_qualcuno_ha_vinto(char (*gameBoard)[columns], int colonna_ultima_mossa){
    /* mio muovero solo nella colonna del ultima mossa, e la riga dove si e'
     * poggiato il gettone con l'ultima mossa
     * */



    //cerco la riga del ultima mossa
    int riga_ultima_mossa = -1;
    for (int i = 0; i<rows; i++) {
        if(gameBoard[i][colonna_ultima_mossa-1] == ' '){
            riga_ultima_mossa = i+1;
        }
    }
    //se con l'uttima mossa ho riempito la colonna
    if(riga_ultima_mossa == -1){
        riga_ultima_mossa = rows;
    }



    //cerco F4 nella colonna
    int F4_ct_client1 = 0; //contatore per client1
    int F4_ct_client2 = 0; //contatore per client2
    for (int i = rows-1; i>= 0; --i) {
        if(gameBoard[i][colonna_ultima_mossa-1] == token_client1){
            F4_ct_client1++;
            F4_ct_client2 = 0;
        }else if(gameBoard[i][colonna_ultima_mossa-1] == token_client2){
            F4_ct_client1 = 0;
            F4_ct_client2++;
        }
    }
    if(F4_ct_client1>=4){
        return 1;
    }else if(F4_ct_client2>=4){
        return 2;
    }

    //se nessuno ha ancora vinto, azzero i contatori
    F4_ct_client1 = 0; //contatore per client1
    F4_ct_client2 = 0; //contatore per client2


    //controllo sulla riga se qualcuno ha vinto
    printf("riga ultima mossa: %i\n colonna ultima mossa: %i\n",riga_ultima_mossa,colonna_ultima_mossa);
    for (int i = 0; i < columns; ++i) {
        printf("gameBoard[riga_ultima_mossa-1][i] = %c\n", gameBoard[riga_ultima_mossa][i]);
        if(gameBoard[riga_ultima_mossa][i] == token_client1){
            F4_ct_client1++;
            F4_ct_client2 = 0;
        }else if(gameBoard[riga_ultima_mossa][i] == token_client2){
            F4_ct_client1 = 0;
            F4_ct_client2++;
        }
    }

    if(F4_ct_client1>=4){
        return 1;
    }else if(F4_ct_client2>=4){
        return 2;
    }

    //se nessuno ha ancora vinto, cerco per il pareggio

    bool pareggio = true;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            if(gameBoard[i][j] == ' '){
                pareggio = false;
            }
        }
    }
    if(pareggio == true){
        return 3;
    }


    //printf for debugging parpouse
    //printf("riga ultima mossa: %i\n colonna ultima mossa: %i\n",riga_ultima_mossa,colonna_ultima_mossa);

    return -1;
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




    //queste info verranno prese in input (argv) dal server
    //int rows = 5, columns = 5; li ho resi globali



    // la size che andro ad allocare
    // allocate a shared memory segment
    printf("<Server> allocating a shared memory segment...\n");
    int shmidServer = alloc_shared_memory(shmKey, sizeof(rows*columns));
    // attach the shared memory segment
    printf("<Server> attaching the shared memory segment...\n");
    struct SharedData  *sharedData  = (struct SharedData *)get_shared_memory(shmidServer, 0);
    sharedData->rows = rows;
    sharedData->columns = columns;
    //creo il campo di gioco
    rows = sharedData->rows;
    columns = sharedData->columns;
    char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
    //per accedere:  gameBoard[i][j]


    // create a semaphore set
    printf("<Server> creating a semaphore set...\n");
    int semid = create_sem_set(semkey);


    //struct playersInfo <attendiamo il palyer_counter venga modificato>

    //struct playersInfo (creo uno spazio di memoria per struct PlayersInfo
    key_t semkeyPlayers = ftok(".", 'c');
    if (semkeyPlayers == -1)
        errExit("ftok failed");

   // printf("semekeyplayers: %i\n", semkeyPlayers);
    int shmidForPlayers = alloc_shared_memory(semkeyPlayers , sizeof(struct PlayersInfo));
    struct PlayersInfo  *playersInfo = (struct PlayersInfo*)get_shared_memory(shmidForPlayers, 0);

    //riempio le informazioni giusto per testare
    playersInfo->player_counter = 0;
    strcpy(playersInfo->client1, "Monty");
    strcpy(playersInfo->client2, "creatura magica");
    playersInfo->token1 = token_client1;
    playersInfo->token2 = token_client2;
    playersInfo->colonna_ultima_mossa = 1;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            if(j%2==0) {
                gameBoard[i][j] = ' ';
            }else{
                gameBoard[i][j] = ' ';
            }
        }
    }

    /* cominciamo il game */
    //provo a muovermi nella sincronizzazione per 5 volte
    printf("e' tutto pronto, attendo i client...\n");
    semOp(semid, attendi_client1, -1);
    printf("e' arrivato il client 1 : cha ha nome %s\n",playersInfo->client1);
    //manda un segnale al client 1
    semOp(semid, attendi_client2, -1);
    printf("e' arrivato il client 2 : cha ha nome %s\n",playersInfo->client2);
    semOp(semid, attendi_client1, 1);//sblocco il client 1
    printf("\n===========COMINCIAMO!===========\n");

    for (int i = 0; i < 5; ++i) {
        semOp(semid, sblocca_server, -1); //blocco il server, attendo che un client giochi
        //conterra -1(nessuno ha vinto), 1(vince client1), 2(vince client2), 3(vince client3)
        int valore_vincitore =  controlla_se_qualcuno_ha_vinto(gameBoard, playersInfo->colonna_ultima_mossa);
        if(valore_vincitore == 1){
            printf("vince client 1, bravo!!\n");
            //chiudo i client
        }else if(valore_vincitore == 2){
            printf("vince client 2, bravo!!\n");
            //chiudo i client
        }else if(valore_vincitore == 3){
            printf("PAREGGIO, BRAVI ENTRAMBI\n");
            //CHIUDO IL GIOCO
        }else if(valore_vincitore == -1){
            printf("nessuno ha vinto, CONTINUATE!\n");
            //CONTINUO IL GIOCO
        }



        semOp(semid, controlla_partita_server, -1);//sblocco i client che ha fatto l'utima
    }



//    gameBoard[4][0] = 'x';
//    gameBoard[3][0] = 'x';
//
//    gameBoard[2][0] = 'o';
//    gameBoard[2][1] = 'o';
//    gameBoard[2][2] = 'o';
//    gameBoard[2][3] = 'o';
    //stampa_campo_da_gioco(gameBoard); l'ho spostato nel client

    //conterra -1(nessuno ha vinto), 1(vince client1), 2(vince client2), 3(vince client3)
   int valore_vincitore =  controlla_se_qualcuno_ha_vinto(gameBoard, playersInfo->colonna_ultima_mossa);
   if(valore_vincitore == 1){
       printf("vince client 1, bravo!!\n");
       //chiudo i client
   }else if(valore_vincitore == 2){
       printf("vince client 2, bravo!!\n");
       //chiudo i client
   }else if(valore_vincitore == 3){
       printf("PAREGGIO, BRAVI ENTRAMBI\n");
       //CHIUDO IL GIOCO
   }else if(valore_vincitore == -1){
       printf("nessuno ha vinto, CONTINUATE!\n");
       //CONTINUO IL GIOCO
   }


//    semOp(semid, attendi_client1, -1);
//    printf("arrivato 1 client, evviva!!\n");
//    printf("player info counter: %d\n", playersInfo->player_counter);
//    semOp(semid, attendi_client2, -1);
//    printf("arrivati 2 client, evviva!!\n");
//    printf("player info counter: %d\n", playersInfo->player_counter);



    //elimino i semafori creati
    printf("<Server> removing semafori...\n");
    elimina_semafori(semid);

    // remove the shared memory segment
    printf("<Server> removing & detaching the shared memory segment...\n");
    rimuovi_e_libera_spazio(sharedData, shmidServer);
    //rimuovo lo spazio anche per playersInfo
    rimuovi_e_libera_spazio(playersInfo,shmidForPlayers);



    return 0;
}