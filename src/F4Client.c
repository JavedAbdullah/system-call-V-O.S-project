/************************************
*VR471543
*Javed Abdullah
*2/01/2024
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <signal.h>
#include "../inc/shared_memory.h"
#include "../inc/semaphore.h"
#include "../inc/errExit.h"
#include "../inc/coda_messaggi.h"

#define attendi_client1 0 //serviranno per capire al server, quando arrivano i client
#define attendi_client2 1
#define just_play_client1 2 //seviranno per sincornizzare i client tra di loro
#define just_play_client2 3
#define sblocca_server 4 //serviranno per sincronizzare i client con il server
#define controlla_partita_server 5
#define attend_pid_client1 6 //mi servira per sincronizzare la coda di messaggi (non ho ancora capito se avro bisgono)
#define attend_pid_client2 7
#define attend_pid_server 8


//rendo variabili globali per necessita nella fase implementativa
int rows = 5, columns = 5; // queste informazioni verranno prese dalla memoria condivisa

// the message queue identifier
int msqid = -1;




/*
    SIG. HANDLERS
*/
void handlerSegnali(int sig){


}



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

void inserisci_token(char (*gameBoard)[columns], char token_inserito){
    int mossa_cliente;
    //printf("io dovrei essere una scanf\n");
    scanf("%d",&mossa_cliente);//devo fare il controllo che quello che inserisco sia giusto
    bool inserito = false;
    while(1){
        for (int i = rows-1; i>= 0; --i) {
            printf("entro nel for:\n");
            if(gameBoard[i][mossa_cliente-1] == ' '){
                inserito = true;
                gameBoard[i][mossa_cliente-1] = token_inserito;
                stampa_campo_da_gioco(gameBoard);
                break;
            }
        }

        if(!inserito){
            printf("inserimento fallito, sciegliere un'altra colonna!\n");
            scanf("%d",&mossa_cliente);//devo fare il controllo che quello che inserisco sia giusto
        }else{
            break;
        }
    }

}







