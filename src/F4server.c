/************************************
*VR471543
*Javed Abdullah
*2/01/2024
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include "string.h"
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


/*
    VARIABILI GLOBALI
*/
//rendo variabili globali per necessita nella fase implementativa
int rows, columns;
char token_client1 = ' ', token_client2 = ' ';
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

int contatoreCtrlC = 0; //contatore Ctl+c
int secondiCtrlC = 5;



/*funziona che fa puliza*/

void elimina_tutto(){
    //elimino i semafori creati
    printf("<Server> removing semafori...\n");
    elimina_semafori(semid);

    // remove the shared memory segment
    printf("<Server> removing & detaching the shared memory segment...\n");
    rimuovi_e_libera_spazio(sharedData, shmidServer);
    //rimuovo lo spazio anche per playersInfo
    rimuovi_e_libera_spazio(playersInfo,shmidForPlayers);
}


/*
    SIG. HANDLERS
*/
void handlerSegnali(int sig){

    // Chiusura terminale
    if(sig == SIGHUP){
        printf("<server> terminale server chiuso\n"); //capo questo non verra stampato
        //comunico ai client che chiudo tutto
        playersInfo->vincitore = 0; // indica che non ce' un vincitore, e' il server a chiudere tutto
        kill(playersInfo->pid_client1, SIGUSR1);
        kill(playersInfo->pid_client2, SIGUSR2);

        elimina_tutto();
        exit(0);
    }


   else if(sig == SIGINT){
        //printf("entror nel sigINIT\n");



        contatoreCtrlC++;

        if(contatoreCtrlC == 2){
            //printf("entro nel contatore\n");
            alarm(0);
            printf("<server> Gioco interrotto dal CTRL+C\n");
            //comunico ai client che chiudo tutto
            playersInfo->vincitore = 0; // indica che non ce' un vincitore, e' il server a chiudere tutto
            kill(playersInfo->pid_client1, SIGUSR1);
            kill(playersInfo->pid_client2, SIGUSR2);

            elimina_tutto();
            exit(0);
        }
        printf("\n premere ancora CTRL+C entro %i sec. per interrompere il gioco!\n", secondiCtrlC);
        alarm(secondiCtrlC);
    }
    else if(sig == SIGALRM){
        printf("entro qui SIGALRM\n");
        alarm(0);
        if(contatoreCtrlC != 0){
            contatoreCtrlC = 0;
            secondiCtrlC = 5;
            printf("<server> Resettato contatore CTRL+C\n");
        }

    }


   else  if(sig == SIGUSR1){
        printf("<server> oh no, client 1: %s abbandona \n", playersInfo->client1);
        playersInfo->abbandono = true;
        kill(playersInfo->pid_client2, SIGUSR2);
        playersInfo->vincitore = 2;
        elimina_tutto();
        exit(0);
    }
    else if(sig == SIGUSR2){
        printf("<server> oh no, client 2: %s abbandona \n", playersInfo->client2);
        playersInfo->abbandono = true;
        kill(playersInfo->pid_client1, SIGUSR1);
        playersInfo->vincitore = 1;
        elimina_tutto();
        exit(0);
    }


}




