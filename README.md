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
*             FORZA 4 GAME                   *
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
*             FORZA 4 GAME                   *
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
*             FORZA 4 GAME                   *
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
