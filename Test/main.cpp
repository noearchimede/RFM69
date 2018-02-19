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
 #define MODULO_2
//------------------------------------------------------------------------------




// t
#ifdef MODULO_1
// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(2, 3);
// Un LED, 0 per non usarlo
#define LED 4
#endif

// q
#ifdef MODULO_2
// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(A2, 3, A3);
// Un LED, 0 per non usarlo
#define LED 7
#endif




void setup() {

    Serial.begin(115200);
    if(LED) pinMode(LED, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Se come secondo argomento si fornisce un riferimento a Serial, la funzione
    // stampa il risultato dell'inizializzazione (riuscita o no).
    radio.inizializza(4, Serial);

}



#ifdef MODULO_1


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



#ifdef MODULO_2


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
