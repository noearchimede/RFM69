/*! @file
@brief Header della classe RFM69
*/


/* NOTA PER LA DOCUMENTAZIONE DOXYGEN
Se si desidera creare una pagina di documentazione esclusivamente per questa classe
bisogna attivare il testo reso condizionale da `@cond DOC_RFM69`, che è ignorato
per l'inserimento della documentazione di questa classe in quella di un intero progetto.
*/



/*! @class    RFM69
    @brief    Driver per i moduli radio %RFM69
    @author   Noè Archimede Pezzoli (noearchimede@gmail.com)
    @date     Febbraio 2018


@cond DOC_RFM69

    Per una descrizione approfondita, cfr. la pagina @ref index

*/
/*!
@mainpage Driver per i moduli radio %RFM69

@endcond

La classe RFM69 permette di collegare due microcontrollori tramite moduli radio
della famiglia %RFM69 di HopeRF, e in particolare tramite il modulo RFM69HCW
(http://www.hoperf.com/rf_transceiver/modules/RFM69HCW.html). Non ho eseguito
alcun test sugli altri moduli.

Alla fine di questo testo si trova un esempio dell'utilizzo di questa classe.


@par Caratteristiche del modulo radio

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

Corrente di alimentazione richiesta (a 3.3V), per modalità:
    - Sleep:      0.0001 mA
    - Standby:    1.25 mA
    - Rx:         16 mA
    - Tx:         16 - 130 mA a seconda della potenza di trasmissione


@par Protocollo di comunicazione

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

Gli schemi sottostanti illustrano la trasmissione di un mesasggio. Nel primo caso
si tratta di un messaaggio con richiesta di ACK, nel secondo no.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mod       ?   |    tx    |            rx             |        def
fz          INVIA       ISR                         ISR
A       ------|----------|---------------------------|-------------------->
RF             |||MESS|||                    ^^^^^^^
|              vvvvvvvv                      ||ACK||
B       ------------------|-----------------|-------|--------------------->
fz                       ISR              LEGGI    ISR
mod             rx        |       stby      |  tx   |          def
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\n
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mod       ?   |    tx    |              def
fz          INVIA       ISR
A       ------|----------|--------------------------------->
RF             |||MESS|||
|              vvvvvvvvv
B       ------------------|-----------------|--------------->
fz                       ISR              LEGGI
mod             rx        |       stby      |       def
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- `A, B`: Programma delle stazioni radio, evoluzione nel tempo
- `fz`: funzioni chiamate. `invia()` e `leggi()` sono chiamate dall'utente, `isr()`
    è l'interrupt service routine della classe
- `mod`: modalità della radio. `tx` = trasmissione, `rx` = ricezione, `stby` = standby,
    `def`: la modalità che l'utente ha scelto come default per quella radio
- `RF`: presenza di segnali radio e loro direzione


@par Collisioni

Le funzioni di questa classe non impediscono che le due radio trasmettano dei
messaggi contemporaneamente. Questo problema deve essere gestito come possibile
dal codice dell'utente. Tuttavia le funzioni della classe in caso di conflitto
impediscono la perdita di entrambi i messaggi (cosa che potrebbe portare a un
blocco senza uscita se entrambi i programmi cercassero di reinviare subito il
proprio messaggio). Dà quindi la priorità ai messaggi già arrivati a scapito di
quelli in uscita, che potrebbero perdersi.

Lo schema sottostante mostara i momenti in cui non si può o non si dovrebbe
trasmettere. Il primo schema si riferisce ai messaggi con richiesat di ACK, il
secondo a quelli senza.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
stato tx                    |*********|############ 1 ############|
A           ----------------|---------|---------------------------|---------->
|                         INVIA      ISR                         ISR
|                                    ISR              LEGGI    ISR
B           ---------------------------|-----------------|-------|----------->
stato tx            |####### 2 ########|                 |*******|
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\n
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
stato tx                    |*********|######## 3 #######|
A           ----------------|---------|----------------------------->
|                         INVIA      ISR
|                                    ISR              LEGGI
B           ---------------------------|-----------------|----------->
stato tx            |######## 2 #######|
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- [   ]: nessuna restrizione, è il momento giusto per trasmettere un messaggio
- [***]: impossibile trasmettere, invia() aspetta che sia di nuovo possibile (ma al
    massimo 50 ms)
- [###]: la funzione invia() non dovrebbe mai essere chiamata qui:
    1. CHIAMATA AD `invia()` QUI --> PROBLEMA NEL CODICE DELL'UTENTE
        In teoria non bisognerebbe trasmettere (l'altra radio non è in modalità rx),
        ma in realtà se l'utente chiama invia() mentre la classe aspetta un ack per
        il messaggio precedente significa che l'utente ha rinunciato a controllare
        quell'ack. In tal caso invia() si comporta come se il messaggio precedente
        non avesse contenuto una richiesta di ack. Probabilmente questo messaggio
        andrà perso, ma il compito della funzione invia() non è aspettare l'ack
        precedente (quello è compito dell'utente, anche se lo aspettasse per un certo
        tempo invia() non potrebbe segnalare se è arrivato o no). La sequenza corretta
        sarebbe: invia() con richiesta ack -> aspettaAck(), che contiene un timeout ->
        ackRicevuto()? -> invia() prossimo messaggio, oppure invia() -> delay(x) ->
        ackRicevuto()? -> rinunciaAck() invia()
    2. momento critico: se si chiama invia() qui ci sarà una collisione con
        l'invia() della radio A e entrambi i messaggi saranno persi, ma questa classe
        non ha modo di evitarlo. Spetta all'utente impedire queste collisioni o saperle
        gestire.
    3. i messaggi inviati qui saranno persi. È un difetto dei messaggi senza ACK.



@par Hardware

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


| %RFM69    | uC      |
|-----------|---------|
| MISO      | MISO    |
| MOSI      | MOSI    |
| SCK       | SCK     |
| NSS       | I/O *   |
| DIO0      | INT **  |
| _RESET &_ | _I/O *_ |

- &:  Opzionale
- *:  OUT è qualsiasi pin di input/output (sarà configurato come output dalla classe)
- **: INT è un pin capace di attivare un interupt del microcontrollore. Ad esempio
    su Atmega328p, il microcontrollore di Arduino UNO, si possono usare i pin 4
    e 5, cioé rispettivamente 2 e 3 nell'ambiete di programmazione Arduino.


@par Struttura messaggi

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

@cond DOC_RFM69
\n
<b>Documentazione dettagliata dei membri della classe: @ref RFM69 </b>
@endcond



\n\n
@par Esempio di utilizzo
\n
@code {.cpp}

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

@endcode
*/




