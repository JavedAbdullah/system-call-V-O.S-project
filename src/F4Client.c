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
#include <string.h>
#include <time.h>
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
int rows, columns; // queste informazioni verranno prese dalla memoria condivisa

// the message queue identifier
int msqid = -1;

/*
    VARIABILI GLOBALI (quelle vere)
*/
int semid;
int shmidServer;
struct SharedData  *sharedData;
int shmidForPlayers;
struct PlayersInfo  *playersInfo;
int whoiam;
bool iambot = false;
int secondi_per_mossa = 20;//setto i secondi necessari affinche un client perdi


void stampa_campo_da_gioco(char (*gameBoard)[columns]);
/*
    SIG. HANDLERS
*/
void handlerSegnali(int sig){
    // Chiusura terminale
    if(sig == SIGHUP){
        printf("<client> hai abbandonato!\n sconfitta a tavolino!\n"); //no stampa

        if(whoiam == 1){
            kill(playersInfo->pid_server, SIGUSR1);
        }else if(whoiam == 2){
            kill(playersInfo->pid_server, SIGUSR2);
        }

        exit(0);
    }

    if(sig == SIGINT){
        printf("<client> hai abbandonato!\n sconfitta a tavolino!\n");

        if(whoiam == 1){
            kill(playersInfo->pid_server, SIGUSR1);
        }else if(whoiam == 2){
            kill(playersInfo->pid_server, SIGUSR2);
        }

        exit(0);
    }

    if(sig == SIGUSR2){
        if(!iambot){
            //mi riprendo il campo da gioco :)
            char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
            stampa_campo_da_gioco(gameBoard);
        }

        //faccio in controllo se sono il vincitore
        if(playersInfo->vincitore == 2 && playersInfo->abbandono == true){
            //client 2, vincitore
            printf("hai vinto per abbandono \n");
            exit(0);
        }
        if(playersInfo->vincitore == 2 ){
            //client 2, vincitore
            printf("hai vinto!! \n");
            exit(0);
        }

        if(playersInfo->vincitore == 0){
            printf("\nIL SERVER SUPREMO HA CHIUSO TUTTO!\n");
            exit(0);
        }
        if(playersInfo->vincitore == 1){
            //stampa_campo_da_gioco();
            printf("\nHAI PERSO!!\n");
            exit(0);
        }

        if(playersInfo->vincitore == 3){
            printf("\nPAREGGIO!!\n");
            exit(0);
        }
    }

    if(sig == SIGUSR1){
        if(!iambot){
            //mi riprendo il campo da gioco :)
        char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
        stampa_campo_da_gioco(gameBoard);
        }


        //faccio in controllo se sono il vincitore
        if(playersInfo->vincitore == 1 && playersInfo->abbandono == true){
            //client 2, vincitore
            printf("hai vinto per abbandono \n");
            exit(0);
        }

        if(playersInfo->vincitore == 1){
            //client 2, vincitore
            printf("hai vinto!!\n");
            exit(0);
        }
        if(playersInfo->vincitore == 0){
            printf("\nIL SERVER SUPREMO HA CHIUSO TUTTO!\n");
            exit(0);
        }

        if(playersInfo->vincitore == 2){
            printf("\nHAI PERSO!!\n");
            exit(0);
        }
        if(playersInfo->vincitore == 3){
            printf("\nPAREGGIO!!\n");
            exit(0);
        }
    }

    //il tempo per mossa e' scaduto;
    if(sig == SIGALRM){
        if(whoiam == 1){
            //sono il client 1
            kill(playersInfo->pid_client1, SIGINT);
        }else if(whoiam == 2){
            kill(playersInfo->pid_client2, SIGINT);
        }
    }

}



