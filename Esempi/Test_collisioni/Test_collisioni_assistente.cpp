/*! @file
@brief Test collisioni tra messaggi

Questo programma permette di stimare il numero di messaggi persi in un canale di
comunicazione tra due radio molto utilizzato (centinaia di messaggi al minuto).

Le due radio svolgono lo stesso compito: inviano messaggi a intervalli casuali,
che tendono però ad una frequenza determinata, e controllano se ci sono messaggi
da leggere. Tutti i messaggi trasmessi richiedono un ACK.
Questo programma, dato un certo numero di messaggi al minuto, stabilisce la
percentuale di messaggi persi. Trovata la percentuale per una certa frequenza di
trasmissione aumenta questo ultimo valore ed esegue lo stesso calcolo. Prosegue
così fino a che le due radio emettono senza sosta e non possono quindi ricevere
i messaggi dell'altra e trasmettere un ACK in risposta. A quel punto stampa una
tabella riassuntiva.

Questo programma è per la radio "assistente". Essa si comporta come l'altra ma
non genera statistiche. Non è quindi necessario collegarla a un monitor seriale.

*/

#include <Arduino.h>
#include "RFM69.h"


//**************************  +--------------+  ********************************
//**************************  | IMPOSTAZIONI |  ********************************
//**************************  +--------------+  ********************************

// Questo esempio può essere compilato direttamente oppure importato ('#include')
// da un altro file. Nel primo caso usare le impostazioni qui sotto, nel secondo
// impostare la costante 'IMPOSTAZIONI_ESTERNE' e definire tutte le costanti
// di impostazioni nell'altro file prima di importare questo.
// È anche possibile definire 'DEFINISCI_FUNZIONE_ESEGUI_TEST' per far si che 
// invece di eseguire il test questo file lo "impacchetti" in una funzione chiamata
// 'eseguiTest()'.

#ifndef IMPOSTAZIONI_ESTERNE

//*** interfaccia di comunicazione con la radio (definire solo uno dei due!) ***
#define INTERFACCIA_SPI
//#define INTERFACCIA_SC18IS602B

//*** pin comunicazione ***
#define PIN_SS 2  // solo per SPI
//#define NUMERO_SS 1 // solo per SC18IS602B
//#define INDIRIZZO_I2C 0x20 // solo per SC18IS602B

//*** pin connesso al pin DIO0 della radio ***
#define PIN_INTERRUPT 3

//*** pin per i LED ***
#define LED_TX 8
#define LED_RX 7

//*** lunghezza dei messaggi usati nel test ***
// NOTA: deve essere almeno 2!
#define LUNGHEZZA_MESSAGGI 4

//*** tempo massimo per aspettare un ACK ***
#define TIMEOUT_ACK 100

#endif


//------------------------------------------------------------------------------
// ## Impostszioni che devono essere identiche sulle due radio ## //
//------------------------------------------------------------------------------

// Lunghezza in bytes del contenuto dei messaggi. La lunghezza effettiva sarà
// 4 bytes in più (lunghezza [1 byte], intestazione [1], contenuto [...], crc [2]);
// a questo si aggiunge il preamble che è lungo per default 4 bytes
const uint8_t lunghezzaMessaggi = LUNGHEZZA_MESSAGGI;
// Tempo massimo di attesa per un ACK
const uint8_t timeoutAck = TIMEOUT_ACK;


//******************************************************************************
//******************************************************************************



// ### Variabili e costanti globali ### //

uint32_t tAccensioneRx, tAccensioneTx;
uint32_t microsInviaPrec = 0;
uint32_t messPerMin = 0;
uint32_t tUltimoMessaggio = 0;


// ### Prototipi ### //

void funzioneSetup();
void funzioneLoop();
void invia();
void leggi();
void accendiLed(uint8_t);
void spegniLed();
void pausa();
void fineProgramma();

// ### Instanza della classe Radio ### //

#if defined(INTERFACCIA_SPI)
RFM69 radio(RFM69::creaInterfacciaSpi(PIN_SS), PIN_INTERRUPT);
#elif defined(INTERFACCIA_SC18IS602B)
RFM69 radio(RFM69::creaInterfacciaSC18IS602B(INDIRIZZO_I2C, NUMERO_SS), PIN_INTERRUPT);
#endif

// ### Esecuzione programma ### //

#ifndef DEFINISCI_FUNZIONE_ESEGUI_TEST
void setup() {
    funzioneSetup();
}
void loop() {
    funzioneLoop();
}
#else
void eseguiTest() {
    funzioneSetup();
    while(true) funzioneLoop();
}
#endif