#ifndef RFM69_h
#define RFM69_h


#include <Arduino.h>
#include <SPI.h>

class RFM69 {

public:

    //! @name Constructor
    //!@{

    //! Constructor da usare se il pin RESET della radio non è connesso al uC.
    /*! @param pinSS          Numero del pin Slave Select
        @param pinInterrupt   Numero del pin attraverso il quale la radio genera
            un interrupt sul uC. Deve ovviamente essere un pin di interrupt.
    */
    RFM69(uint8_t pinSS, uint8_t pinInterrupt);

    //! Constructor da usare se il pin RESET è connesso al uC.
    /*! @param pinSS          Numero del pin Slave Select
        @param pinInterrupt   Numero del pin attraverso il quale la radio genera
            un interrupt sul uC. Deve ovviamente essere un pin di interrupt.
        @param pinReset       Numero del pin collegato al pin RESET della radio.
    */
    RFM69(uint8_t pinSS, uint8_t pinInterrupt, uint8_t pinReset);

    //!@}
    /*! @name Inizializzazione
    Funzione di inizializzazione della classe, da chiamare prima di qualsiasi altra
    */
    //!@{

    //! Inizializza la radio. Deve essere chiamato all'inizio del programma.
    /*! La funzione esegue le seguenti operazioni in questo ordine:
        - controllo che la classe non debba gestire troppe radio (cioé più di una)
        - prepara l'interfaccia SPI per comunicare con la radio
        - (eventualmente esegue il reset della radio)
        - controlla che un dispositivo sia connesso
        - controlla che il dispositivo sia una radio %RFM69
        - collega l'interrupt della radio all'ISR della classe
        - scrive tutti i registri della radio inserendovi le impostazioni stabilite
            nel file RFM69_impostazioni.h
        - inizializza alcune variabili della classe
        - crea un buffer di `lunghezzaMaxMessaggio` bytes che resterà allocato
            fino alla distruzione dell'istanza della classe.

        @note L'esecuzione di questa funzione richiede alcuni decimi di secondo.


        @note Sarà allocata un'array di `lunghezzaMaxMessaggio` bytes.

        @param lunghezzaMaxMessaggio  Lunghezza massima dei messagi ricevuti da
            questa radio. Può essere diverso dalla lunghezza massima dei messaggi
            inviati (quindi da questo stesso parametro sull'altra radio)

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int inizializza(uint8_t lunghezzaMaxMessaggio);


    //!@}
    /*! @name Funzioni fondamentali
    Devono essere usate in ogni programma per permettere una comunicazione radio
    */
    //!@{

