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


per giocare con un'altro gicatore:
```bash
./F4Client <nome>
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
ASPETTIAMO che si colleghi il secondo giocatore...
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

### Logica Generale del Programma::

```txt
Il programma utilizza una struct nella memoria condivisa per identificare quale client (client 1 o client 2) sta eseguendo il codice. Questo permette la sincronizzazione dei semafori e dei segnali.
```

### logica semafori:

```txt
Il server si blocca inizialmente, aspettando l'arrivo del primo client (rinominato c1). Quando c1 si connette, il server è notificato e attende il secondo client (rinominato c2). Dopo la connessione di c2, il server sblocca c1 e c2, inizia il gioco, e il flusso di controllo passa tra i due client. Il server controlla la vittoria e invia un segnale ai client per notificarli. Il ciclo di gioco continua indefinitamente.

```
### logica segnali

```txt
Per il server, è prevista la gestione del segnale CTRL+C, che, se premuto due volte entro 5 secondi, termina il gioco notificando la chiusura ai client (c1 e c2), che a loro volta terminano.

Per i client, la pressione di CTRL+C equivale a un abbandono della partita e viene comunicato al server. È inoltre previsto un limite di tempo per ogni mossa (20 secondi), e se superato, il giocatore perde per abbandono della partita. La chiusura del terminale da parte del server comporta la chiusura dei client, comunicando un il server supremo ha chiuso la partita.
```

### logica del bot

```c
Quando il client 1 (c1) si collega e indica di voler giocare con il bot, viene impostata una variabile condivisa "iambot" a true. Successivamente, il server crea un processo figlio tramite fork() e nel figlio esegue execl() con il codice del client. Il bot, identificandosi come client 2 (c2), inserisce il gettone in una colonna casuale finché riesce. Il comportamento del bot è simile a c2, ma con laggiunta della scelta casuale della colonna in cui inserire il gettone.
```