////////////////////////////////////////////////////////////////////////////////
// Inizializzazione
////////////////////////////////////////////////////////////////////////////////


void funzioneSetup() {

    Serial.begin(115200);

    Serial.println("\n\n\n\nRFM69 - Test collisione messaggi\n\n");
    Serial.println("I risultati del test sono stampati dall'altra radio.\n\n");


    if(LED_TX) pinMode(LED_TX, OUTPUT);
    if(LED_RX) pinMode(LED_RX, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Se come secondo argomento si fornisce un riferimento a Serial, la funzione
    // stampa il risultato dell'inizializzazione (riuscita o no).
    radio.inizializza(lunghezzaMessaggi, Serial);

    radio.modalitaRicezione();

    radio.impostaTimeoutAck(timeoutAck);

    Serial.println("Radio pronta per il test.\n\n");

    randomSeed(micros());

}


////////////////////////////////////////////////////////////////////////////////
// Loop principale
////////////////////////////////////////////////////////////////////////////////


void funzioneLoop() {
    leggi();
    delay(2);
    invia();
    spegniLed();
    pausa();
    radio.controlla();
}





void invia() {


    // Decidi a caso se inviare o no, con una probabilità tale da avvicinarsi alla
    // frequenza di invio `messAlMinuto`.

    // Tempo trascorso dall'ultima chiamata ad `invia()`, in microsecondi
    uint32_t t = micros();
    uint32_t deltaT = (t - microsInviaPrec);
    microsInviaPrec = t;
    // `decisione` vale `true` con una probabilita di [messPerMin * deltaT / 1 min]
    bool decisione = ((messPerMin * deltaT) > (uint32_t)random(60000000));

    if(!decisione) return;

    Serial.println("tx -");

    // crea un messaggio e inserisci numeri casuali
    uint8_t mess[lunghezzaMessaggi];
    for(int i = 2; i < lunghezzaMessaggi; i++) mess[i] = (uint8_t)(micros() % (500 - i));
    // Invia
    accendiLed(LED_TX);
    radio.inviaConAck(mess, lunghezzaMessaggi);

    // Controlla ACK
    while(radio.ackInSospeso());

}





void leggi() {

    // c'è un nuovo messaggio? se non c'è return
    if(!radio.nuovoMessaggio()) return;

    tUltimoMessaggio  = millis();

    Serial.println("   - rx");

    // ottieni la dimensione del messaggio ricevuto
    uint8_t lung = radio.dimensioneMessaggio();
    // crea un'array in cui copiarlo
    uint8_t mess[lung];

    accendiLed(LED_RX);

    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);

    // Il messaggio contiene ordini dalla radio che conduce l'esperimento
    uint32_t nuovoMessPerMin = ((uint16_t) mess[0] << 8) | mess[1];
    if(messPerMin != nuovoMessPerMin) {
        messPerMin = nuovoMessPerMin;
        Serial.print("messaggi/minuto: ");
        Serial.println(messPerMin);
    }

    if(mess[0] == 0xff && mess[1] == 0xff) fineProgramma();

    // stampa un eventuale errore
    if(erroreLettura) radio.stampaErroreSerial(Serial, erroreLettura);

}




void accendiLed(uint8_t led) {
    if(led) digitalWrite(led, HIGH);
    if(led == LED_TX) tAccensioneTx = millis();
    if(led == LED_RX) tAccensioneRx = millis();
}

void spegniLed() {
    if(millis() - tAccensioneTx > 50) digitalWrite(LED_TX, LOW);
    if(millis() - tAccensioneRx > 50) digitalWrite(LED_RX, LOW);
}




void pausa() {
    if(millis() - tUltimoMessaggio < 5000) return;

    Serial.println("-- Aspetto altra radio --");

    radio.modalitaRicezione();
    bool statoLed = true;
    uint32_t tLed = millis();
    uint8_t i = 0;
    while(!radio.nuovoMessaggio()) {
        if(millis() - tLed > 20) {
            digitalWrite(LED_RX, statoLed);
            digitalWrite(LED_TX, !statoLed);
            statoLed = !statoLed;
            tLed = millis();
            i++;
        }
        if(i == 5) break;
    }
    tUltimoMessaggio = millis();
    digitalWrite(LED_RX, LOW);
    digitalWrite(LED_TX, LOW);

    leggi();
}




void fineProgramma() {
    radio.sleep();
    bool statoLed = true;
    uint32_t t = millis();
    while(true) {
        if(millis() - t > 2000) {
            digitalWrite(LED_RX, statoLed);
            digitalWrite(LED_TX, !statoLed);
            statoLed = !statoLed;
            t = millis();
        }
    }
}