    //! Invia un messaggio
    /*! Questa funzione prepara il modulo radio per trasmettere un messaggio. La
        trasmissione inizierà alla fine di questa funzione e sarà terminata
        dall'isr().

        Cfr. la descrizione generale della classe @ref RFM69 per informazioni
        su quando questa funzione può o non può essere usata.

        @param messaggio[in]  array di bytes (`uint8_t`) che costituiscono il messaggio
        @param lunghezza[in]  lunghezza del messaggio in bytes

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int invia(const uint8_t messaggio[], uint8_t lunghezza);

    //! Invia un messaggio con richiesta di ACK
    /*! Il messaggio inviato conterrà una richiesta di ACK, che dovrà poi essere
        gestito da altre funzioni. Questa agisce come invia(), cioé prepara il
        modulo radio per trasmettere un messaggio.
        La trasmissione inizierà alla fine di questa funzione e sarà terminata
        dall'isr().

        Cfr. la descrizione generale della classe @ref RFM69 per informazioni
        su quando questa funzione può o non può essere usata.

        @param messaggio[in]  array di bytes (`uint8_t`) che costituiscono il messaggio
        @param lunghezza[in]  lunghezza del messaggio in bytes

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int inviaConAck(const uint8_t messaggio[], uint8_t lunghezza);

    //! Invia un messaggio ripetutamente fino alla ricezione di un ACK
    /*! Questa funzione invia un messaggio, aspetta l'ACK e se non lo riceve entro
        il tesmpo specificato in `impostaTimeoutAck()` lo invia di nuovo. Ripete
        questa operazione per al massimo `tentativi` volte, dopo le quali
        restituirà 1 (`Errore::ListaErrori::errore`) se il messaggio non è ancora
        stato confermato (quindi probabilmente non è arrivato). Restituirà invece
        0 (`Errore::ListaErrori::ok`)subito dopo aver ricvevuto un ACK.

        Dopo l'esecuzione della funzione `tentativi` conterrà il numero di tentativi
        effettuati.

        @param messaggio[in]      array di bytes (`uint8_t`) che costituiscono il messaggio
        @param lunghezza[in]      lunghezza del messaggio in bytes
        @param tentativi[in/out]  /b prima: numero di tentativi da effettuare prima
                                  di rinunciare alla trasmissione del messaggio
                                  /b dopo: numero di tentativi effettuati

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int inviaFinoAck(const uint8_t messaggio[], uint8_t lunghezza, uint16_t& tentativi);

    //! Restituisce un messaggio, se ce n'è uno da leggere
    /*! Il messaggio è trasferito dalla radio al microcontrollore già nell'isr().
        Questa funzione restituisce all'utente il contenuto del buffer della classe
        senza accedere alla radio. Deve tuttavia utilizzarla se il messaggio ricevuto
        contiene una richiesta di ACK. In tal caso alla fine della funzione viene
        iniziata la trasmissione dell'ack. Se è già in corso una trasmissione
        (funzione invia() chiamata prima di leggi(), normalmente non dovrebbe
        succedere), leggi() aspetterà fino alla fine della trasmissione prima di
        inviare l'ack; dopo 50 ms interromperà ogni eventuale trasmissione.

        @param messaggio  array[out] in cui sarà copiato il messaggio. Deve essere almeno
            grande quanto il messaggio, del quale si può ottenere la lunghezza con
            la funzione dimensioneMessaggio().
        @param lunghezza [in/out]  lunghezza di messaggio. Dopo l'esecuzione della
            funzione corrisponderà esattamente alla dimensione del messaggio.

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int leggi(uint8_t messaggio[], uint8_t &lunghezza);

    //! Attiva la radio in modo che possa ricevere dei messaggi
    /*! Per riceve la radio deve essere in modalità ricezione. Nelle altre modalità
        non si accorgerà nemmeno di aver ricevuto un messaggio. Quindi se non si
        chiama questa funzione sulla radio ricevente tutti i messaggi inviatile
        andranno persi.
    */
    int iniziaRicezione();