int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(semkey, 9, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
       errExit("semget failed");

    // Initialize the semaphore set
    union semun arg;
    unsigned short values[] = {0, 0, 0, 0, 0, 0, 0, 0,0};
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



    //cerco F4 nella colonna
    int F4_ct_client1 = 0; //contatore per client1
    int F4_ct_client2 = 0; //contatore per client2
    for (int i = rows-1; i>= 0; --i) {
       // printf("sto controllando la poszione %i nella colonna %i \n", i, colonna_ultima_mossa-1);
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
    //printf("controllo sulle righe: \n");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            printf("%c ",gameBoard[i][j]);
            if(gameBoard[i][j] == token_client1){
                F4_ct_client1++;
                F4_ct_client2 = 0;
            }else if(gameBoard[i][j] == token_client2){
                F4_ct_client1 = 0;
                F4_ct_client2++;
            }else{
                F4_ct_client1 = 0; //contatore per client1
                F4_ct_client2 = 0; //contatore per client2
            }
            //controllo preventivo  x x x x o (per far si che funzioni in questo caso)
            if(F4_ct_client1>=4){
                return 1;
            }else if(F4_ct_client2>=4){
                return 2;
            }


        }
        printf("\n");

        //forse questo controllo e' inutile
        if(F4_ct_client1>=4){
            return 1;
        }else if(F4_ct_client2>=4){
            return 2;
        } else{
            F4_ct_client1 = 0;
            F4_ct_client2 = 0;
        }

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

    //controllo in diagonale ma cosi / :

    //cercor di capire in che riga si e' fermato il gettone

    int row_where_insert = -1;
    for (int i = rows-1; i >= 0; i--) {
        if(gameBoard[i][colonna_ultima_mossa-1] == ' '){
            //row_where_insert = i - 1;
            row_where_insert = i+1; //(perche come faccio la ricerca nel gameBoard, e siccome trovo vuoto, allora vado al ultimo inserimento con +1
            break;
        }
    }

    /*
     * se e' ancora 0, allora la colonna e' stata riempita con l'ultimo inserimento
     * */
    if(row_where_insert == -1){
        row_where_insert = 0;
    }
    int original_row_where_insert = row_where_insert;
   // printf("row where insert: %i\n", row_where_insert);

    int column_where_insert = colonna_ultima_mossa-1;

    for (int i = 0; i < columns; ++i) {
        if(column_where_insert== 0 || row_where_insert == rows-1){
            break;
        }
        row_where_insert++;
        column_where_insert--;
    }
    //printf("partiro dalla richerca riga: %i, colonna: %i\n", row_where_insert,column_where_insert);

    F4_ct_client1 = 0;
    F4_ct_client2 = 0;
    for (int i = 0; i < columns; ++i) {
        if(gameBoard[row_where_insert][column_where_insert] == token_client1){
            F4_ct_client1++;
            F4_ct_client2 = 0;
        }else if(gameBoard[row_where_insert][column_where_insert] == token_client2){
            F4_ct_client2++;
            F4_ct_client1 = 0;
        }else{
            F4_ct_client1 = 0; //contatore per client1
            F4_ct_client2 = 0; //contatore per client2
        }

        if(F4_ct_client1>=4){
            return 1;
        }else if(F4_ct_client2>=4){
            return 2;
        }
        row_where_insert--;
        column_where_insert++;
    }

    //controllo diagonale ma cosi \

    //rinizzializzo
    row_where_insert =  original_row_where_insert;
    column_where_insert = colonna_ultima_mossa-1;

    for (int i = 0; i < columns; ++i) {
        if(column_where_insert== columns-1 || row_where_insert == rows-1){
            break;
        }
        row_where_insert++;
        column_where_insert++;
    }


    F4_ct_client1 = 0;
    F4_ct_client2 = 0;
    for (int i = 0; i < columns; ++i) {
        if(gameBoard[row_where_insert][column_where_insert] == token_client1){
            F4_ct_client1++;
            F4_ct_client2 = 0;
        }else if(gameBoard[row_where_insert][column_where_insert] == token_client2){
            F4_ct_client2++;
            F4_ct_client1 = 0;
        }else{
            F4_ct_client1 = 0; //contatore per client1
            F4_ct_client2 = 0; //contatore per client2
        }

        if(F4_ct_client1>=4){
            return 1;
        }else if(F4_ct_client2>=4){
            return 2;
        }
        row_where_insert--;
        column_where_insert--;
    }





    //printf for debugging parpouse
    //printf("riga ultima mossa: %i\n colonna ultima mossa: %i\n",riga_ultima_mossa,colonna_ultima_mossa);

    return -1;
}

