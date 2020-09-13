/*! @file
@brief Header della classe RFM69
*/


// NOTA PER LA DOCUMENTAZIONE DOXYGEN
//
// Per includere la documentazione di questa classe all'interno di una documentazione
// Doxygen più ampia inserire il contenuto del file README.md come descrizione
// dettagliata della classe.



/*! @class    RFM69
    @brief    Driver per i moduli radio %RFM69

    Per una descrizione approfondita, cfr. la pagina @ref index
*/




#ifndef RFM69_h
#define RFM69_h


#include <Arduino.h>

class RFM69 {

public:

    //! @name Test
    //!@{
    //! Test della connessione con il dispositivo
    /*! Questa funzione contatta la radio, richiede il codice della versione e
        lo restituisce. Il valore atteso è 0x24.
    */
    uint8_t testConnessione();
    //!@}


    //! @name Constructor, destructor ecc.
    //!@{

//private:
    class Bus; //serve al constructor
public:

    //! Constructor: richiede l'utilizzo di una delle funzioni sottostanti
    /*! @param pinInterrupt   Numero del pin attraverso il quale la radio genera
            un interrupt sul uC. Deve essere un pin di interrupt.
        @param pinReset       Numero del pin collegato al pin RESET della radio.
            0xFF significa che il pin di reset non è collegato.
    */
   RFM69(Bus* interfaccia, uint8_t pinInterurpt, uint8_t pinReset = 0xff);

   //! Helper per il constructor: usa l'interfaccia SPI
   /*! Questa funzione genera un oggetto della classe 'RFM69::Spi', che gestisce
    la comunicazione con la radio.
        @param pinSS Numero del pin Slave Select
   */
   static Bus* creaInterfacciaSpi(uint8_t pinSS);

   //! Helper per il constructor: usa l'interfaccia I2C tramite SC18IS602B
   /*! Questa funzione genera un oggetto della classe 'RFM69::SC18IS602B', che
    gestisce la comunicazione con la radio tramite il convertitore SPI - I2C
    SC18IS602B.
        @param indirizzo    Indirizzo I2C di SC18IS602B
        @param numeroSS     Numero del pin Slave Select di SC18IS602B utilizzato
            per la radio
   */
   static Bus* creaInterfacciaSC18IS602B(uint8_t indirizzoSC18, uint8_t numeroSS);


    //! Destructor
    /*! Dopo aver chiamato il destructor su un'istanza è possibile chiamare
    `inizializza()` su un'altra senza ricevere l'errore
    `Errore::ListaErrori::initTroppeRadio`
    */
    ~RFM69();
    //! Copy Constructor
    /*! Deleted perché non ha senso copiare una radio
    */
    RFM69(const RFM69&) = delete;
    //! Copy assignment Operator
    /*! Deleted perché non ha senso copiare una radio
    */
    RFM69& operator = (const RFM69&) = delete;

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

    //! Inizializza la radio e stampa il risultato (ok o errore...) sul monitor seriale.
    /*! Questa funzione chima `inizializza(uint8_t)` e `stampaErroreSerial()`:
        ~~~{.cpp}
        int RFM69::inizializza(int lunghezzaMaxMessaggio, HardwareSerial& serial) {
            serial.print(F("Inizializzazione RFM69... "));
            int errore = inizializza(lunghezzaMaxMessaggio);
            if(!errore) serial.println(F("ok"));
            else stampaErroreSerial(serial, errore);
            return errore;
        }
        ~~~

        @return La funzione restituisce comunque il codice di errore restituito da
                `inizializza(uint8_t)`
    */
    int inizializza(uint8_t lunghezzaMaxMessaggio, HardwareSerial& serial);

    //!@}
    /*! @name Funzioni fondamentali
    Devono essere usate in ogni programma per permettere una comunicazione radio
    */
    //!@{
    
    
    //! Da chiamare regolarmente! Aggiorna lo stato, scarica in nuovi messaggi ecc.
    /*! Questa funzione dipende dall'ISR
    */
    int controlla();