    //!@}
    /*! @name Funzioni importanti
    Probabilmente saranno chiamate in tutti i programmi, ma non sono strettamente
    indispensabili
    */
    //!@{

    //! Controlla se c'è un nuovo messaggio
    /*! @return `true` se il buffer della classe contiene un nuovo messaggio.
    */
    bool nuovoMessaggio();

    //! Restituisce la dimensione dell'ultimo mesasggio
    /*! @return la dimensione dell'utlimo messaggio ricevuto in bytes
    */
    uint8_t dimensioneMessaggio();

    //! Restituisce true se la classe sta aspettando un ACK
    /*! @return `true` se la classe sta aspettando un ack, cioé se ha inviato un
            messaggio con richiesta di ACK e non lo ha ancora ricevuto.

        Questa funzione contiene un sistema di timeout. Dopo lo scadere del tempo
        restituisce `false` anche se non ha ricevuto nessun ack.

        @warning `false` ha due signifcati opposti:
              1. L'ACK è stato ricevuto
              2. L'ACK non è arrivato, ma il tempo massimo di attesa ê scaduto.
            Per questo è necessario chiamare _sempore_ anche ricevutoAck().
    */
    bool aspettaAck();

    //! Restituisce true se la radio ha ricevuto un ACK
    /*! @return `true` se la radio ha ricevuto un ACK per l'ultimo messaggio inviato

        Questa funzione non aspetta l'ACK, dice solo se è arrivato, quindi chiamata
        immediatamente dopo `invia()` restituisce sempre `false`. Normalmente è
        perciò usata insieme a aspettaAck() (cioé subito dopo di essa).

        Esempio:
        ~~~{.cpp}
        for(int i = 0; i < 3; i++) {
            invia();
            aspettaAck();
            if(ricevutoAck()) break;
        }
        if(!ricevutoAck()) {
            print("Trasmissione messaggio fallita");
        }
        ~~~
    */
    bool ricevutoAck();

    //! Simula la ricezione di un ACK.
    /*!
        @note Normalmente questza funzione non dovrebbe mai essere chiamata.

        Questa funzione è chiamata automaticamente da aspettaAck() allo scadere
        del tempo massimo. Può essere usata dall'utente per terminare l'attesa
        prima di quella scadenza.
            In caso di invio di un secondo messaggio prima della ricezione dell'ACK
        (ad es. per trasmettere un'informazione urgente) è chiamata automaticamente.

        Se il vero ACK dovesse arrivare dopo l'esecuzione di questa funzione
        non avrà nessun effetto.
    */
    void rinunciaAck();

    //! Mette la radio in standby (richiederà 1.25mA di corrente)
    /*! La modalità `standby` serve per mettere la radio in pausa per qualche secondo.
        Per pause più lunghe conviene usare `sleep()`, soprattutto se l'intero
        dispositivo deve essere messo in standby per un certo periodo. Se invece
        il dispositivo utilizza relativamente tanta corrente durante lo standby
        della radio (ad es. ha accesi diversi LED, un motore, ...) la differenza
        tra `sleep()` e `standby()` non è rilevante.
    */
    int standby();

    //!@}
    /*! @name Funzioni ausiliarie
    Utili ma non indispensabili
    */
    //!@{

    //! Mette la radio in modalità `listen`
    /*! `listen` è una modalità particolare che consiste in realtà nella continua
        alternanza tra due modalità: `rx` (ricezione) e `idle` (una specie di
        sleep adattato alla modalità listen).
    */
    int listen();

    //! Mette la radio in modalità sleep (richiederà 0.1uA di corrente)
    /* @ref standby()
    */
    int sleep();