int main(int argc, char *argv[]) {

    if(argc != 5){
        printf("Usage: %s \n", argv[0]);
        printf("eseguire il programma in questo modo:\n ./<eseguibile> <righe> <colonne> <gettone 1> <gettone 2>\n");
        return 1;
    }
    rows = atoi(argv[1]);
    columns = atoi(argv[2]);

    if(rows< 5 || columns< 5){
        printf("le righe e colonne DEVONO essere >=5\n");
        return 1;
    }

    /*
     *
     * stampo benvenuto al gioco
     *
     * */

    /* * * * * * * * * * * * * * * * * * * * * */
    /*             WELCOME TO                  *
     *             FORZA4 GAME                 *
     *                                         *
     *                                         *
     *            made by Javed A.             *
     *  * * * * * * * * * * * * * * * * * * * */

    system("clear");
    printf("\n* * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("*             WELCOME TO                    *\n");
    printf("*             FORZA 4 GAME                   *\n");
    printf("*                                           *\n");
    printf("*             made by Javed A.              *\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * *\n");



   // strcpy(token_client1, argv[3]);
    token_client1 = argv[3][0];
    //printf("token 1 %c: \n", token_client1);
   token_client2 = argv[4][0];


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
    shmidServer = alloc_shared_memory(shmKey, sizeof(rows*columns)+sizeof (char));
    // attach the shared memory segment
    printf("<Server> attaching the shared memory segment...\n");
     sharedData  = (struct SharedData *)get_shared_memory(shmidServer, 0);
    sharedData->rows = rows;
    sharedData->columns = columns;
    //creo il campo di gioco
    //rows = sharedData->rows;
    //columns = sharedData->columns;
    char (*gameBoard)[columns] = (char (*)[columns])sharedData->gameBoard;
    //per accedere:  gameBoard[i][j]
    //printf("column: %i \n rows: %i \n", columns,rows);

    // create a semaphore set
    printf("<Server> creating a semaphore set...\n");
    semid = create_sem_set(semkey);


    //struct playersInfo <attendiamo il palyer_counter venga modificato>

    //struct playersInfo (creo uno spazio di memoria per struct PlayersInfo
    key_t semkeyPlayers = ftok(".", 'c');
    if (semkeyPlayers == -1)
        errExit("ftok failed");

   // printf("semekeyplayers: %i\n", semkeyPlayers);
     shmidForPlayers = alloc_shared_memory(semkeyPlayers , sizeof(struct PlayersInfo));
   playersInfo = (struct PlayersInfo*)get_shared_memory(shmidForPlayers, 0);

    //riempio le informazioni giusto per testare
    playersInfo->player_counter = 0;
    //strcpy(playersInfo->client1, "Monty");
    //strcpy(playersInfo->client2, "creatura magica");
    playersInfo->token1 = token_client1;
    playersInfo->token2 = token_client2;
   // playersInfo->colonna_ultima_mossa = 1;
    playersInfo->pid_server = getpid();//metto il pid del server;

    //metto spazi vuoti nella matrice
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
                gameBoard[i][j] = ' ';
        }
    }
//    for (int i = 0; i < rows; ++i) {
//        for (int j = 0; j < columns; ++j) {
//            printf("%c", gameBoard[i][j]);
//        }
//        printf("\n");
//    }

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

   /*===================fine gestione segnali==========================*/





   // printf("Sending the message for client 2 ...\n");
    //mSize = sizeof(struct IvariPid) - sizeof(long);


//    if (msgsnd(msqid, &pid_di_tutti, mSize, 0) == -1)
//        errExit("msgsnd failed");

    /* cominciamo il game */
    //provo a muovermi nella sincronizzazione per 5 volte
    printf("e' tutto pronto, attendo i client...\n");
    semOp(semid, attendi_client1, -1);
    printf("e' arrivato il client 1 : cha ha nome %s\n",playersInfo->client1);
    //manda un segnale al client 1
    if(playersInfo->bot == true){
        //il client 1 vuole giocare con il bot, che coraggioso!
        pid_t pid = fork();

        if (pid == -1){
            printf("bot  not created!");
            elimina_tutto();
            return 1;
        }else if(pid == 0){
            /* sono nel figlio
             * faccio un exec del client e termino
             *
             * */

            execl("F4Client","F4Client", "BOT", (char *) NULL);
            printf("execl ha fallito\n");
            elimina_tutto();
            return 1;

        }


    }

    semOp(semid, attendi_client2, -1);
    printf("e' arrivato il client 2 : cha ha nome %s\n",playersInfo->client2);
    semOp(semid, attendi_client1, 1);//sblocco il client 1

    //prima di cominciare i vari giochi mi salvo i vari pid dei client



    //comincia la creazione di una coda di messaggi
//    int msgKey= ftok(".", 'q'); //(q - for queue)
//    if ( msgKey == -1)
//        errExit(" msgKey ftok failed");
//
//    printf("<Server> Making MSG queue...\n");
//    // get the message queue, or create a new one if it does not exist
//    msqid = msgget(  msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
//    if (msqid == -1)
//        errExit("msgget failed");
//
//    //create a struct del messaggio da inviare
//    struct IvariPid pid_di_tutti;
//
//
//    pid_di_tutti.mtype = 1;
//    pid_di_tutti.pid_server = getpid();
//    printf("Sending the message for client 1...\n");
//    size_t mSize = sizeof(struct IvariPid) - sizeof(long);
//    if (msgsnd(msqid, &pid_di_tutti, mSize, 0) == -1)
//        errExit("msgsnd failed");
   // semOp(semid, attend_pid_server, 1);
   // semOp(semid, attend_pid_client1, -1);