    //! Invia un messaggio
    /*! @note Spesso nella documentazione ci sono riferimenti a questa funzione.
        A volte tali riferimenti non sono in realtà a questa funzione ma al gruppo
        delle funzioni invia, cioé `invia()`, `inviaConAck()` e `inviaFinoAck()`.

        Questa funzione prepara il modulo radio per trasmettere un messaggio. La
        trasmissione inizierà alla fine di questa funzione e sarà terminata
        dall'isr().

        Cfr. la descrizione generale della classe @ref RFM69 per informazioni
        su quando questa funzione può o non può essere usata.

        @param messaggio[in]  array di bytes (`uint8_t`) che costituiscono il messaggio
        @param lunghezza[in]  lunghezza del messaggio in bytes
        @param titolo   [in]  cfr. il commento alla funzione `titoloMessaggio()`

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int invia(const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo = 0);

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
        @param titolo   [in]  cfr. il commento alla funzione `titoloMessaggio()`

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int inviaConAck(const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo = 0);

    //! Invia un messaggio ripetutamente fino alla ricezione di un ACK
    /*! Questa funzione invia un messaggio, aspetta l'ACK e se non lo riceve entro
        il tesmpo specificato in `impostaTimeoutAck()` lo invia di nuovo. Ripete
        questa operazione per al massimo `tentativi` volte, dopo le quali
        restituirà 1 (`Errore::ListaErrori::errore`) se il messaggio non è ancora
        stato confermato (quindi probabilmente non è arrivato). Restituirà invece
        0 (`Errore::ListaErrori::ok`)subito dopo aver ricvevuto un ACK.

        Dopo l'esecuzione della funzione `tentativi` conterrà il numero di tentativi
        effettuati.

        @param tentativi[in/out]  /b prima: numero di tentativi da effettuare prima
                                  di rinunciare alla trasmissione del messaggio
                                  /b dopo: numero di tentativi effettuati
        @param messaggio[in]      array di bytes (`uint8_t`) che costituiscono il messaggio
        @param lunghezza[in]      lunghezza del messaggio in bytes
        @param titolo   [in]  cfr. il commento alla funzione `titoloMessaggio()`

        @return Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    int inviaFinoAck(uint16_t& tentativi, const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo = 0);

    //! Versione di `inviaFinoAck` senza restituzione del numero di tentativi
    /*! Questa funzione corrisponde ad `inviaFinoAck(uint16_t&, const uin8_t, uint8_t, uint8_t)`
        ma il suo primo argomento non è una 'reference'; può essere usata se non
        si è interessati a sapere il numero di tentativi effettuati oppure
        se si passa come argomentoun valore e non una variabile.
    */
    int inviaFinoAck(const uint16_t &&tentativi, const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo = 0) {uint16_t t = tentativi; return inviaFinoAck(t, &messaggio[0], lunghezza, titolo);}

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
    /*! Per riceve la radio deve essere in modalità ricezione. Nelle altre
        modalità non si accorgerà nemmeno di aver ricevuto un messaggio. Quindi
        se non si chiama questa funzione sulla radio ricevente tutti i messaggi
        inviatile andranno persi.
        @note Questa funzione non garantisce che la modalità sia già rx al proprio
            termine, perché se la radio è occupata delega il cambiamento di modalità
            alla funzione `controlla()` che imposterà rx non appena possibile.
            @param aspetta Blocca il programma fino a che la radioella modalità
            desiderata. Pue utile se per qualche ragione la funz eccezionaleione
            'controlla()' non sta girando al momento della chiamata.
    */
    void ricevi(bool aspetta = false);


    //!@}
    /*! @name Funzioni importanti
    Probabilmente saranno chiamate in tutti i programmi, ma non sono strettamente
    indispensabili
    */
    //!@{


    //! Mette la radio in standby (richiederà 1.25mA di corrente)
    /*! La modalità `standby` serve per mettere la radio in pausa per qualche
        secondo. Per pause più lunghe conviene usare `sleep()`, soprattutto se
        l'intero dispositivo deve essere messo in standby per un certo periodo.
        Se invece il dispositivo utilizza relativamente tanta corrente durante
        lo standby della radio (ad es. ha accesi diversi LED, un motore, ...) la
        differenza tra `sleep()` e `standby()` non è rilevante.
        @note vedi nota per @ref `ricevi()`
        @param aspetta Blocca il programma fino a che la radio è nella modalità
            desiderata. Può essere utile se per qualche ragione eccezionale la
            funzione 'controlla()' non sta girando al momento della chiamata.
    */
    void standby(bool aspetta = false);


    //! Controlla se c'è un nuovo messaggio
    /*! @return `true` se il buffer della classe contiene un nuovo messaggio.
    */
    bool nuovoMessaggio();

    //! Restituisce la dimensione dell'ultimo mesasggio
    /*! @return la dimensione dell'utlimo messaggio ricevuto in bytes
    */
    uint8_t dimensioneMessaggio();

    //! Restituisce il titolo dell'ultimo messaggio
    /*! Il titolo di un messaggio è un numero compreso tra 1 e 64 scritto
        nell'intestazione dall'utente (con il parametro `titolo` di `invia()`).
        La classe si limita a inviarlo e renderlo disponibile prima della lettura
        del messaggio tramite questa funzione, non lo utilizza. L'utente può
        usarlo per scartare subito messaggi non interessanti e dare grande importanza
        ad altri, oppure semplicemente per creare più funzioni di lettura dei
        messaggi, ognuna specializzata in un particolare tipo di messaggio, e
        chiamare subito quella giusta.

        @return il titolo dell'ultimo messaggio
    */
    uint8_t titoloMessaggio();