    //! Imposta la modalità di default della radio.
    /*! Questa funzione non cambia la modalità attuale!\n
        Ad es.`sleepDefault();` non corrisponde a `sleepDefault(); sleep();`
    */
    void sleepDefault();
    //! @copydoc sleepDefault()
    void standbyDefault();
    //! @copydoc sleepDefualt()
    void listenDefault();
    //! @copydoc sleepDefualt()
    void rxDefault();

    //! Imposta il tempo d'attesa massimo per un ACK
    /*! @param tempoMs Tempo di attesa in millisecondi per la funzione `aspettaAck()`.
               Dopo aver atteso per questo tempo la funzione terminerà senza
               aver ricevuto un ACK.
    */
    void impostaTimeoutAck(uint16_t tempoMs);

    //! Restituisce il valore RSSI per l'ultimo messaggio
    /*! @return Received Signal Strength Indicator (RSSI) per l'ultimo messaggio
                ricevuto.
    */
    int8_t rssi();
    //! Restituisce l'"ora" della ricezione dell'ultimo messaggio.
    /*! @return Il tempo in millisecondi dall'inizio del programma a cui è stata
                attivata l'`isr()` che segna la finem della ricezione del messaggio.
    */
    uint8_t tempoRicezione();


    //!@}
    /*! @name Log
    Funzioni utili per monitorare il funzionamento della radio
    */
    //!@{

    //! Restituisce il numero di messaggi inviati dopo l'ultima inizializzazione
    /*! @return Il numero di messaggi inviati dopo l'ultima inizializzazione
    */
    int nrMessaggiInviati() {return messaggiInviati;}
    //! Restituisce il numero di messaggi ricevuti dopo l'ultima inizializzazione
    /*! @return Il numero di messaggi ricevuti dopo l'ultima inizializzazione
    */
    int nrMessaggiRicevuti() {return messaggiRicevuti;}


    //! Stampa la descrizione di un errore sul monitor seriale
    /*! Questa funzione permette di stampare sul monitor seriale la causa di un
        errore segnalato da una funzione di questa classe.

        Funziona per i codici di errore definiti dall'`enum` Errore::ListaErrori.

        @note Le stringhe di testo sono salvate nella memoria flash (del programma),
        non nella SRAM (delle variabili), e pesano circa 0.5kB.

        @note Siccome contiene alcune centinaia di caratteri, questa funzione "pesa"
              più delle altre. Si consiglia perciò di usarla solo in fase di debug
              per poi liberare spazio quando la radio funziona.

        @param serial un oggetto di `HardwareSerial`. Tipicamente sarà `Serial`
              (o ev. `Serial1` ecc. se si usa un Arduino Mega).
        @paral errore codice di errore restituito da una delle funzioni sopra elencate.
    */
    void stampaErroreSerial(HardwareSerial &serial, int errore);


    //!@}
    /*!@name Debug
        Le seguenti funzioni permettono all'utente di leggere i registri della radio,
        normalmentenon sono utilizzate
    */
    //!@{

    //! Stampa il valore di tutti i registri della radio sul monitor seriale
    /*! Strampa il valore di tutti i registri della radio
    */
    void stampaRegistriSerial(HardwareSerial& Serial);
    //! Leggi il valore di un registro della radio
    /*  @param indirizzo l'indirizzo del registro da leggere. Si consignlia di
                         `#include`re il file RFM69_registri.h per avere un elenco
                         `#define`d dei nomi dei registri con il loro indirizzo.
        @return il valore del registro selezionato
    */
    uint8_t valoreRegistro(uint8_t indirizzo);
    //!@}

    /*! @name Destructor, copy constructor, copy operator
    Queste funzioni servono per evitare perdite di memoria visto che la classe
    gestisce una risorsa allocata dinamicamente (il buffer) e non dovrebbe mai
    essere copiata
    */
    //!@{

    //! Destructor
    /*! Dopo aver chiamato il destructor su un'istanza è possibile chiamare
    `inizializza()` su un'altra senza ricevere l'errore
    `Errore::ListaErrori::initTroppeRadio`
    */
    ~RFM69(); // Implementato
    //! Copy Constructor
    /*! Deleted perché non ha senso copiare una radio
    */
    RFM69(const RFM69&) = delete;
    //! Copy Operator
    /*! Deleted perché non ha senso copiare una radio
    */
    RFM69& operator = (const RFM69&) = delete;

