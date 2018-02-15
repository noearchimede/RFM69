/*! @file
@brief Header della classe RFM69
*/


// NOTA PER LA DOCUMENTAZIONE DOXYGEN
//
// Per inserire la documentazione di questa classe all'interno di una documentazione
// Doxygen più ampia inserire il contenuto del file README.md come descrizione
// dettagliata della classe.



/*! @class    RFM69
    @brief    Driver per i moduli radio %RFM69

    Per una descrizione approfondita, cfr. la pagina @ref index
*/


// Per una documentazione dettagliata, cfr. il file README.md



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