    //! Restituisce true se la classe sta aspettando un ACK
    /*! @return `true` se e solo se la classe sta aspettando un ack, cioé se ha
            inviato un messaggio con richiesta di ACK e non lo ha ancora ricevuto
            oppure non ha ancora verificato se un messaggio che dovrebbe esesere
            un ack lo è davvero.
        @note Chiama internamente `controlla()`, quindi è possibile scrivere
            `while(ackInSospeso());` per aspettare la ricezione di un Ack o lo
            scadere del tempo `timeoutAck()`.
    */
    bool ackInSospeso();

    //! Restituisce true se la radio ha ricevuto un ACK
    /*! @return `true` se la radio ha ricevuto un ACK per l'ultimo messaggio inviato

        Questa funzione non aspetta l'ACK, dice solo se è arrivato, quindi chiamata
        immediatamente dopo `invia()` restituisce sempre `false`. Normalmente è
        perciò usata insieme a ackInSospeso() (cioé subito dopo di essa).

        Esempio:
        ~~~{.cpp}
        // prova per tre volte al massimo di inviare un messaggio
        for(int i = 0; i < 3; i++) {
            invia(...);
            while(ackInSospeso());
            if(ricevutoAck()) break;
        }
        // se anche il terzo tentativo fallisce stampa un messaggio
        if(!ricevutoAck()) {
            print("Trasmissione messaggio fallita");
        }
        ~~~
    */
    bool ricevutoAck();


    //!@}
    /*! @name Funzioni di impostazione
    Impostazioni nel file di impostazione che possono essere modificate anche nel
    programma
    */
    //!@{

    //! Imposta la bit rate di trasmissione
    /*! @note Con la bit rate conviene modificare anche la Frequency Deviation,
              se si utilizza la modulazione FSK (default per questa classe).
              Vedi il commento a `impostaBitRate()` per più dettagli.

        @warning La bit rate deve essere identica per entrambe le radio

        @param BitRate bit rate in bit al secondo, compresa tra 1'200 e 300'000
               per la modulazione FSK e tra 1'200 e 32'768 per OOK

        @return Errore secondo l'`enum` `Errore::ListaErrori` (1 se c'è un errore
                e 0 se tutto va bene).
    */
    int impostaBitRate(uint32_t bitRate);

    //! Imposta la Frequency Deviation per la modulazione FSK
    /*! Con la Frequency Deviation conviene modificare anche la bit rate.
        Si tenga sempre presente che:

        - (1/5) * bitRate < freqDev < 5 * bitRate
        - 600 < freqDev < 500'000 - (bitRate / 2)

        Si potrebbero applicare molte altre forumle, che nn sono riportate qui
        (vedi ad ed. il teorema di Shannon–Hartley).

        @warning La Frequency Deviation deve essere identica per entrambe le radio

        @note Questa impostazione non ha alcun effetto se la modulazione scelta
              nel file RFM69_impostazioni.h è OOK

        @param freqDev Frequency deviation, in Hertz, compresa tra 600 e 300'000

        @return Errore secondo l'`enum` `Errore::ListaErrori` (1 se c'è un errore
                e 0 se tutto va bene).
    */
    int impostaFreqDev(uint32_t freqDev);

    //! Imposta la bandwidth del channel filter
    /*!
    */
    //! Imposta la frequenza di comunicazione
    /*! Imposta la frequenza della trasmissione radio.
        @note La funzione non impone limiti al valore ricevuto. Esistono però due
              limiti:

              - Il limite del modulo, che esiste in 4 varianti diverse con ciascuna
                una banda di frequenze possibili diversa;
              - La legge, che regola l'utilizzo dello spettro elettromagnetico
                lasciando alcune bande ben definite a moduli a bassa potenza come
                questo.

        @param freq La frequenza di trasmissione in Hertz

        @return Errore secondo l'`enum` `Errore::ListaErrori` (1 se c'è un errore
                e 0 se tutto va bene).
    */
    int impostaFrequenzaMHz(uint32_t freq);


    //!@}
    /*! @name Funzioni ausiliarie
    Utili ma non indispensabili
    */
    //!@{

    //! Mette la radio in modalità `listen`
    /*! `listen` è una modalità particolare che consiste in realtà nella continua
        alternanza tra due modalità: `rx` (ricezione) e `idle` (una specie di
        sleep adattato alla modalità listen).
        @note vedi nota per @ref `ricevi()`
        @param aspetta Blocca il programma fino a che la radioella modalità
            desiderata. Pue utile se per qualche ragione la funz eccezionaleione
            'controlla()' non sta girando al momento della chiamata.
    */
    void listen(bool aspetta = false);

    //! Mette la radio in modalità sleep (richiederà 0.1uA di corrente)
    /*! @ref standby()
        @param aspetta Blocca il programma fino a che la radioella modalità
            desiderata. Pue utile se per qualche ragione la funz eccezionaleione
            'controlla()' non sta girando al momento della chiamata.
    */
    void sleep(bool aspetta = false);