    //!@}



    //! Contenitore dell'`enum` Errore::ListaErrori.
    /*! vedi @ref ListaErrori per dettagli sui codici di errore
    */
    struct Errore {
        //! Lista degli errori che le funzioni della classe possono restituire
        /*! Tutte le funzioni di RFM69 che restituiscono un codice di errore utilizzano
            questa `enum` per definirlo. Si tratta delle funzioni seguenti:
            - `inizializza()`
            - `inviaMessaggio()` (privata) e i suoi derivati pubblici (`leggi()`)
            - `leggi()`
            - `cambiaModalita()` (privata) e i suoi derivati pubblici
            (`iniziaRicezione()`, `standby()`, ...)

            I codici di errore vengono sempre forniti all'utente sotto forma di `int`
            per non costringerlo a usare questa enum se non vuole (i valori dei codici
            sono espliciti, dunque l'untente può limitarsi a stamparli e guardare a che
            errore corrispondono leggendo la definizione dell'`enum`).
        */
        enum ListaErrori {

            //! Nessun errore
            ok                          = 0, // false
            //! %Errore generico
            errore                      = 1, // true

            /*! inizializza(): Visto che ha una sola ISR questa classe può gestrire
            una sola radio ma il programma ha provato ad inizializzarne una seconda
            */
            initTroppeRadio             = 2,
            /*! inizializza(): L'inizializzazione di SPI non è riuscita
            */
            initInitSPIFallita          = 3,
            /*! inizializza(): Non è stata trovato nessun dispositivo connesso
            a SPI con lo Slave Select specificato nel constructor
            */
            initNessunaRadioConnessa    = 4,
            /*! inizializza(): È stato trovato un dispositivo ma non è una radio
            %RFM69
            */
            initVersioneRadioNon0x24    = 5,
            /*! inizializza(): Il pin scelto come interrupt non va bene perché
            non è collegato ad alcun interrupt nel microcontrollore
            */
            initPinInterruptNonValido   = 6,
            /*! inizializza(): %Errore nella scrittura dei registri della radio
            */
            initErroreImpostazione      = 7,


            /*! inviaMessaggio(): La lunghezza del messaggio è nulla
            */
            inviaMessaggioVuoto         = 8,
            /*! inviaMessaggio(): invia() è stata chiamata mentre c'era un messaggio
            in uscita, la funzione ha aspettato per più del tempo massimo di invio di
            un messaggio e alla fine dell'attesa il messaggio non era ancora partito
            */
            inviaTimeoutTxPrecedente    = 9,


            /*! leggi(): Non c'è nessun nuovo messaggio da leggere
            */
            leggiNessunMessaggio        = 10,
            /*! leggi(): l'array in cui la funzion `leggi()` dovrerbbe copiare il
            messaggio è troppo corta per contenerlo
            */
            leggiArrayTroppoCorta       = 11,
            /*! leggi(): il messaggio è troppo lungo per essere letto da questa
            istanza della classe, che è stata inizializzata con un buffer di
            dimensione inferiore
            */
            messaggioTroppoLungo        = 12,

            /*! cambiaModalita(): Nel contesto in cui è stata chiamata non è possibile
            passare alla modalità richiesta
            */
            modImpossibile              = 13,
            /*! cambiaModalita(): Il cambio di modalità non è avvenuto entro un
            tempo massimo ampiamente sufficiente.
            */
            modTimeout                  = 14,
        };
    };

private:

    // All'inizio vale 0 (come tutte le variabili `static`); ogni volta che
    // il constructor di questa classe viene chiamato il suo valore aumenta di
    // uno. In questo modo si scopre se esiste più di un'istanza di questa classe,
    // che per funzionare può averne una sola a causa dell'isr, che è una sola.
    // Per consentire l'utilizzo di più radio si può creare una nuova isr per
    // ogni radio e collegare un'isr diversa a ogni istanza oppure gestire in
    // qualche modo l'utilizzo comune di un'unica isr (ad esempio con funzioni
    // "on" ed "off" per ogni radio). Il numero massimo di isr è limitato dal
    // numero di interrupt possibili.
    static unsigned int nrIstanze;



