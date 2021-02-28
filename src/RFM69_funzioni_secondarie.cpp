/*! @file

@brief Implementazione di funzioni utili ma non indispensabili

Qui sono implementati wrapper pubblici per varie funzioni `inviaMessaggio`,
funzioni per controllore lo stato della radio e funzioni per cambiarne la
modalità di funzionamento.

1. Invio
2. Ricezione
3. ACK
4. ISR
5. Gestione modalità
6. Impostazioni
*/

#include "RFM69.h"
#include "RFM69_registri.h"



// ### 1. Invio ### //

// Invia un messaggio di dimensione compresa tra 1 e 64 bytes
//
int RFM69::invia(const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo) {
    Intestazione intestazione;
    if(titolo > valMaxTitolo) titolo = 0;
    intestazione.bit.titolo = titolo;
    return inviaMessaggio(messaggio, lunghezza, intestazione.byte);
}

// Invia un messaggio di dimensione compresa tra 1 e 64 bytes richiedendo
// un ACK all'altra radio
//
int RFM69::inviaConAck(const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo) {
    Intestazione intestazione;
    intestazione.bit.richiestaAck = 1;
    if(titolo > valMaxTitolo) titolo = 0;
    intestazione.bit.titolo = titolo;
    return inviaMessaggio(messaggio, lunghezza, intestazione.byte);
}

// Invia un messaggio di dimensione compresa tra 1 e 64 bytes richiedendo
// un ACK all'altra radio
//
int RFM69::inviaFinoAck(uint16_t& tentativi, const uint8_t messaggio[], uint8_t lunghezza, uint8_t titolo) {

    Intestazione intestazione;
    intestazione.bit.richiestaAck = 1;
    if(titolo > valMaxTitolo) titolo = 0;
    intestazione.bit.titolo = titolo;

    int errore;
    uint16_t i = 0;
    for(; i < tentativi; i++) {
        // Invia il messaggio
        errore = inviaMessaggio(messaggio, lunghezza, intestazione.byte);
        if(errore != Errore::ok) return errore;
        // attesa di al massimo `timeoutAck` millisecondi
        while(ackInSospeso());
        // controllo
        if(ricevutoAck()) break;
    }

    if(!ricevutoAck()) return Errore::errore;
    tentativi = i;
    return Errore::ok;
}




bool RFM69::staTrasmettendo() {
    switch(stato) {
        case Stato::invioAck:
        case Stato::invioMessConAck:
        case Stato::invioMessSenzaAck:
            return true;
        case Stato::attesaAck:
        case Stato::attesaAzione:
        case Stato::passivo:
            return false;
    }
}



// ### 2. Ricezione ### //





// Restituisce `true` se un nuovo messaggio è disponibile
//
bool RFM69::nuovoMessaggio() {
    controlla();
    return messaggioRicevuto;
}



// Restituisce la dimensione dell'ultiomo messaggio ricevuto
//
uint8_t RFM69::dimensioneMessaggio() {
    return ultimoMessaggio.dimensione;
}

// Restituisce il titolo dell'ultimo messaggio
//
uint8_t RFM69::titoloMessaggio() {
    return ultimoMessaggio.intestazione.bit.titolo;
}

// Restituisce il valore RSSI del segnale più recente (messaggio o ACK)
//
int8_t RFM69::rssi() {
    return ultimoRssi;
}

// Restituisce l'"ora" di ricezione dell'ultimo messaggio
uint8_t RFM69::tempoRicezione() {
    return ultimoMessaggio.tempoRicezione;

}




// ### 3. ACK ### //


// Restituisce true se la classe sta aspettando un ack
//
bool RFM69::ackInSospeso() {
    controlla();
    if(statoUltimoAck == StatoAck::pendente) return true;
    if(statoUltimoAck == StatoAck::attesaVerifica) return true;
    return false;
}


// Restituisce true se la radio ha ricevuto un ACK per l'ultimo messaggio inviato
//
bool RFM69::ricevutoAck() {
    if(statoUltimoAck == StatoAck::ricevuto) return true;
    return false;
}



// ### 4. ISR ### //

// nessuna funzione "derivata"



// ### 5. Gestione modalità ###


bool RFM69::radioPronta(bool aspetta) {
    if(stato != Stato::passivo) {
        // forse basta aggiornare le variabili di stato (un controllo dura poco)
        controlla();
        if(stato != Stato::passivo) {
            // aspettare un attimo (insisti) o uscire subito con un errore?
            if (aspetta) {
                uint32_t t = millis();
                while (stato != Stato::passivo) {
                    // a disposizione di altre classi, questa è un'attesa non
                    // critica. Occhio però a non abusare di questa funzione! il
                    // comportamento del programma diventa facilmente
                    // incomprensibilmente incorretto.
                    yield();
                    controlla();
                    if (millis() - timeoutAspetta > t) return false;
                }
            }
            else { // !aspetta
                return false;
            }
        }
    }
    return true;
}


