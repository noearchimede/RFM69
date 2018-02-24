/*! @file
@brief Esempio con il minimo di funzioni per permettere una comuniczione

Questo programma stabilisce una comunicazione unidirezionale ma con ACK tra due
radio.
*/

#include <Arduino.h>
#include "RFM69.h"


// #define MODULO_1 o MODULO_2 per compilare uno dei due programmi
//------------------------------------------------------------------------------
//#define MODULO_1
#define MODULO_1
//------------------------------------------------------------------------------


// Impostazioni
//------------------------------------------------------------------------------

// Numero medio di messaggi che ogni radio invierà ogni minuto
uint16_t messPerMin = 30;


// t
#ifdef MODULO_1
RFM69 radio(2, 3); // Pin SS, pin Interrupt, (eventualmente pin Reset)
#define LED_TX 4 // Led rosso
#define LED_RX 5 // Led giallo
#endif

// q
#ifdef MODULO_2
RFM69 radio(A2, 3, A3); // Pin SS, pin Interrupt, (eventualmente pin Reset)
#define LED_TX 8 // Led giallo
#define LED_RX 7 // Led rosso
#endif
//------------------------------------------------------------------------------



// ### Variabili, costanti e  prototipi ###

const int lunghezzaMessaggi = 4;
uint32_t tAccensioneRx, tAccensioneTx;

uint16_t messInviati = 0, messNonInviati = 0;

void invia();
void leggi();
void accendiLed(uint8_t);
void spegniLed();




void setup() {

    Serial.begin(115200);

    Serial.println("\nEsempio RFM69 - programma simmetrico\n");

    if(LED_TX) pinMode(LED_TX, OUTPUT);
    if(LED_RX) pinMode(LED_RX, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Se come secondo argomento si fornisce un riferimento a Serial, la funzione
    // stampa il risultato dell'inizializzazione (riuscita o no).
    radio.inizializza(lunghezzaMessaggi, Serial);

    // con `defaultRicezione()` basta chiamare una volta sola `iniziaRicezione()`
    radio.defaultRicezione();

    radio.iniziaRicezione();
}


bool novita = false;

void loop() {

    leggi();
    if((((micros() >> 3) + (micros() << 3)) % 60000) < messPerMin) invia();

    if(novita) {
        novita = false;
        Serial.print("ok: ");
        Serial.print(messInviati);
        Serial.print(", persi: ");
        Serial.print(messNonInviati);
        Serial.print(" - Successo: ");
        Serial.print((float)messInviati * 100 / (messInviati + messNonInviati));
        Serial.print("%");
        Serial.println();
    }

    spegniLed();
    delay(1);
}


void invia() {
    novita = true;
    // crea un messaggio
    uint8_t mess[lunghezzaMessaggi] = {1,2,3,4};

    // Invia
    radio.inviaConAck(mess, lunghezzaMessaggi);
    while(radio.aspettaAck());
    if(radio.ricevutoAck()) {
        messInviati++;
        accendiLed(LED_TX);
    }
    else {
        messNonInviati++;
    }

}


void leggi() {

    // c'è un nuovo messaggio? se non c'è return
    if(!radio.nuovoMessaggio()) return;

    novita = true;

    accendiLed(LED_RX);

    // ottieni la dimensione del messaggio ricevuto
    uint8_t lung = radio.dimensioneMessaggio();
    // crea un'array in cui copiarlo
    uint8_t mess[lung];
    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);

    // stampa un eventuale errore
    if(erroreLettura) radio.stampaErroreSerial(Serial, erroreLettura);

}


void accendiLed(uint8_t led) {
    if (led) digitalWrite(led, HIGH);
    if(led == LED_TX) tAccensioneTx = millis();
    if(led == LED_RX) tAccensioneRx = millis();
}

void spegniLed() {
    if(millis() - tAccensioneRx > 50) digitalWrite(LED_RX, LOW);
    if(millis() - tAccensioneTx > 50) digitalWrite(LED_TX, LOW);
}