    // ### TIPI COMPOSTI PRIVATI DELLA CLASSE ###

    // Modalità possibili
    enum class Modalita {
        sleep,
        standby,
        fs,
        tx,
        rx,
        listen
    };

    // Union per scrivere/leggere e trasmettere l'intestazione
    union Intestazione {
        struct {
            uint8_t ack : 1;
            uint8_t richiestaAck : 1;
        } bit;          // scrittura e lettura
        uint8_t byte;   // trasmissione

        Intestazione() : byte(0) {}
    };


    // Struct per salvare informazioni sui messaggi ricecvuti
    struct InfoMessaggio {
        uint8_t dimensione;
        Intestazione intestazione;
        uint32_t tempoRicezione;
        int8_t rssi;
    };


    // ### GESTIONE COMUNICAZIONI ###

    //! [privata] Funzione di invio dei messaggi
    /*! Questa funzione è alla base di tutte le funzioni di invio messaggi, che
        si limitano a scrivere l'intestazione del messaggio prima di chiamarla.
        Le funzioni pubbliche di invio sono:
        - `inviaConAck()`
        - `inviaFinoAck()`
        - `invia()`
    */
    int inviaMessaggio(const uint8_t messaggio[], uint8_t lunghezza, uint8_t intestazione);

    //! Imposta la modalità di funzionamento
    /*! Cambia la modalità della radio. Questa funzione è chiamata per ogni
        cambiamento di modalità, richiesto dall'utente direttamente
        (`iniziaRicezione();`, `standby();`, ...), indirettamente (`invia()`;, ...)
        o dall'`isr()`. Se richiesta dall'utente può bloccare il programma per
        qualche millisecondo, soprattutto in caso di cambiamento verso la modalità
        listen, o da o verso la modalità trasmissione, mentre è molto rapida se
        chiamata nell'`isr()`.
    */
    int cambiaModalita(Modalita, bool aspetta = true);

    // Scrive le impostazioni "high power" (per l'utilizzo del modulo con una potenza
    void highPowerSettings(bool attiva);
    // Invia un ACK
    void inviaAck();

    // # ISR #
    // ISR che reagisce ai segnali di interrupt della radio chiamando l'interrupt
    // handler `isr()`
    static void isrCaller();

    //! Interrupt handler
    /*! Funzione chiamata quando la radio richiede un interrupt tramite il proprio
    pin DIO0. I momenti in cui può essere chiamata sono illustrati in uno schema
    nella sezione Protocollo di comunicazione della descrizione generale della
    classe @ref RFM69.
    */
    void isr();
    // `this` pointer a disposizione dell'ISR
    static RFM69* pointerRadio;





    // ### FUNZIONI DI IMPOSTAZIONE ###

    // Scrive in ogni registro della radio il valore definito nel file di impostazione
    bool caricaImpostazioni();



    // ### CONSTANTI DI IMPOSTAZIONE ###

    // ## Specifiche di ogni radio ##

    // Modalità usata quando non ne è specificata un'altra
    Modalita modalitaDefault = Modalita::standby;

    // Tempo massimo di attesa dell'ACK (deve essere scelto in base alla frequenza
    // con cui viene chiamata la funzione `leggi()` sull'altra radio
    uint16_t timeoutAck = 250;

    // Dimensione massima dei messaggi in entrata
    uint8_t lungMaxMessEntrata;


    // Pin
    const uint8_t pinSS;
    const uint8_t pinInterrupt;
    const uint8_t pinReset; //non usato se `haReset == false`
    // Il microcontrollore può controllare il pin Reset della radio
    const bool haReset;


    // ## Uguali per tutte le radio nella rete ##

    // La radio è in modalità high power e richiede impostazioni particolari
    // a ogni trasmissione.
    bool highPower;


    // ### VARIABILI ###