    //! Imposta il tempo d'attesa massimo per un ACK
    /*! @param tempoMs Tempo di attesa in millisecondi per la funzione `ackInSospeso()`.
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

    //! Imposta la potenza di trasmissione
    /*! Imposta la potenza di trasmissione della radio. Sono validi i valori interi
        compresi tra -2 e +20 dBm.

        I valori non sono accurati, ma la relazione tra di essi è coerente.

        @note Alla potenza massima, +20dBm, il segnale può in realtà rivelarsi
              meno forte o più disturbato che con una potenza di +19.

        @param Valore di potenza assoluta di trasmissione desiderato in dBm [-2; +20]

        @param Codice di errore definito nell'enum RFM69::Errore::ListaErrori
    */
    bool impostaPotenzaTx(int dBm);

    //! Controlla se c'è una trasmissione in corso
    /*! Questa funzione può essere utile se vicino alla radio si trova un dispsitivo
        sensibile alle onde elettromagnetiche della frequenza emessa dalla radio
        (ad es. un sensore che non è accurato in caso di forte disturbo attorno ai
        433Mhz, nel caso il modulo radio usato sia europeo, potrebbe usare questa
        funzione per rimandare la propria misurazione a dopo la trasmissione del
        messaggio).

        @return `true` se la radio sta trasmettendo un messaggio.
    */
    bool staTrasmettendo() {return (statoOld.trasmissioneMessaggio || statoOld.trasmissioneAck);}

    //!@}
    /*! @name Log
    Funzioni utili per monitorare il funzionamento della radio
    */
    //!@{

    //! Restituisce la durata dell'ultima attesa di un ACK (ricevuto)
    /*! Il valore restituito da questa funzione è il tempo in millisecondi impiegato
        per svolgere le operazioni elencate di seguito l'ultima volta che sono state
        eseguite _tutte_; se il messaggio non richiedeva ACK o comunque l'ACK non è
        arrivato questa funzione darà il tempo relativo a un messaggio precedente che
        ha avuto il suo ACK:

        `[attesa ACK] = [trasmissione] + [decodificazione] + [TEMPO PRIMA DELLA CHIAMATA
        ALLA FUNZIONE leggi()] + [trasmissione ACK]`

        Il `[TEMPO PRIMA DELLA CHIAMATA ALLA FUNZIONE leggi()]` è il fattore più
        variabile; è più basso se il codice della radio ricevente è scritto bene
        (dal punto di vista di questa classe).
    */
    uint16_t ottieniAttesaAck() {return durataUltimaAttesaAck;}

    //! Restituisce la durata massima di attesa di un ACK (ricevuto)
    /*! Restituisce lo stesso valore della funzione `durataAttesaAck()`, ma relativo
        all'attesa più lunga dall'ultima inizializzazione al momento della chiamata.

        Può essere usato per definire un tempo timeoutAck adeguato: chiamata dopo
        aver eseguito il programma finale con un timeout arbitrariamente grande,
        questa funzione restituirà il valore ideale del timeout: se ad es. restituisse
        100 un timeout adeguato potrebbe essere 110 o 120. Se però la media
        ottieniAttesaMediaAck() fosse nettamente inferiore (ad es. 50) converrebbe
        probabilmente identificare il punto in cui il tempo di attesa è massimo
        usando la funzione ottieniAttesaAck() e eliminare la causa di tale ritardo
        oppure aumentare momrntaneamente il timeout il quel punto.
    */
    uint16_t ottieniAttesaMassimaAck() {return durataMassimaAttesaAck;}

    //! Restituisce la durata media di attesa di un ACK (ricevuto)
    /*! Restituisce lo stesso valore della funzione `durataAttesaAck()`, ma anziché
        riferirsi a un'attesa in particolare restituisce l'attesa media dall'ultima
        inizializzazione al momento della chiamata.

        Può essere un indice della buona scrittura del codice della radio ricevente:
        più la funzione `leggi()` è chiamata (o pronta ad essere chiamata) regolarmente
        e frequentemente più questo valore sarà basso.
    */
    uint16_t ottieniAttesaMediaAck() {return (sommaAtteseAck/nrAckRicevuti);}

    //! Restituisce il numero di messaggi inviati dopo l'ultima inizializzazione
    /*! @return Il numero di messaggi inviati dopo l'ultima inizializzazione
    */
    uint16_t nrMessaggiInviati() {return messaggiInviati;}
    //! Restituisce il numero di messaggi ricevuti dopo l'ultima inizializzazione
    /*! @return Il numero di messaggi ricevuti dopo l'ultima inizializzazione
    */
    uint16_t nrMessaggiRicevuti() {return messaggiRicevuti;}


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
        @param errore codice di errore restituito da una delle funzioni sopra elencate.
        @param contesto se `true` stampa il nome della classe e della funzione prima
               dell'errore, ad es.:

