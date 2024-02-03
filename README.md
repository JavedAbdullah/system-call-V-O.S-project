# system-call-V-O.S-project
# Elaborato S.O

prima di inziare:
```bash
 make clean 
 make
```

### utilizzo del programma:

### per avviare il server: 
```bash
./F4Server <Nrgihe> <Ncolonne> <gc1> <gc2>

<Nrgihe> -> numero righe
<Ncolonne>  -> numero colonne 
<gc1> -> gettone client 1
<gc2> -> gettone client 2

```

esempio:
```bash
 ./F4Server 7 6 x o

output:


* * * * * * * * * * * * * * * * * * * * * * *
*             WELCOME TO                    *
*             FORZA 4 GAME                  *
*                                           *
*             made by Javed A.              *
* * * * * * * * * * * * * * * * * * * * * * *
<Server> allocating a shared memory segment...
<Server> attaching the shared memory segment...
<Server> creating a semaphore set...
e' tutto pronto, attendo i client...

```

### utilizzo client:

```bash
per giocare con un'altro gicatore:
./F4Client <nome>

esempio:
```bash
./F4Client abdu

* * * * * * * * * * * * * * * * * * * * * * *
*             WELCOME TO                    *
*             FORZA 4 GAME                  *
*                                           *
*             made by Javed A.              *
* * * * * * * * * * * * * * * * * * * * * * *
<client> allocating a shared memory segment...
<client> attaching the shared memory segment...
<client> creating a semaphore set...
ASPETTIAMO che si colleghi il secondo giocatore...
```



```
avviare un'altro terminale:
```bash
./F4Client <nome2>
```
esempio:
```bash
./F4Client abdu

* * * * * * * * * * * * * * * * * * * * * * *
*             WELCOME TO                    *
*             FORZA 4 GAME                  *
*                                           *
*             made by Javed A.              *
* * * * * * * * * * * * * * * * * * * * * * *
<client> allocating a shared memory segment...
<client> attaching the shared memory segment...
<client> creating a semaphore set...

===========Cominciamo=============
ATTENDI! sta facendo la mossa il tuo avversario, abdu ....



```

```bash
per giocare con il bot:
./F4Client <nome> bot

nota: scrivere esattamente bot

esempio:
./F4Client abdu bot

 ~   ~   ~   ~   ~   ~  
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
[ ] [ ] [ ] [ ] [ ] [ ] 
 ~   ~   ~   ~   ~   ~  
 1   2   3   4   5   6  
il tuo TOKEN: ~ x ~ 
~ scegli una colonna  ~ 

 hai 20 sec. per la mossa, altrimenti perdi per abbandono
inserisci => 

```

### logica del programma ~generale~:

```txt
in pratica e' stato creato una struct nella
memoria condivisa, nella quale tramite un 
contatore si riesce a capire se sto eseguendo
il codice con il client 1 oppure client 2.

grazie a questo e' stato possibile anche la
sincronizzazione dei semafori e segnali.
```

### logica semafori:

```txt
parte il server che si blocca, 
aspettando che arrivi un client,
appena arriva un client (che rinomino in c1) 
comunica il server il suo arrivo.
il server si blocca di nuovo aspettando 
che si colleghi il client 2 (che rinomino in c2)

ora server sblocca c1 e c2 e si blocca
c2 si blocca e avanza solo c1.
e il gioco inzia, c1 fa la mossa e si blocca
sbloccando c2 e il server (che controlla che
se qualcuno ha vinto, in caso di vittoria di
c1, c2  oppure pareggio manda un segnale 
a c1 e c2, modificando anche una variabile 
condivisa 
dove viene indiacato il vincitore
che fa la mossa e sblocca c1
tutto questo dentro un ciclo infinto
```
### logica segnali

```txt
appena il server iniza e arrivano c1 e c2
salvo nella memoria condivisa i pid dei
vari attori in gioco, nelle rispettive
variabili (pid_server, pid_client1, pid_client2)

segnali per il server:
- e' stato preveisto il CTRL+C, che dopo
essere stato premuto per 2 volte di seguito
entro 5 sec. termina il gioco, comunicando
la chiusura a c1 e c2 (che a loro volta terminano)

segnali per i client:
e' stato previsto la chiusra immediata dopo
aver premuto CRL+C, condirando come aver abbandonato la partita con la retiva sconfitta
e tutto cio' viene comunicato al server
che comunica anche al altro client la vittoria
e poi chiude tutto

per i client e' stato previsto anche un tempo 
per mossa (per ora 20 sec.) superati i quali si
perde per abbandono della partita 

sia per client e server e' stato previsto la
chiusra del terminale, in caso del server chiude anche c1 e c2 dicendo che il server supremo ha 
chiuso tutto, per i client e' considerato come un
abbandono della partita.
```

### logica del bot

```c
e' il c1 che si collega che indica se vuole
giocare con il bot, mettendo a true una variabile
condivisa, poi il server fa una fork() 
e nel figlio faccio un execl() eseguendo
il codice del client, quindi giocando come se
fosse c2, ma poiche' la variabile iambot=true
invece di chiedere in input la colonna dove 
inserire il gettone, scelgo una una colonna a 
caso provo a inserire finche non riesco, per il
resto il comportamento e' uguale a c2
```