    // Modalità in cui si trova attualmente la radio
    volatile Modalita modalita = Modalita::sleep;
    // La radio sta trasmettendo un messaggio
    volatile bool trasmissioneMessaggio = false;
    // La radio sta trasmettendo un ACK
    volatile bool trasmissioneAck = false;
    // "ora" di trasmissione dell'ultimo messaggio (ms)
    volatile uint32_t tempoUltimaTrasmissione = 0;
    // Un ACK richiesto non è ancora stato ricevuto
    volatile bool attesaAck = false;
    // È stato ricevuto un ACK per l'ultimo messaggio inviato
    volatile bool ackRicevuto = false;
    // Nella radio c'è un nuovo messaggio da leggere
    volatile bool messaggioRicevuto = false;
    // Informazioni sull'ultimo messaggio ricevuto
    volatile InfoMessaggio ultimoMessaggio;
    // pointer al buffer in cui l'ISR copia la memoria FIFO del messaggio ricevuto
    uint8_t* buffer = nullptr;


    // totale di messaggi inviati dall'ultima inizializzazione
    volatile uint16_t messaggiInviati;
    // totale di messaggi ricevuti dall'ultima inizializzazione
    volatile uint16_t messaggiRicevuti;

    // Numero di ACK ricevuti mentre `attesaAck == false`
    uint16_t ackInattesi = 0;
    // Numero di trasmissioni fallite per timeout
    uint16_t trasmissioniFalliteTimeout = 0;



    // ### SPI ###

    // Gestione di SPI (membro privato di RFM69)
    // Questa classe è scritta per interagire con la radio RFM69, non con
    // un dispositivo qualsiasi, anche se probabilmente si adatterebbe molto
    // bene anche a questo scopo.
    // Il comportamento in caso di utilizzo di SPI anche da parte di altre parti
    // del programma non è stato testato, ma in teoria dovrebbe essere corretto
    // (questa classe scvrive nei registri di SPI le proprie impostazioni prima
    // ogni trasferimeto di dati).
    //
    class Spi {

    public:
        // Impostazioni
        enum BitOrder {LSBFirst, MSBFirst};
        enum DataMode {cpol0cpha0, cpol0cpha1, cpol1cpha0, cpol1cpha1};

        // Constructor. Seleziona il pin da usare come Slave Select per la radio
        // e imposta SPI con:
        // pinSS:        pin di Slave Select della radio
        // frequenzaHz:  frequenza della clock di SPI
        // bitOrder:     LSBFirst o MSBFirst. La radio richiede il secondo
        // dataMode:     modalità di SPI (cpol: Clock POLarity, cpha: Clock PHAse)
        //
        Spi(uint8_t pinSS, uint32_t frequenzaHz, BitOrder bitOrder, DataMode dataMode);

        // inizializza SPI
        bool inizializza();


        // ### Funzioni standard di lettura/scrittura ###


        // leggi un registro della radio (un byte)
        uint8_t leggiRegistro(uint8_t addr);
        // scrivi un byte in un registo della radio
        void scriviRegistro(uint8_t addr, uint8_t val);

        // questa funzione deve essere chiamata prima e dopo l'utilizzo di SPI
        // all'interno di un'ISR (prima cona rgomento `true`, dopo con `false)
        void usaInIsr(bool x) {gestisciInterrupt = !x;}


        // ### Funzioni di manipolazione "semidiretta" del bus ###

        // Le seguenti funzioni devono sempre essere usate insieme, nell'ordine
        // in cui sono presentate qui (prepara - trasferisci - termina)

        // prepara il trasferimento di dati su SPI. Deve essere chiamato prima
        // di ogni chiamata a `leggiRegistro` o `scriviRegistro`
        void preparaTrasferimento();
        // Trasferisci un byte tramite SPI da Master a Slave o viceversa
        uint8_t trasferisciByte(uint8_t byte = 0);
        // termina il trasferimento di dati su SPI. Deve essere chiamato dopo
        // di ogni chiamata a `leggiRegistro` o `scriviRegistro`
        void terminaTrasferimento();


    private:

        // Impostazioni
        const uint8_t ss;
        const uint32_t frequenza;
        const BitOrder bitOrder;
        const DataMode dataMode;

        // stato del pin SS (il 10 su Arduino UNO), controllato a ogni trasferimento
        bool pinSSInput;

        // deve essere false quando SPI è usata in un'ISR
        bool gestisciInterrupt;

    };
    // `spi` è l'unica istanza della "classe interna" Spi di RFM69.
    Spi spi;

};




#endif