//    if (msgrcv(msqid, &pid_di_tutti, mSize, 0, 0) == -1)
//        errExit("msgget failed");

//    printf("in quanto server ho pid: %i, salvato nella struct ho %i\n", getpid(), pid_di_tutti.pid_server);
//    printf("il pid del client1 e' %i", pid_di_tutti.pid_client1);
//








    //stampo i vari pid per vedere le corrispondeze



   printf("in quanto server ho pid: %i, salvato nella struct ho %i\n", getpid(), playersInfo->pid_server);
    printf("server, ha pid %i\n", playersInfo->pid_server);
    printf("client 1, ha pid %i\n",  playersInfo->pid_client1);
    printf("client 2, ha pid %i\n",  playersInfo->pid_client2);

    int pid_client1 =  playersInfo->pid_client1;
    int pid_client2 = playersInfo->pid_client2;









    printf("\n===========COMINCIAMO!===========\n");

    while(1){

        printf("mi blocco, affiche un client giochi\n");
        semOp(semid, sblocca_server, -1); //blocco il server, attendo che un client giochi
        //conterra -1(nessuno ha vinto), 1(vince client1), 2(vince client2), 3(vince client3)
        int valore_vincitore =  controlla_se_qualcuno_ha_vinto(gameBoard, playersInfo->colonna_ultima_mossa);
        if(valore_vincitore == 1){
            printf("vince client 1: %s, bravo!!\n", playersInfo->client1);
            //chiudo i client
            //kill(pid_client1, SIGINT);
            //kill(pid_client2, SIGINT);
            playersInfo->vincitore = 1;
            kill(pid_client1, SIGUSR1);
            kill(pid_client2, SIGUSR2);
            break;

        }else if(valore_vincitore == 2){
            printf("vince client 2: %s, bravo!!\n", playersInfo->client2);
            //chiudo i client
            //kill(pid_client1, SIGINT);
            //kill(pid_client2, SIGINT);
            playersInfo->vincitore = 2;
            kill(pid_client1, SIGUSR1);
            kill(pid_client2, SIGUSR2);
            break;
        }else if(valore_vincitore == 3){
            printf("PAREGGIO, BRAVI ENTRAMBI\n");
            //CHIUDO IL GIOCO
            //kill(pid_client1, SIGINT);
            //kill(pid_client2, SIGINT);
            playersInfo->vincitore = 3;
            kill(pid_client1, SIGUSR1);
            kill(pid_client2, SIGUSR2);
            break;
        }else if(valore_vincitore == -1){
            printf("nessuno ha vinto, CONTINUATE!\n");
            //CONTINUO IL GIOCO
        }


        printf("dico al client ho finito il controllo\n");
        semOp(semid, controlla_partita_server, 1);//sblocco i client che ha fatto l'utima mossa
        printf("dopo la conferma al client\n");
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
//   int valore_vincitore =  controlla_se_qualcuno_ha_vinto(gameBoard, playersInfo->colonna_ultima_mossa);
//   if(valore_vincitore == 1){
//       printf("vince client 1, bravo!!\n");
//       //chiudo i client
//   }else if(valore_vincitore == 2){
//       printf("vince client 2, bravo!!\n");
//       //chiudo i client
//   }else if(valore_vincitore == 3){
//       printf("PAREGGIO, BRAVI ENTRAMBI\n");
//       //CHIUDO IL GIOCO
//   }else if(valore_vincitore == -1){
//       printf("nessuno ha vinto, CONTINUATE!\n");
//       //CONTINUO IL GIOCO
//   }


//    semOp(semid, attendi_client1, -1);
//    printf("arrivato 1 client, evviva!!\n");
//    printf("player info counter: %d\n", playersInfo->player_counter);
//    semOp(semid, attendi_client2, -1);
//    printf("arrivati 2 client, evviva!!\n");
//    printf("player info counter: %d\n", playersInfo->player_counter);


/*
 *
 * * * * * * * * * * faccio puliza * * * * * * * * * * *
 *
 * */
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