               - `contesto == true` -> RFM69: init: nessuna radio connessa
               - `contesto == false` -> nessuna radio connessa
    */
    void stampaErroreSerial(HardwareSerial &serial, int errore, bool contesto = true);


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
            (`ricevi()`, `standby()`, ...)

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
            /*! inviaMessaggio(): invia() è stata chiamata mentre la classe
            stava eseguendo un'altra operazione, e quest'ultima non è stata
            completata entro il tempo d'attesa massimo scelto per questa
            situazione oppure l'opzione 'insisti' non era selezionata
            (equivalente a un timeout nullo).
            */
            inviaTimeout                = 9,


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

            /*! controlla(): Registrato un timeout per l'invio di un messaggio
            (la radio viene sbloccata automaticamente, ma non dovrebbe mai succedere)
            */
            controllaTimeoutTx          = 15
        };
    };

//private:

    // All'inizio vale 0 (come tutte le variabili `static`); ogni volta che
    // il constructor di questa classe viene chiamato il suo valore aumenta di
    // uno. In questo modo si scopre se esiste più di un'istanza di questa classe,
    // che per funzionare può averne una sola a causa dell'isr, che è una sola.
    // Per consentire l'utilizzo di più radio si può creare una nuova isr per
    // ogni radio e collegare un'isr diversa a ogni istanza oppure gestire in
    // qualche modo l'utilizzo comune di un'unica isr (ad esempio con funzioni
    // "on" ed "off" per ogni radio). Il numero massimo di isr è limitato dal
    // numero di interrupt possibili.
    static uint8_t nrIstanze;



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
            uint8_t titolo: 6;
        } bit;          // scrittura e lettura
        uint8_t byte;   // trasmissione

        Intestazione() : byte(0) {}
    };
    // Valore massimo nel field Intestazione::bit::titolo (dipende dalla sua dimensione)
    const uint8_t valMaxTitolo = 64;

    // Struct per salvare informazioni sui messaggi ricecvuti
    struct InfoMessaggio {
        uint8_t dimensione;
        Intestazione intestazione;
        uint32_t tempoRicezione;
    };


    // ### GESTIONE COMUNICAZIONI ###

    // [privata] Funzione di invio dei messaggi
    /* Questa funzione è alla base di tutte le funzioni di invio messaggi, che
        si limitano a scrivere l'intestazione del messaggio prima di chiamarla.
        Le funzioni pubbliche di invio sono:
        - `inviaConAck()`
        - `inviaFinoAck()`
        - `invia()`
        @note l'opzione `insisti` non è attualmente utilizzata, cioé nessuna
        funzione a disposizione dell'utente la usa ed è quindi sempre `true`
    */
    int inviaMessaggio(const uint8_t messaggio[], uint8_t lunghezza,
                    uint8_t intestazione, bool insisti = true);

    // Imposta la modalità di funzionamento
    /* Cambia la modalità della radio. Questa funzione è chiamata per ogni
        cambiamento di modalità, richiesto dall'utente direttamente
        (`ricevi();`, `standby();`, ...), indirettamente (`invia()`;, ...)
        o dall'`isr()`. Se richiesta dall'utente può bloccare il programma per
        qualche millisecondo, soprattutto in caso di cambiamento verso la modalità
        listen, o da o verso la modalità trasmissione, mentre è molto rapida se
        chiamata nell'`isr()`.
    */
    int cambiaModalita(Modalita, bool aspetta = true);

    void modalitaStandby();

    // Aspetta che la 

    enum class AMEnterCond : uint8_t {
        //none = 0x0 non può essere usata (bisogna impostare sia enter sia exit)
        fifoNotEmptyRising  = 0x1, // valori validi per il registro corrispondente
        fifoLevelRising     = 0x2,
        crcOkRising         = 0x3,
        payloadReadyRising  = 0x4,
        syncAddressRising   = 0x05,
        packetSentRising    = 0x06, 
        fifoNotEmptyFalling = 0x07
    };
    enum class AMExitCond : uint8_t {
        //none = 0x0 non può essere usata (bisogna impostare sia enter sia exit)
        fifoNotEmptyFalling = 0x1, // valori validi per il registro corrispondente
        fifoLevelRising     = 0x2,
        crcOkRising         = 0x3,
        payloadReadyRising  = 0x4,
        syncAddressRising   = 0x05,
        packetSentRising    = 0x06, 
        timeoutRising       = 0x07
    };
    enum class AMModInter : uint8_t { // non tutte le modalità sono possibili
        sleep   = 0x0, // valori validi per il registro corrispondente
        standby = 0x1,
        rx      = 0x2,
        tx      = 0x3
    };

    // Imposta la funzione AutoModes (-> p 42 datasheet)
    // nota: funzione relativamente lunga, fino a (stima) 5ms a causa della
    // chiamata a cambiamodalita() in con l'opzione 'aspetta'
    void autoModes(Modalita modBase, AMModInter modInter,
                    AMEnterCond enterCond, AMExitCond exitCond);

    void disattivaAutoModes();


    // Scrive le impostazioni "high power" (per l'utilizzo del modulo con una potenza
    void highPowerSettings(bool attiva);
    // Invia un ACK
    void inviaAck();

    // # ISR #
    // ISR che reagisce ai segnali di interrupt della radio chiamando l'interrupt
    // handler `isr()`
    static void isrCaller();

    // Interrupt handler
    /* Funzione chiamata quando la radio richiede un interrupt tramite il proprio
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

    // Modalità usata quando non ne è specificata un'altra per eseguire un'azione
    // particolare
    Modalita modalitaDefault;

    // Dimensione massima dei messaggi in entrata
    // costante dopo l'inizializzazione, può essere modificato da un'init. successiva
    uint8_t lungMaxMessEntrata;

    // mantieni una copia di regOpMode localmente perché serve spesso
    uint8_t regOpMode;


    // Pin
    const uint8_t pinReset; //non usato se `haReset == false`
    // Numero dell'external interrupt usato dalla radio
    const uint8_t numeroInterrupt;
    // Il microcontrollore può controllare il pin Reset della radio
    const bool haReset;


    // ## Uguali per tutte le radio nella rete ##

    // La radio è in modalità high power e richiede impostazioni particolari
    // a ogni trasmissione.
    bool highPower;


    // ### VARIABILI ###

    // controllo ISR (serve sia per debug che per le statistiche sugli ACK qui sotto)
    uint32_t tempoUltimaEsecuzioneIsr; // in ms

    // # Timeout #
    // Tempo massimo di attesa dell'ACK (deve essere scelto in base alla frequenza
    // con cui viene chiamata la funzione `leggi()` sull'altra radio;
    // 250 è un valore arbitrario.
    uint16_t timeoutAck = 250;
    // Tempo massimo per aspettare che la radio si liberi prima di inviare un
    // messaggio cambiare modalità ecc. 30 ms sembra essere un ragionevole tempoo
    // massimo per inviare un messaggio della lunghezza massima
    // TODO sostituire questa attesa con un sistema che rimandi l'azione in questione
    // a quando la radio sarà libera sfruttando la funzione controlla(
    uint8_t timeoutAspetta = 30;

    // # Statistiche Ack # 

    // Durata dell'ultima attesa di un ACK (se timoeuut, =~ tiemoutAck).
    // Calcolata solo se l'ACK è arrivato ed è stato verificato
    // [attesa ACK] = [trasmissione] + [decodificazione] + [TEMPO PRIMA DELLA CHIAMATA
    // ALLA FUNZIONE leggi()] + [trasmissione ACK]
    // Questa variabile ha un 'getter' pubblico
    uint16_t durataUltimaAttesaAck;
    // Durata massima dell'attesa di un ACK dall'ultima inizializzazione ad ora
    // Questa variabile ha un 'getter' pubblico perché può essere usata per impostare
    // `timeoutAck`
    uint16_t durataMassimaAttesaAck;
    // Durata media dell attesa di un ACK dopo l'ultima inizializzazione
    // La media ha un 'getter' pubblico perché può essere un indice della
    // buona formattazionje del codice della radio ricevente.
    uint32_t sommaAtteseAck;
    // Numero di ACK ricevuti, serve nel calcolo della media
    uint16_t nrAckRicevuti;
    // Valore RSSI più recente disponibile
    int8_t ultimoRssi;
    // l'ultimo ack richiesto è stato ricevuto, non ricevuto, oppure si sta
    // la radio lo sta aspettando o sta aspettando che un possibile ack sia
    // verificato
    enum class StatoAck {pendente, attesaVerifica, ricevuto, nonRicevuto} statoUltimoAck;


    // Modalità in cui si trova attualmente la radio
    volatile Modalita modalita = Modalita::sleep;
    // "ora" di trasmissione dell'ultimo messaggio (ms)
    volatile uint32_t tempoUltimaTrasmissione = 0;
    // Informazioni sull'ultimo messaggio ricevuto
    volatile InfoMessaggio ultimoMessaggio;
    
    struct StatoOld {
        StatoOld() : trasmissioneMessaggio(0), trasmissioneAck(0),
                  attesaAck(0), ackRicevuto(0), messaggioRicevuto(0) {}

        // La radio sta trasmettendo un messaggio
        volatile bool trasmissioneMessaggio : 1;
        // La radio sta trasmettendo un ACK
        volatile bool trasmissioneAck : 1;
        // Un ACK richiesto non è ancora stato ricevuto
        volatile bool attesaAck : 1;
        // È stato ricevuto un ACK per l'ultimo messaggio inviato
        volatile bool ackRicevuto : 1;
        // Nella radio c'è un nuovo messaggio da leggere
        volatile bool messaggioRicevuto : 1;
    } statoOld;


    enum class Stato {
        // nessuna azione in corso, tranne eventualmente ascolto passivo (rx, listen)
        pronto,
        // questo è uno stato di transizione possibile tra un'esecuzione
        // dell'ISR e la successiva chiamata di controlla(). Per gli interventi
        // in questione vedi poco sotto.
        attesaConcludiAzione,
        // La radio sta trasmettendo un messaggio con richiesta di ack
        invioMessConAck,
        // Sta trasmettendo un messaggio senza richesta di Ack
        invioMessSenzaAck,
        // Sta aspettando un ack
        attesaAck

    };
    volatile Stato stato = Stato::pronto;

    // Restituisce true se la radio è pronta per eseguire un novo compito
    // (inviare, cambiare modalità, ...) Rispetto a un semplice controllo della
    // variabile `stato` questa funzione chiama 'controlla()' se necessario e
    // offre la possiblità di aspettare fino a `timeoutAspetta` ms che la radio
    // sia pronta
    bool radioPronta(bool aspetta);



    // l'ISR imposta queste variabili, la funzione controlla() esegue le azioni
    // richieste
    struct Intervento {
        // AutoModes ha eseguito la transizione di cui è incaricato, non serve più
        bool disattivaAutoModes;
        // è arrivato un messaggio, scaricalo nella memoria interna per liberare la radio
        bool scaricaMesasggio;
        // è arrivato un mesasggio che dovrebbe essere un ACK, verifica se lo è davvero
        bool verificaAck;

    };
    // NOTA: dopo l'intervento in concludiAzione l'azione in corso è conclusa.
    // Per implementare un'azione intermedia bisognerebbe usare un'altra
    // variabile Intervento
    volatile Intervento concludiAzione {};

    // piccoli helper
    inline void set(volatile bool& x) { x = true; }
    inline void clear(volatile bool& x) { x = false; }

    class Buffer {
        typedef uint8_t data_type;
        data_type * dataptr = nullptr;
        uint8_t len = 0;
    public:
        // la dimensione non è nota al momento dela costruzione di RFM69
        Buffer() = default;
        void init(uint8_t dimensione) {
            if(dataptr != nullptr) delete[] dataptr;
            dataptr = new data_type[dimensione];
            len = dimensione; }
        ~Buffer() { delete[] dataptr; }
        // non copiabile (come RFM69)
        Buffer(const Buffer&) = delete;
        Buffer& operator = (const Buffer&) = delete;
        // operatori di accesso
        data_type operator [] (unsigned int i) {
            return i < len ? *(dataptr + i) : 0; }
        operator data_type*() { return dataptr; }
    } buffer;


    // totale di messaggi inviati dall'ultima inizializzazione
    volatile uint16_t messaggiInviati;
    // totale di messaggi ricevuti dall'ultima inizializzazione
    volatile uint16_t messaggiRicevuti;

    // Numero di ACK ricevuti mentre `attesaAck == false`
    uint16_t ackInattesi = 0;
    // Numero di trasmissioni fallite per timeout
    uint16_t trasmissioniFalliteTimeout = 0;


    // ### Comunicazione con la radio ###

    // Bus di comunicazine generico, specializzato sotto
    class Bus {
    
    public:

        // destructor
        virtual ~Bus() {}

        // funzione eseguita una sola volta all'inizio del programma
        virtual bool inizializza() = 0;

        // leggi un registro della radio (un byte)
        virtual uint8_t leggiRegistro(uint8_t addr) = 0;
        // scrivi un byte in un registo della radio
        virtual void scriviRegistro(uint8_t addr, uint8_t val) = 0;

        // leggi una sequenza di len bytes a partire da addr0 e salvali in data
        virtual void leggiSequenza(uint8_t addr0, uint8_t len, uint8_t* data) = 0;

    };

    // ### SPI ###

    // Questa classe è scritta per interagire con la radio RFM69, non con
    // un dispositivo qualsiasi, anche se probabilmente si adatterebbe molto
    // bene anche a questo scopo.
    // Il comportamento in caso di utilizzo di SPI anche da parte di altre parti
    // del programma non è stato testato, ma in teoria dovrebbe essere corretto
    // (questa classe scvrive nei registri di SPI le proprie impostazioni prima
    // ogni trasferimeto di dati).
    //
    class Spi : public Bus {

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
        Spi(uint8_t pinSS, uint32_t frequenzaHz, BitOrder bitOrder, DataMode dataMode);


        // ### Implementazione delle funzioni virtuali di Bus
        
        bool inizializza() override;

        uint8_t leggiRegistro(uint8_t addr) override;
        void scriviRegistro(uint8_t addr, uint8_t val) override;

        void leggiSequenza(uint8_t addr0, uint8_t len, uint8_t* data) override;


    private:

        void apriComunicazione();
        uint8_t trasferisciByte(uint8_t byte = 0);
        void chiudiComunicazione();

        // Impostazioni
        const uint8_t ss;
        // calcolati in base a un input "leggibile"
        uint8_t spcr;
        uint8_t spsr;

        // L'utente desidera usare il pin SS (10 su Arduino UNO) come input
        // altrove nel programma
        bool pinSSInput;

    };


    // ### I2C (tramite SC18IS602B) ###

    class SC18IS602B : public Bus {
    
    public:

        SC18IS602B(uint8_t indirizzo, uint8_t numerSS);

        // ### Implementazione delle funzioni virtuali di Bus
        
        bool inizializza() override;

        uint8_t leggiRegistro(uint8_t addr) override;
        void scriviRegistro(uint8_t addr, uint8_t val) override;

        void leggiSequenza(uint8_t addr0, uint8_t len, uint8_t* data) override;

    private:

        // funzioni per leggere e scrivere il Data Buffer di SC18IS602B
        //  byte1: primo byte da inviare (tipicamente function ID di SC18)
        //  byte2: secondo byte (tipicamente indirizzo di un registro della radio)
        //  nrAltriByte: numero di byte da inviare oltre ai primi due.
        //  altriByte: il resto dei byte da inviare. Niente per inviare zeri.
        void sc18_inviaDati(uint8_t byte1, uint8_t byte2, uint8_t nrAltriByte,
                            uint8_t* altriByte = nullptr);
        void sc18_richiediDati(uint8_t dataLen, uint8_t * data);

        // indirizzo I2C di SC18IS602B
        const uint8_t indirizzo;
        // pin Chip Select di SC18IS602B usato per la radio (1-4)
        const uint8_t codiceCS;

    };

    // Istanza della classe che gestisce la comunicazione con il chip
    // (allocata dinamicamente nel constructor)
    Bus* bus;


};




#endif




/*! @todo
Si potrebbe aggiungere la possibilità di chiamare una funzione sull'altra radio
in situazioni di urgenza, con due funzioni e un adattamento dell'ISR:
/ /! Collega una funzione da eseguire immediatamente su richiestsa dell'altra radio
/ *! Ogni radio può chiedere all'altra di eseguire una di al massimo tre
    funzioni immediatamente dopo la ricezione del messaggio contentente la
    richiesta (cioé all'interno dell'ISR).

    @warning Le funzioni collegate saranno eseguite all'interno di un'Interrupt
    Service Routine (ISR). Devono perciò:

    - essere il più breve possibile (non contenere `delay()`s o funzioni lunghe);
    - non aver bisogno di altri interrupt (ad es. il valore di `millis()` e
      `micros()` non incrementa in un'ISR);
    - non attivare gli interrupt (cioè non chiamare `sei()`).

    È meglio usare queste funzioni solo in caso di urgenza, e nella maggior
    parte dei casi gestire la chiamata a funzioni sull'altra radio con messaggi
    normali (ad es. con uno `switch` che in base a un valore contenuto nel
    messaggio chiama una funzione diversa).

    @note La funzione deve essere della forma `void nome(void)`

    @param fz Una funzione
    @param nr Il numero con il quale essa sarà chiamata dall'altra radio
* /
bool collegaFunzioneISR(void (*fz)(), uint8_t nr);

/ /! Chiama una funzione sull'altra radio
/ *! La funzione selezionata sarà eseguinta nell'ISR <b>dall'altra radio</b>
    non appena questa riceverà il messaggio.
    cfr. `collegaFunzioneISR()` per informazioni sulla funzione chiamata.

    Questa funzione farà un tentativo di trasmissione ogni 20 ms fino alla ricezione
    di un ACK o allo scadere del tempo massimo specificato.

    @param nr       Numero della funzione da eseguire
    @param timeout  Tempo durante il quale la funzione proverà a trasmettere
                    la sua richiesta di esecuzione all'altra radio
    @return Codice errore come per le funzioni `invia()`
* /
int eseguiFunzioneRemota(uint8_t nr, uint16_t timeout);



Impostare bit rate in una funzione; tenere presente che:
 - fdev + (bitRate/2) <= 500kHz
 - 0,5 <= b <= 10 dove b = 2*fDev/bitRate
 - fDev > fRF * crystalTolerance (es.: 433*50 se freq = 433MHz e cry.tol = 50 ppm)

 - rxBw > 2*Fdev + bitRate + (fRF*cryTolRx + fRF*cyTolTx)(1)
    dove (1) vale, per ad es. cryTol = 50 ppm e freq 433MHz, (433*50*2)
*/