int create_sem_set(key_t semkey) {
    // Create a semaphore set with ? semaphores
    int semid = semget(semkey, 9, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set
    union semun arg;
    unsigned short values[] = {0, 0, 0, 0, 0, 0,0,0,0};
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



    /*===================inzio gestione segnali==========================*/
    /*
     Set di segnali per il server
 */
    sigset_t setSegnali;
    sigfillset(&setSegnali);

    sigdelset(&setSegnali, SIGUSR1);
    sigdelset(&setSegnali, SIGHUP);//segnael della chiusura del terminale
    sigdelset(&setSegnali, SIGUSR2);
    sigdelset(&setSegnali, SIGINT);
    sigdelset(&setSegnali, SIGALRM);

    sigprocmask(SIG_SETMASK, &setSegnali, NULL);
    if(signal(SIGUSR1, handlerSegnali) == SIG_ERR || signal(SIGINT, handlerSegnali) == SIG_ERR || signal(SIGALRM, handlerSegnali) == SIG_ERR || signal(SIGUSR2, handlerSegnali) == SIG_ERR || signal(SIGHUP, handlerSegnali) == SIG_ERR){
        printf("<server> Errore nel settare l'handler segnali. \n");
        return 0;
    }



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

    bool im_client1 = false;
    bool im_client2 = false; //mi serviranni per determinare che giocatore sono

    if(playersInfo->player_counter == 1){
        im_client2 = true;
    }else if(playersInfo->player_counter == 0){
        im_client1 =  true;
    }
    playersInfo->player_counter++;
    printf("playersInfo->player_counter: %i\n", playersInfo->player_counter);
    if(im_client1){
        playersInfo->pid_client1 = getpid();
        printf("in quanto client 1, ho pid %i\n",  getpid());
        //saro dentro client 1, faccio andare avanti il server
        printf("mi collego da client 1\n");
        semOp(semid, attendi_client1, 1);
        printf("attendo che il server mi dica che e' arrivato il client 2\n");
        semOp(semid, attendi_client1, -1);
    }

    if(im_client2){
        playersInfo->pid_client2 = getpid();
        printf("in quanto client 2, ho pid %i\n",  getpid());
        //saro dentro client 2, faccio andare avabti il server
        printf("mi collego da client 2\n");
        semOp(semid, attendi_client2, 1);
        //semOp(semid, attendi_client2, -1);//mi blocco in attesa di ordini
    }

    printf("server, ha pid %i\n", playersInfo->pid_server);
    printf("client 1, ha pid %i\n",  playersInfo->pid_client1);
    printf("client 2, ha pid %i\n",  playersInfo->pid_client2);




//==============tentativo fallito================================
    //comincia la creazione di una coda di messaggi
//    int msgKey= ftok(".", 'q'); //(q - for queue)
//    if ( msgKey == -1)
//        errExit(" msgKey  ftok failed");

//    printf("<client> Making MSG queue...\n");
//    // get the message queue, or create a new one if it does not exist


    //create a struct del messaggio da inviare
    //pid_di_tutti.mtype = 1;
   // pid_di_tutti.pid_server = getpid();

//    size_t mSize;
//    int pid_server;
//
//    if(im_client1){
//
//       // semOp(semid, attend_pid_server, -1);
//        msqid = msgget(  msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
//        if (msqid == -1)
//            errExit("msgget failed");
//        struct IvariPid pid_di_tutti;
//        size_t mSize = sizeof(struct IvariPid) - sizeof(long);
//        if (msgrcv(msqid, &pid_di_tutti, mSize, 0, 0) == -1)
//            errExit("msgget failed");
//       // pid_server = pid_di_tutti.pid_server;
//        printf("ho ricevuto il pid del server %i \n", pid_di_tutti.pid_server);
//        printf("ricarico la struct da inviare \n");
//        pid_di_tutti.pid_client1 = getpid();
//        printf("il mio pid e' %i, quello nella struct e' %i \n", getpid(), pid_di_tutti.pid_client1);
//        if (msgsnd(msqid, &pid_di_tutti, mSize, 0) == -1)
//            errExit("msgsnd failed");

        //semOp(semid, attend_pid_client1, 1);

//    }else if(im_client2){
//        printf("ricevero i messaggi inciallah\n");
//
//    }

//==============fine tentativo fallito================================


















    //let's play
    char my_token;
    char my_name; //li mettero il suo nome scelto
    if(im_client2){
        my_token = playersInfo->token2;
    }else{
        my_token = playersInfo->token1;
    }
    printf("\n===========Cominciamo=============\n");

    for (int i = 0; i < 20; ++i) { //lo faccio girare solo per 5 volte, ma dovrebbe essere while(true)
        if(im_client2){
            printf("in quanto client 2, mi blocco\n");
                semOp(semid, just_play_client2, -1);//mi blocco affinche client 1 possa fare la sua mossa

        }
        //stampo il campo di gioco
        stampa_campo_da_gioco(gameBoard);
        printf("il tuo TOKEN: ~ %c ~ \n",my_token);
        printf("~ scegli una colonna  ~ \n");


        inserisci_token(gameBoard,my_token);

        //move
        printf("prima del blocco del server\n");
        //prima di continuare, comunico al server che ho fatto la mossa
        semOp(semid, sblocca_server, 1);//sbolocco il server, affinche possa fare i controlli
        semOp(semid, controlla_partita_server, -1);//mi blocco affinche il server possa controllare
        printf("DOPO IL  blocco del server\n");
        if(im_client2){
            printf("entro qui per sbloccare il client1\n");
            semOp(semid, just_play_client1, 1); //sblocco il client 1
        }else if(im_client1){
            printf("entro qui per sbloccare il client2, e bloccarmi in quanto client 1\n");
            semOp(semid, just_play_client2, 1); //sblocco client 2
            printf("ho gia sbloccato il client 2\n");
            semOp(semid, just_play_client1, -1);//blocco il client 1
        }


    }



    return 0;
}