//stampa campo da gioco:
void stampa_campo_da_gioco(char (*gameBoard)[columns]){
    system("clear");
    printf("\n* * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("*             WELCOME TO                    *\n");
    printf("*             FORZA 4 GAME                   *\n");
    printf("*                                           *\n");
    printf("*             made by Javed A.              *\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * *\n");


    printf("~ sciegli la colonna: ~ \n");
    for (int i = 0; i < columns; ++i) {
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
    for (int i = 0; i < columns; ++i) {
        printf(" ~  ");
    }
    printf("\n");
    for (int i = 0; i < columns; ++i) {
        printf(" %i  ",i+1);
    }
    printf("\n");

}

void inserisci_token(char (*gameBoard)[columns], char token_inserito){
    int mossa_cliente;
    //printf("io dovrei essere una scanf\n");
    if(iambot == true){
        mossa_cliente = rand()%columns+1;
    }else{
        fflush(stdin);
        printf("inserisci => ");
        scanf("%d",&mossa_cliente);
    }

    while(mossa_cliente>columns){
        printf("guarda non ho cosi tante colonne per te\n inserisci uno colonna non piu grande di %i\n", columns);
        if(iambot == true){
            mossa_cliente = rand()%columns+1;
        }else{
            fflush(stdin);
            printf("inserisci => ");
            scanf("%d",&mossa_cliente);
        }
    }

    //TO-DO: devo fare il controllo che quello che inserisco sia giusto
    bool inserito = false;
    while(1){

        //alarm(secondi_per_mossa); //imposto l'allarme
        for (int i = rows-1; i>= 0; --i) {
            //printf("entro nel for:\n");
            if(gameBoard[i][mossa_cliente-1] == ' '){
                inserito = true;
                gameBoard[i][mossa_cliente-1] = token_inserito;
                stampa_campo_da_gioco(gameBoard);
                break;
            }
        }

        if(!inserito){
            fflush(stdout);//pulisco il buffer (?)
            fflush(stdin);
            printf("inserimento fallito, sciegliere un'altra colonna!\n");
            printf("\n hai %i sec. per la mossa, altrimenti perdi per abbandono\n", secondi_per_mossa);
            alarm(0);//risetto l'allarme
            alarm(secondi_per_mossa);//risetto il timer
            fflush(stdout);//pulisco il buffer (?)
            fflush(stdin);

            if(iambot == true){
                mossa_cliente = rand()%columns+1;
            }else{
                scanf("%d",&mossa_cliente);
                while(mossa_cliente>columns){
                    printf("guarda non ho cosi tante colonne per te\n inserisci uno colonna non piu grande di %i\n", columns);
                    printf("inserisci => ");
                    scanf("%d",&mossa_cliente);

                }
            }



           // scanf("%d",&mossa_cliente);
            //TO-DO: devo fare il controllo che quello che inserisco sia giusto
        }else{
            //alarm(0);//risetto l'allarme
            playersInfo->colonna_ultima_mossa = mossa_cliente;
            break;
        }
    }

}







int create_sem_set(key_t semkey) {
    // Create a semaphore set with ? semaphores
     semid = semget(semkey, 9, IPC_CREAT | S_IRUSR | S_IWUSR);
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

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if(argc <2 || argc > 3){
        printf("Usage: %s \n", argv[0]);
        printf("eseguire il programma in questo modo: ./<eseguibile> <nome giocatore>\n oppure per giocare con un bot: ./<eseguibile> <nome giocatore> bot\n");
        return 1;
    }
    system("clear");
    printf("\n* * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("*             WELCOME TO                    *\n");
    printf("*             FORZA 4 GAME                   *\n");
    printf("*                                           *\n");
    printf("*             made by Javed A.              *\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * *\n");

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
     shmidServer = alloc_shared_memory(shmKey, sizeof(rows*columns));

    // attach the shared memory segment
    printf("<client> attaching the shared memory segment...\n");
    sharedData  = (struct SharedData *)get_shared_memory(shmidServer, 0);

    // create a semaphore set
    printf("<client> creating a semaphore set...\n");
     semid = create_sem_set(semkey);


    rows = sharedData->rows;
    columns = sharedData->columns;

    char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
    //per accedere:  gameBoard[i][j]

    //printf("column: %i \n rows: %i \n", columns,rows);

    //struct playersInfo (accedo allo spazio allocato dal server)
    key_t semkeyPlayers = ftok(".", 'c');
    if (semkeyPlayers == -1)
        errExit("ftok failed");
    shmidForPlayers = alloc_shared_memory(semkeyPlayers , sizeof(struct PlayersInfo));
    playersInfo = (struct PlayersInfo*)get_shared_memory(shmidForPlayers, 0);

    if(argc == 3){
        if(!strcmp("bot", argv[2])){
            printf("client vuole giocare con il bot... \n caricamento del bot in corso...\n");
            playersInfo->bot = true;

        }else{
            printf("Usage: %s \n", argv[0]);
            printf("eseguire il programma in questo modo: ./<eseguibile> <nome giocatore>\n oppure per giocare con un bot: ./<eseguibile> <nome giocatore> bot\n");
            return 1;
        }

    }


    bool im_client1 = false;
    bool im_client2 = false; //mi serviranni per determinare che giocatore sono

    if(playersInfo->player_counter == 1){
        im_client2 = true;
    }else if(playersInfo->player_counter == 0){
        im_client1 =  true;
    }
    playersInfo->player_counter++;
    //printf("playersInfo->player_counter: %i\n", playersInfo->player_counter);
    if(im_client1){
        whoiam = 1;
        strcpy(playersInfo->client1, argv[1]);
        playersInfo->pid_client1 = getpid();
       // printf("in quanto client 1, ho pid %i\n",  getpid());
        //saro dentro client 1, faccio andare avanti il server
        //printf("mi collego da client 1\n");
        semOp(semid, attendi_client1, 1);
        //printf("attendo che il server mi dica che e' arrivato il client 2\n");
        printf("ASPETTIAMO che si colleghi il secondo giocatore...\n");
        semOp(semid, attendi_client1, -1);
    }

    if(im_client2){
        if(playersInfo->bot == true){
            iambot = true;
            //setbuf(stdout, NULL);//tolgo i printf del bot (?)
        }
        whoiam = 2;
        strcpy(playersInfo->client2, argv[1]);
        playersInfo->pid_client2 = getpid();
       // printf("in quanto client 2, ho pid %i\n",  getpid());
        //saro dentro client 2, faccio andare avabti il server
        //printf("mi collego da client 2\n");
        semOp(semid, attendi_client2, 1);
        //semOp(semid, attendi_client2, -1);//mi blocco in attesa di ordini
    }

   // printf("server, ha pid %i\n", playersInfo->pid_server);
    //printf("client 1, ha pid %i\n",  playersInfo->pid_client1);
    //printf("client 2, ha pid %i\n",  playersInfo->pid_client2);




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

    rows = sharedData->rows;
    columns = sharedData->columns;

    char my_token;
    //char my_name; //li mettero il suo nome scelto
    if(im_client2){
        my_token = playersInfo->token2;
    }else{
        my_token = playersInfo->token1;
    }
    printf("\n===========Cominciamo=============\n");

    while(1){ //lo faccio girare solo per 20 volte, ma dovrebbe essere while(true)
        if(im_client2){
            //printf("in quanto client 2, mi blocco\n");
            printf("ATTENDI! sta facendo la mossa il tuo avversario, %s ....\n", playersInfo->client1);
                semOp(semid, just_play_client2, -1);//mi blocco affinche client 1 possa fare la sua mossa

        }
        //stampo il campo di gioco
        stampa_campo_da_gioco(gameBoard);
        printf("il tuo TOKEN: ~ %c ~ \n",my_token);
        printf("~ scegli una colonna  ~ \n");

        //move
        fflush(stdout);//pulisco il buffer (?)
        fflush(stdin);

        printf("\n hai %i sec. per la mossa, altrimenti perdi per abbandono\n", secondi_per_mossa);
        alarm(secondi_per_mossa);
        inserisci_token(gameBoard,my_token);
        alarm(0);//risetto l'allarme

        //printf("prima del blocco del server\n");
        //prima di continuare, comunico al server che ho fatto la mossa
        semOp(semid, sblocca_server, 1);//sbolocco il server, affinche possa fare i controlli
        semOp(semid, controlla_partita_server, -1);//mi blocco affinche il server possa controllare
        //printf("DOPO IL  blocco del server\n");
        if(im_client2){
            //printf("entro qui per sbloccare il client1\n");
            semOp(semid, just_play_client1, 1); //sblocco il client 1
        }else if(im_client1){
            //printf("entro qui per sbloccare il client2, e bloccarmi in quanto client 1\n");
            printf("ATTENDI! sta facendo la mossa il tuo avversario, %s ....\n", playersInfo->client2);
            semOp(semid, just_play_client2, 1); //sblocco client 2
            //printf("ho gia sbloccato il client 2\n");
            semOp(semid, just_play_client1, -1);//blocco il client 1
        }


    }



    return 0;
}