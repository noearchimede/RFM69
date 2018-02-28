Driver Arduino per i moduli radio RFM69
=======================================
_Arduino RFM69 radio module driver_

_**Note** This library is written and commented in Italian. If you'd like
to use it but don't understand that language please [contact me](mailto:noearchimede@gmail.com),
I'll provide a tanslated version of my code with class member names and documentation
in English._

---

**Autore**   Noè Archimede Pezzoli<br>
**Data**  Febbraio 2018<br>

**Contatto**  <noearchimede@gmail.com> <br>

---

### Introduzione ###

La libreria RFM69 permette di collegare due microcontrollori tramite una coppia
di moduli radio della famiglia RFM69 di HopeRF, in particolare del modello
[RFM69HCW](http://www.hoperf.com/rf_transceiver/modules/RFM69HCW.html).

Il codice della libreria è ampiamente commentato in italiano. I commenti normali
(`// ...`), presenti soprattutto nei files di implementazione (`.cpp`), forniscono
dettagli sull'implementazione. I commenti "speciali" (`//! ...` o `/*! ... */`)
sono concentrati nel file header e costituiscono una documentazione per l'utilizzatore
della libreria che non desidera conoscere i dettagli del suo funzionamento.
Questa documentazione può essere riunita da Doxygen in un unico file o pagina html;
la versione html più recente è consultabile
[qui](http://htmlpreview.github.io/?https://rawgit.com/noearchimede/RFM69/master/Doc/html/index.html).

---

### Indice ###

[1. Caratteristiche del modulo radio](#1)<br>
[2. Protocollo di comunicazione](#2)<br>
[3. Collisioni](#3)<br>
[4. Hardware](#4)<br>
[5. Struttura dei messaggi](#5)<br>
[6. Impostazioni](#6)<br>
[7. Esempio di utilizzo](#7)<br>

[Documentazione](http://htmlpreview.github.io/?https://rawgit.com/noearchimede/RFM69/master/Doc/html/index.html)

---
<br><div id='id-section1'/>

## 1. Caratteristiche del modulo radio ##

Caratteristiche principali dei moduli radio RFM69HCW:

- frequenza: 315, 433, 868 oppure 915 MHz (esistono quattro versioni per adattarsi
alle bande utilizzabili senza licenza in diversi paesi)
- potenza di emissione: da -18dBm a +20dBm (100mW)
- sensdibilità: fino a -120dBm (con bassa bitrate)
- bitrate fino a 300'000 Baud
- modulazioni: FSK, GFSK, MSK, GMSK, OOK


I messaggi possono includere un controllo CRC16 di due bytes che riduce drasticamente
la probabilità di errore durante la trasmissione. Possono inoltre essere criptati
secondo l'algoritmo Avanced Encryption Standard AES-128 con una chiave di 16 bytes
per impedirne la lettura da parte di eventuali terze radio. La possibilità offerta
dal modulo di assegnare ad ogni modulo un indirizzo unico in modo da creare una rete
con fino a 255 dispositivi nonb è sfruttata, ma un risultato simile è ottenibile
creando una rete in cui ogni radio ha una sync word unica che può sostituire
temporaneamente con quella di un'altra radio (ottenuta da una tabella pubblica)
per inviare un messaggio a quella radio. Questo permette di creare una rete di
dimensione arbitraria (è possibile impostare fino a 8 byte di sync word, per un
totale di 2^64 indirizzi possibili) ma non di inviare messaggi broadcast, come
invece il sistema di addressing incluso nel modulo permetterebbe.

<br>
Corrente di alimentazione richiesta (a 3.3V), per modalità:

- Sleep:      0.0001 mA
- Standby:    1.25 mA
- Rx:         16 mA
- Tx:         16 - 130 mA a seconda della potenza di trasmissione


<br><div id='2'/>

## 2. Protocollo di comunicazione ##

Il protocollo di comunicazione alla base di questa classe presuppone che in una
stessa banda di frequenza siano presenti esattamente due radio che condividono
la stessa sync word. La stessa frequenza può quindi essere utilizzata anche da
altri dispositivi; naturalmente, però, se dispositivi trasmittenti sulla stessa
frequenza trasmettono dati nell stesso momento nessuno di essi riceverà un mesaggio
valido (a meno che la differenza nella potenza trasmessa sia abbastanza grande da
permettere al segnale più forte di "coprire" il più debole, in tal caso solo il
dispositivo ricevente il più forte otterrà un messaggio).

Alla lettura di ogni messaggio la radio ricevente può trasmettere automaticamente
un segnale di ACK se la radio trasmittente lo ha richiesto. In questo modo se
l'utente deve essere certo che un messaggio trasmesso sia stato ricevuto e letto
(quindi certamente anche utilizzato, visto che la lettura avviene solo su richiesta
dell'utente e non automaticamente come la ricezione) non deve né implementare un
sistema di ACK né modificare il codice ricevente, e il segnale di ACK sarà il
meno dispendioso possibile in termini di tempo del programma.

Gli schemi sottostanti illustrano la trasmissione di un mesasggio.

1 - con ACK:

	mod       ?   |    tx    |            rx             |        def
	fz          INVIA       ISR                         ISR
	A       ------|----------|---------------------------|-------------------->
	RF             |||MESS|||                    ^^^^^^^
	|              vvvvvvvv                      ||ACK||
	B       ------------------|-----------------|-------|--------------------->
	fz                       ISR              LEGGI    ISR
	mod             rx        |       stby      |  tx   |          def

2 - senza ACK:

	mod       ?   |    tx    |              def
	fz          INVIA       ISR
	A       ------|----------|--------------------------------->
	RF             |||MESS|||
	|              vvvvvvvvv
	B       ------------------|-----------------|--------------->
	fz                       ISR              LEGGI
	mod             rx        |       stby      |       def

- `A, B`: Programma delle stazioni radio, evoluzione nel tempo
- `fz`: funzioni chiamate. `invia()` e `leggi()` sono chiamate dall'utente, `isr()`
    è l'interrupt service routine della classe
- `mod`: modalità della radio. `tx` = trasmissione, `rx` = ricezione, `stby` = standby,
    `def`: la modalità che l'utente ha scelto come default per quella radio
- `RF`: presenza di segnali radio e loro direzione

<br><div id='3'/>

## 3. Collisioni ##

Le funzioni di questa classe non impediscono che le due radio trasmettano dei
messaggi contemporaneamente. Questo problema deve essere gestito come possibile
dal codice dell'utente. Tuttavia le funzioni della classe in caso di conflitto
impediscono la perdita di entrambi i messaggi (cosa che potrebbe portare a un
blocco senza uscita se entrambi i programmi cercassero di reinviare subito il
proprio messaggio). Dà quindi la priorità ai messaggi già arrivati a scapito di
quelli in uscita, che potrebbero perdersi.

Lo schema sottostante illusta la trasmissione di un mesaggio evidenziando i momenti
in cui non si può o non si dovrebbe trasmetterne altri.

1 - con ACK:

	stato tx                    |*********|############ 1 ############|
	A           ----------------|---------|---------------------------|---------->
	|                         INVIA      ISR                         ISR
	|                                    ISR              LEGGI    ISR
	B           ---------------------------|-----------------|-------|----------->
	stato tx            |####### 2 ########|                 |*******|

2 - Senza ACK:

	stato tx                    |*********|######## 3 #######|
	A           ----------------|---------|----------------------------->
	|                         INVIA      ISR
	|                                    ISR              LEGGI
	B           ---------------------------|-----------------|----------->
	stato tx            |######## 2 #######|

- [`   `]: nessuna restrizione, è il momento giusto per trasmettere un messaggio
- [`***`]: impossibile trasmettere, `invia()` aspetta che sia di nuovo possibile (ma al
    massimo 50 ms)
- [`###`]: la funzione `invia()` non dovrebbe mai essere chiamata qui.
    -  [`1`]: In teoria non bisognerebbe trasmettere (l'altra radio non è in modalità rx),
        ma in realtà se l'utente chiama `invia()` mentre la classe aspetta un ack per
        il messaggio precedente significa che l'utente ha rinunciato a controllare
        quell'ack. In tal caso `invia()` si comporta come se il messaggio precedente
        non avesse contenuto una richiesta di ack. Probabilmente questo messaggio
        andrà perso, ma il compito della funzione `invia()` non è aspettare l'ack
        precedente (quello è compito dell'utente, anche se lo aspettasse per un certo
        tempo `invia()` non potrebbe segnalare se è arrivato o no). La sequenza corretta
        sarebbe:

        ```cpp
		inviaConAck();
		aspettaAck(); 		// contiene un timeout
		if(ackRicevuto())
			invia(); 		// prossimo messaggio
        ```
        oppure

        ```cpp
        inviaConAck();
        delay(x); 			// potrebbero essere altre funzioni
        if(ackRicevuto())
			invia(); 		// prossimo messaggio  
        rinunciaAck(); 		// smetti di aspettare l'ACK
        ```
    - [`2`]: Momento critico: se si chiama invia() qui ci sarà una collisione con
        l'invia() della radio A e entrambi i messaggi saranno persi, ma questa classe
        non ha modo di evitarlo. Spetta all'utente impedire queste collisioni o saperle
        gestire.
    - [`3`]: I messaggi inviati qui saranno persi. È un difetto dei messaggi senza ACK.


<br><div id='4'/>

## 4. Hardware ##

Come già detto ho scritto questa classe in particolare per il modulo RFM69HCW
di HopeRF, in commercio sia da solo sia inserito in altri moduli che offrono,
ad esempio, un logic level shifting da 5V a 3.3V (ad es. Adafruit vende
https://www.adafruit.com/product/3071 per la maggior parte dei paesi, tra cui
tutti quelli europei, e https://www.adafruit.com/product/3070 per gli USA e pochi
altri).

Il modulo comunica con il microcontrollore tramite SPI, deve poter chiamare
un'interrupt su quest'ultimo e può "affidargli" il proprio pin di reset (non
veramente sfruttato da questa classe, ma se è già connesso deve essere gestito
per evitare reset indesiderati). Deve essere alimentato con una tensione di 3.3V.


| RFM69       | uC        |
|-------------|-----------|
| MISO        | MISO      |
| MOSI        | MOSI      |
| SCK         | SCK       |
| NSS         | I/O `*`   |
| DIO0        | INT `**`  |
| _RESET `&`_ | _I/O `*`_ |

- `&`:  Opzionale
- `*`:  OUT è qualsiasi pin di input/output (sarà configurato come output dalla classe)
- `**`: INT è un pin capace di attivare un interupt del microcontrollore. Ad esempio
    su Atmega328p, il microcontrollore di Arduino UNO, si possono usare i pin 4
    e 5, cioé rispettivamente 2 e 3 nell'ambiete di programmazione Arduino.


<br><div id='5'/>

## 5. Struttura dei messaggi ##

Tutti i messaggi inviati con le funzioni di questa classe hanno la seguente
struttura:

| Preamble       | Sync word  | Lunghezza | Intestazione | Contenuto | CRC |
|----------------|------------|-----------|--------------|-----------|-----|
| PREAMBLE_SIZE  | SYNC_SIZE  | 1         | 1            | lunghezza | 2   |
| 01010101...    | SYNC_VAL   | lunghezza | intestazione | messaggio | crc |

La prima riga è la lunghezzza della sezione in bytes, la seconda è il suo contenuto.

- `PREAMBLE_SIZE`, `SYNC_SIZE` e `SYNC_VAL` sono costanti definite nel file
    "RFM69_impostazioni.h".
- `lunghezza` e `messaggio` sono gli argomenti della funzione `invia()`.
- `intestazione` è un byte generato dalle funzioni di invio e letto da quelle di
    ricezione, inaccessibile all'utente.
- `crc` è un Cyclic Redundancy Checksum generato dalla radio.


<br><div id='7'/>

## 6. Impostazioni ##

Il modulo RFM69 offre all'utente ampie possibilità di impostazione.
Alcune di queste impostazioni sono richieste dalla classe, come ad esempio il
modo di trasmissione dei dati (che deve essere "a pacchetti"). La maggior parte
resta però a disposizione dell'utente.


<br><div id='7'/>

## 7. Esempio di utilizzo ##

```cpp

#include <Arduino.h>
#include "RFM69.h"


// #define MODULO_r o MODULO_t per compilare rispettivamente il programma per la
// radio ricevente o per quella trasmittente.
//------------------------------------------------------------------------------
#define MODULO_r
// #define MODULO_t
//------------------------------------------------------------------------------




// telecomando
#ifdef MODULO_r
// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(2, 3);
// Un LED, 0 per non usarlo
#define LED 4
#endif

// quadricotetro
#ifdef MODULO_t
// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(A2, 3, A3);
// Un LED, 0 per non usarlo
#define LED 7
#endif




void setup() {

    Serial.begin(115200);
    if(LED) pinMode(LED, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Restituisce 0
    int initFallita = radio.inizializza(4);
    if(initFallita) {
        // Stampa l'errore riscontrato (questa funzione pesa quasi 0.5 kB)
        radio.stampaErroreSerial(Serial, initFallita);
        // Inizializzazione fallita, blocca il progrmma
        while(true);
    }
}




#ifdef MODULO_t


void loop(){

    // crea un messaggio
    uint8_t lung = 4;
    uint8_t mess[lung] = {0,0x13, 0x05, 0x98};

    unsigned long t;
    bool ok;

    while(true) {

        // Aggiorna messaggio
        mess[0] = (uint8_t)radio.nrMessaggiInviati();

        Serial.print("Invio...");
        if(LED) digitalWrite(LED, HIGH);

        // Registra tempo di invio
        t = millis();

        // Invia
        radio.inviaConAck(mess, lung);
        // Aspetta fino alla ricezione di un ack o al timeout impostato nella classe
        while(radio.aspettaAck());
        // Controlla se è arrivato un Ack (l'attesa può finire anche senza ack, per timeout)
        if(radio.ricevutoAck()) ok = true;  else ok = false;

        // calcola il tempo trascorso dall'invio
        t = millis() - t;

        if(LED) digitalWrite(LED, LOW);

        if(ok) {
            Serial.print(" mess #");
            Serial.print(radio.nrMessaggiInviati());
            Serial.print(" trasmesso in ");
            Serial.print(t);
            Serial.print(" ms");
        }
        else {
            Serial.print(" messaggio #");
            Serial.print(radio.nrMessaggiInviati());
            Serial.print(" perso");
        }
        Serial.println();

        delay(1000);
    }
}

#endif



#ifdef MODULO_r


void loop(){

    // metti la radio in modalità ricezione
    radio.iniziaRicezione();

    // aspetta un messaggio
    while(!radio.nuovoMessaggio());

    if(LED) digitalWrite(LED, HIGH);

    // ottieni la dimensione del messaggio ricevuto
    uint8_t lung = radio.dimensioneMessaggio();
    // crea un'array in cui copiarlo
    uint8_t mess[lung];
    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);
    // ora `mess` contiene il messaggio e `lung` corrisponde alla lunghezza del
    // messaggio (in questo caso corrispondeve anche prima, ma avrebbe anche
    // potuto essere più grande, ad. es. se mess. fosse stato un buffer generico
    // già allocato alla dimensione del messaggio più lungo possibile)

    if (erroreLettura) {
        Serial.print("Errore lettura");
    }
    else {
        Serial.print("Messaggio (");
        Serial.print(lung);
        Serial.print(" bytes): ");
        for(int i = 0; i < lung; i++) {
            Serial.print(" 0x");
            Serial.print(mess[i], HEX);
        }
        Serial.print("  rssi: ");

        // Ottieni il valore RSSI del segnale che ha portato questo messaggio
        Serial.print(radio.rssi());
    }
    Serial.println();

    delay(50);
    if(LED) digitalWrite(LED, LOW);

}
#endif
```
