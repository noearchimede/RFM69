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

//------------------------------------------------------------------------------
//----------------- Descrizione dell'hardware di questa radio ------------------
//------------------------------------------------------------------------------

// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(A2, 3, A3);

#define LED_TX 8
#define LED_ACK 7

//------------------------------------------------------------------------------
// ## Impostszioni che devono essere identiche sulle due radio ## //
//------------------------------------------------------------------------------

// Lunghezza in bytes del contenuto dei messaggi. La lunghezza effettiva sarà
// 4 bytes in più (lunghezza [1 byte], intestazione [1], contenuto [...], crc [2]);
// a questo si aggiunge il preamble che è lungo per default 4 bytes
const uint8_t lunghezzaMessaggi = 4;
// Tempo massimo di attesa per un ACK
const uint8_t timeoutAck = 50;


//******************************************************************************
//******************************************************************************



// ### Variabili e costanti globali ### //

uint32_t tAccensioneRx, tAccensioneTx;
uint32_t microsInviaPrec = 0;
uint32_t messPerMin = 0;
uint32_t tUltimoMessaggio = 0;


// ### Prototipi ### //

void invia();
void leggi();
void accendiLed(uint8_t);
void spegniLed();
void pausa();
void fineProgramma();


//******************************************************************************
//******************************************************************************


void setup() {

    Serial.begin(115200);

    Serial.println("\n\n\n\nRFM69 - Test collisione messaggi\n\n");
    Serial.println("I risultati del test sono stampati dall'altra radio.\n\n");


    if(LED_TX) pinMode(LED_TX, OUTPUT);
    if(LED_ACK) pinMode(LED_ACK, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Se come secondo argomento si fornisce un riferimento a Serial, la funzione
    // stampa il risultato dell'inizializzazione (riuscita o no).
    radio.inizializza(lunghezzaMessaggi, Serial);

    // con `defaultRicezione()` basta chiamare una volta sola `iniziaRicezione()`
    radio.defaultRicezione();
    radio.iniziaRicezione();

    radio.impostaTimeoutAck(timeoutAck);

    Serial.print("Aspetto un messaggio... ");
    bool statoLed = true;
    uint32_t t = millis();
    while(!radio.nuovoMessaggio()) {
        if(millis() - t > 1000) {
            digitalWrite(LED_ACK, statoLed);
            digitalWrite(LED_TX, !statoLed);
            statoLed = !statoLed;
            t = millis();
        }
    }
    leggi();
    digitalWrite(LED_ACK, LOW);
    digitalWrite(LED_TX, LOW);

    Serial.println("ricevuto, inizio test.\n\n");

    randomSeed(micros());

}



void loop() {

    leggi();
    invia();
    spegniLed();
    delay(1);

    // Visto che questo test termina dopo la compromissione dell'unico canale di
    // comunicazione tra le due radio, ogni tanto è necessario che questa radio
    // smetta di trasmettere per sapere se il test è ancora in corso e se si a
    // che velocita.
    if(millis() - tUltimoMessaggio > 5000) pausa();
}





void invia() {


    // Decidi a caso se inviare o no, con una probabilità tale da avvicinarsi alla
    // frequenza di invio `messAlMinuto`.

    // Tempo trascorso dall'ultima chiamata ad `invia()`, in microsecondi
    uint32_t t = micros();
    uint32_t deltaT = (t - microsInviaPrec);
    microsInviaPrec = t;
    // `decisione` vale `true` con una probabilita di [messPerMin * deltaT / 1 min]
    bool decisione = ((messPerMin * deltaT) > random(60000000));

    if(!decisione) return;

    Serial.print("tx | mess/minuto: ");
    Serial.println(messPerMin);

    // crea un messaggio e inserisci numeri casuali
    uint8_t mess[lunghezzaMessaggi];
    for(int i = 2; i < lunghezzaMessaggi; i++) mess[i] = (uint8_t)(micros() % (500 - i));

    // Invia
    accendiLed(LED_TX);
    radio.inviaConAck(mess, lunghezzaMessaggi);
    while(radio.ackInSospeso());


    if(radio.ricevutoAck()) accendiLed(LED_ACK);


}





void leggi() {

    // c'è un nuovo messaggio? se non c'è return
    if(!radio.nuovoMessaggio()) return;

    tUltimoMessaggio  = millis();

    Serial.println("rx");

    // ottieni la dimensione del messaggio ricevuto
    uint8_t lung = radio.dimensioneMessaggio();
    // crea un'array in cui copiarlo
    uint8_t mess[lung];

    accendiLed(LED_ACK);

    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);

    // Il messaggio contiene ordini dalla radio che conduce l'esperimento
    messPerMin = ((uint16_t) mess[0] << 8) | mess[1];

    if(mess[2] == 0x7B) fineProgramma(); //0x7B è un valore arbitrario

    // stampa un eventuale errore
    if(erroreLettura) radio.stampaErroreSerial(Serial, erroreLettura);

}




void accendiLed(uint8_t led) {
    if (led) digitalWrite(led, HIGH);
    if(led == LED_TX) tAccensioneTx = millis();
    if(led == LED_ACK) tAccensioneRx = millis();
}

void spegniLed() {
    if(millis() - tAccensioneRx > 10) digitalWrite(LED_TX, LOW);
    if(millis() - tAccensioneTx > 10) digitalWrite(LED_ACK, LOW);
}




void pausa() {
    radio.iniziaRicezione();
    bool statoLed = true;
    uint32_t t = millis();
    while(!radio.nuovoMessaggio()) {
        if(millis() - t > 200) {
            digitalWrite(LED_ACK, statoLed);
            digitalWrite(LED_TX, !statoLed);
            statoLed = !statoLed;
            t = millis();
        }
    }
    digitalWrite(LED_ACK, LOW);
    digitalWrite(LED_TX, LOW);

    leggi();
}




void fineProgramma() {
    radio.sleep();
    bool statoLed = true;
    uint32_t t = millis();
    while(true) {
        if(millis() - t > 2000) {
            digitalWrite(LED_ACK, statoLed);
            digitalWrite(LED_TX, !statoLed);
            statoLed = !statoLed;
            t = millis();
        }
    }
}