// Attiva la radio in modo che possa ricevere dei messaggi
//
void RFM69::modalitaRicezione(bool aspetta) {
    modalitaDefault = Modalita::rx;
    if(radioPronta(aspetta)) {
        // automodes qui è usato solo per entrare in standby, la condizione di
        // uscita è volutamente irragigungibile per peché l'uscita è gestita da
        // controlla()
        autoModes(Modalita::rx, AMModInter::standby, AMEnterCond::crcOkRising, AMExitCond::packetSentRising);
    }
    autoModes(Modalita::rx, AMModInter::standby, AMEnterCond::crcOkRising, AMExitCond::packetSentRising);
    // else l'ISR e controlla() si occuperanno di impostare la modalità default,
    // che ora è rx    
}

void  RFM69::modalitaListen(bool aspetta) {
    modalitaDefault = Modalita::listen;
    if(radioPronta(aspetta)) cambiaModalita(Modalita::listen);
}

void  RFM69::standby(bool aspetta) {
    modalitaDefault = Modalita::standby;
    if(radioPronta(aspetta)) cambiaModalita(Modalita::standby);
}

void  RFM69::sleep(bool aspetta) {
    modalitaDefault = Modalita::sleep;
    if(radioPronta(aspetta)) cambiaModalita(Modalita::sleep);
}

// Passa in modalità standby (ottimizzata per poter essere usata in `controlla()`)
//
void RFM69::modalitaStandby() {
    // Prepara il byte da scrivere nel registro
    regOpMode &= 0xE3;
    regOpMode |= 0x04;
    //Scrivi il registro RegOpMode
    bus->scriviRegistro(RFM69_01_OP_MODE, regOpMode);
    // Ricorda la modalita attuale della radio
    modalita = Modalita::standby;
}




// ### 6. Impostazioni ### //





// Scrive le impostazioni "high power" (per l'utilizzo del modulo con una potenza
// superiore a 17 dBm).
// Vedi il commento della funzione di inizializzazione 'impostaPotenzaTx()' per
// dettagli.
//
void RFM69::highPowerSettings(bool attiva) {

    if(attiva) {
        bus->scriviRegistro(RFM69_13_OCP, 0x0F); // RegOcp (0x13) -> 0x0F
        bus->scriviRegistro(RFM69_5A_TEST_PA_1, 0x5D); // RegTestPa1 (0x5A) -> 0x5D
        bus->scriviRegistro(RFM69_5C_TEST_PA_2, 0x7C); // RegTestPa2 (0x5C) -> 0x7C
    }
    else {
        bus->scriviRegistro(RFM69_13_OCP, 0x1A); // RegOcp (0x13) -> 0x1A
        bus->scriviRegistro(RFM69_5A_TEST_PA_1, 0x55); // RegTestPa1(0x5A) -> 0x55
        bus->scriviRegistro(RFM69_5C_TEST_PA_2, 0x70); // RegTestPa2(0x5C) -> 0x70
    }
}



void RFM69::impostaTimeoutAck(uint16_t tempoMs) {
    timeoutAck = tempoMs;
}







void RFM69::stampaErroreSerial(HardwareSerial& serial, int errore, bool contesto) {

    if(contesto) {

        switch(errore) {
            case Errore::ok :
            case Errore::errore :
            break;

            case Errore::initTroppeRadio :
            case Errore::initInitSPIFallita :
            case Errore::initNessunaRadioConnessa :
            case Errore::initVersioneRadioNon0x24 :
            case Errore::initPinInterruptNonValido :
            case Errore::initErroreImpostazione :
            serial.print(F("RFM69: init: ")); break;

            case Errore::inviaMessaggioVuoto :
            case Errore::inviaTimeout :
            serial.print(F("RFM69: invia: ")); break;

            case Errore::leggiNessunMessaggio :
            case Errore::leggiArrayTroppoCorta :
            serial.print(F("RFM69: leggi: ")); break;

            case Errore::modImpossibile:
            case Errore::modTimeout :
            serial.print(F("RFM69: cambiaMod: ")); break;
        }
    }
    switch(errore) {
        case Errore::ok :
        serial.print(F("ok!")); break;
        case Errore::errore :
        serial.print(F("errore")); break;

        case Errore::initTroppeRadio :
        serial.print(F("troppe radio")); break;
        case Errore::initInitSPIFallita :
        serial.print(F("init SPI fallita")); break;
        case Errore::initNessunaRadioConnessa :
        serial.print(F("nessuna radio connessa")); break;
        case Errore::initVersioneRadioNon0x24 :
        serial.print(F("versione radio non 0x24")); break;
        case Errore::initPinInterruptNonValido :
        serial.print(F("pin interrupt non valido")); break;
        case Errore::initErroreImpostazione :
        serial.print(F("errore impostazione")); break;

        case Errore::inviaMessaggioVuoto :
        serial.print(F("messaggio vuoto")); break;
        case Errore::inviaTimeout :
        serial.print(F("radio occupata, timeout")); break;

        case Errore::leggiNessunMessaggio :
        serial.print(F("nessun messaggio")); break;
        case Errore::leggiArrayTroppoCorta :
        serial.print(F("array troppo corta")); break;

        case Errore::modImpossibile:
        serial.print(F("impossibile cambiare")); break;
        case Errore::modTimeout :
        serial.print(F("timeout")); break;
    }
    serial.println();
}
