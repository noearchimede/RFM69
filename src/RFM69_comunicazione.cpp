/*! @file

@brief Implementazione delle principali funzioni pubbliche della classe RFM69

Questo file contiene l'implementazione della maggior parte delle funzioni pubbliche
della classe. Ne sono escluse solo le funzioni di impostazione della radio, che
utilizzano le costanti definite nel file RFM69_impostazioni.h che non è incluso
in questo file (le funzioni qui non devono usare quelle costanti).

Il file è suddiviso in 5 sezioni:

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


// [funzione privata] Invia un messaggio conoscendone già l'intestazione
//
int RFM69::inviaMessaggio(const uint8_t messaggio[], uint8_t lunghezza, uint8_t intestazione, bool insisti) {

    // la radio non può inviare pacchetti di lunghezza 0 (solo byte "dimensione")
    if(lunghezza == 0) return Errore::inviaMessaggioVuoto;


    // l'opzione 'insisti' permette di inviare anche quando un particolare stato
    // della classe lo impedisce, aspettando, fino a un ragionavole timeout, che
    // la condizione ostacolante sia risolta
    if(!radioPronta(insisti)) return Errore::inviaTimeout;

    cambiaModalita(Modalita::standby, true);
    disattivaAutoModes();

    // Il primo byte contiene la lunghezza del messaggio compresa l'intestazione
    // ma sé stesso escluso.
    // Anche le radio useranno questo valore per inviare/ricevere il pacchetto.
    bus->scriviRegistro(RFM69_00_FIFO, lunghezza + 1);
    // Il secondo byte è l'intestazione della classe
    bus->scriviRegistro(RFM69_00_FIFO, intestazione);
    // Tutti gli altri bytes sono il messaggio dell'utente
    for(int i = 0; i < lunghezza; i++) {
        bus->scriviRegistro(RFM69_00_FIFO, messaggio[i]);
    }


    // separa mesasggi con e senza richiesta di ACK
    Intestazione intest;
    intest.byte = intestazione;
    if(intest.bit.richiestaAck) {
        // metti la radio in modalità trasmissione con l'ordine di passare a
        // ricezione non appena il pacchetto è stato inviato. In questo modo la
        // radio è da subito pronta per ricevere un ACK. Siccome la radio non sembra
        // essere capace di eseguire la sequenza ideale tx -> rx ->
        // standby/modalitaDefault autonomamente, dopo la ricezione di un ACK rimane
        // in modalità rx. (La condizione di uscita dalla modalità intermedia scelta
        // (packetSent) non accade mai in modalità rx).
        //
        // Questa soluzione non è ottimale: per esempio un nuovo messaggio potrebbe
        // essere sovrascritto all'ACK prima che questo sia riconosciuto da
        // controlla(). Non si può però tornare alla modalità tx perché questo
        // passaggio cancella la memoria FIFO, e non sembra esserci modo per passare
        // a una terza modalità senza dover scrivere registri della radio nell'ISR.
        autoModes(Modalita::tx, AMModInter::rx, AMEnterCond::packetSentRising, AMExitCond::crcOkRising);
        stato = Stato::invioMessConAck;
        statoUltimoAck = StatoAck::pendente;
    }
    else {
        cambiaModalita(Modalita::tx);
        stato = Stato::invioMessSenzaAck;
    }

    tempoUltimaTrasmissione = millis();

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



// Leggi l'ultimo messaggio ricevuto.
// l'argomento `dimensione` deve essere uguale (o minore) alla grandezza
// dell'array `messaggio`. Dopo l'esecuzione della funzione conterrà la
// lunghezza effettiva del messaggio.
//
int RFM69::leggi(uint8_t messaggio[], uint8_t &lunghezza) {

    // Nessun messaggio in entrata
    if(!messaggioRicevuto) return Errore::leggiNessunMessaggio;
    clear(messaggioRicevuto);

    // termina il processo e torna alla modalità di default
    stato = Stato::attesaAzione;
    
    set(richiestaAzione.terminaProcesso);
    controlla(); // solo la parte di terminaProcesso è seguita in questa situazione

    // Messaggio troppo lungo per questa radio
    if(lungMaxMessEntrata < ultimoMessaggio.dimensione) return Errore::messaggioTroppoLungo;
    // Messaggio troppo lungo per l'array dell'utente
    if(lunghezza < ultimoMessaggio.dimensione) return Errore::leggiArrayTroppoCorta;

    // Trascrivi messaggio
    lunghezza = ultimoMessaggio.dimensione;
    for(unsigned int i = 0; i < lunghezza; i++) {
        messaggio[i] = buffer[i];
    }

    return Errore::ok;
}



void RFM69::inviaAck() {

    // nell'implementazione attuale inviaAck è chiamata solo in un punto, e lì
    // la modalità è per forza gestisciNuovoMessaggio, quindi non controllare
    // if(stato != Stato::gestisciNuovoMessaggio) return codiceErrore;

    cambiaModalita(Modalita::standby);
    disattivaAutoModes();

    // Lunghezza, obbligatoria perché serve alla radio
    bus->scriviRegistro(RFM69_00_FIFO, 1);

    // Intestazione, segnala che il messaggio è un ACK
    Intestazione intestazione;
    intestazione.bit.ack = 1;
    bus->scriviRegistro(RFM69_00_FIFO, intestazione.byte);


    // "controlla()" si occupa di uscire dalla modalità intermedia
    //autoModes(Modalita::tx, AMModInter::standby, AMEnterCond::packetSentRising, AMExitCond::crcOkRising);
    cambiaModalita(Modalita::tx, true);
    // while(bus->leggiRegistro(RFM69_28_IRQ_FLAGS_2) & RFM69_FLAGS_2_PACKET_SENT);
    // set(richiestaAzione.terminaProcesso);
    // controlla();

    stato = Stato::invioAck;

}


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



// ISR reale, static, che chiama un interrupt handler (la funzione `isr()`) che non essendo static è legata all'istanza della radio
void RFM69::isrCaller() {
    pointerRadio->isr();
}



// Reagisce agli interrupt generati dalla radio sul suo pin DI0
//
void RFM69::isr() {

    tempoUltimaEsecuzioneIsr = millis();

    switch(stato) {
        
        // Un interrupt in stato passivo può solo indicare la ricezione di un
        // messaggio
        case Stato::passivo:
            // AutoModes si occupa di mettere la radio in standby
            set(richiestaAzione.scaricaMessaggio);
            set(richiestaAzione.inviaAckOTermina);
            set(richiestaAzione.annunciaMessaggio);
            stato = Stato::attesaAzione;
            break;

        // fine della trasmissione
        case Stato::invioMessSenzaAck:
            ++messaggiInviati;
            // siamo ancora in modalità tx, che non serve più
            set(richiestaAzione.terminaProcesso);
            stato = Stato::attesaAzione;
            break;
            
        // fine della trasmissione
        case Stato::invioMessConAck:
            ++messaggiInviati;
            // non è richiesta nessuna azione perché il passaggio da tx a rx
            // necessario in questo momento è gestito autonomamente dalla radio
            // grazie alla funtione AutoModes
            stato = Stato::attesaAck;
            break;

        // Questo caso si verifica se arriva un Ack (o un altro messaggio
        // durante l'attesa di un ack)
        case Stato::attesaAck:
            statoUltimoAck = StatoAck::attesaVerifica;
            set(richiestaAzione.scaricaMessaggio);
            set(richiestaAzione.verificaAck);
            set(richiestaAzione.terminaProcesso);
            stato = Stato::attesaAzione;
            break;

        case Stato::invioAck:
            set(richiestaAzione.terminaProcesso);
            digitalWrite(3, !digitalRead(3));
            break;

        default: break;
    }

}




// funzione che esegue gli ordini dell'ISR, controlla timeout, ...
//
int RFM69::controlla() {

    int errore = Errore::ok; // returned alla fine

    // # 1. controlla timeout #
    if(stato == Stato::attesaAck) {
        if(millis() - tempoUltimaTrasmissione > timeoutAck) {
            Serial.println("Timeout ACK");
            // a questo punto, ack non ancora ricevuto = ack non arriverà mai
            statoUltimoAck = StatoAck::nonRicevuto;
            stato = Stato::attesaAzione;
            set(richiestaAzione.terminaProcesso);
        }
    }
    if(stato == Stato::invioMessConAck || stato == Stato::invioMessSenzaAck) {
        // usa arbitrariamente 100 ms come tempo massimo per trasmettere
        if(millis() - tempoUltimaTrasmissione > 100) {
            Serial.println("Timeout trasmissione");
            errore = Errore::controllaTimeoutTx;
            stato = Stato::attesaAzione;
            set(richiestaAzione.terminaProcesso);
        }
    }

    
    // # 2. Esegui compiti ordinati dall'ISR per concludere un'azione #

    // nota: l'ordine di esecuzione è rilevante, perché alcune azioni dipendono
    //  dalla precedente esecuzione di altre

    if(stato == Stato::attesaAzione) {
        Serial.println("Azione Controlla: ");

        // per verificare l'ack è necessario prima scaricare l'intestazione
        if(richiestaAzione.scaricaMessaggio) {
        Serial.println("   scaricaMessaggio");
            clear(richiestaAzione.scaricaMessaggio );

            ultimoMessaggio.tempoRicezione = tempoUltimaEsecuzioneIsr;
            // leggi e salva localmente i primi due bytes (lunghezza e intestazione)
            uint8_t lung = bus->leggiRegistro(RFM69_00_FIFO);
            ultimoMessaggio.dimensione = lung - 1;
            ultimoMessaggio.intestazione.byte = bus->leggiRegistro(RFM69_00_FIFO); 
            // leggi tutti gli altri bytes
            if(ultimoMessaggio.dimensione > 0) {
            bus->leggiSequenza(RFM69_00_FIFO, ultimoMessaggio.dimensione, buffer);
            }

            // qualsiasi messagio (ack, messaggio, atteso o no) porta
            // l'informazione più recente sulla distanza dell'altra radio
            //[RSSI = - REG_0x24 / 2, vedi datasheet]
            ultimoRssi = -(bus->leggiRegistro(RFM69_24_RSSI_VALUE/2));
        }

        if(richiestaAzione.verificaAck) {
        Serial.println("   verificaAck");
            clear(richiestaAzione.verificaAck);

            if(ultimoMessaggio.intestazione.bit.ack) {
                statoUltimoAck = StatoAck::ricevuto;
                // statistiche
                durataUltimaAttesaAck = ultimoMessaggio.tempoRicezione - tempoUltimaTrasmissione;
                if(durataMassimaAttesaAck > durataMassimaAttesaAck) {
                    durataMassimaAttesaAck = durataUltimaAttesaAck;
                }
                ++nrAckRicevuti;
                sommaAtteseAck += durataUltimaAttesaAck;
            }
            else {
                statoUltimoAck = StatoAck::nonRicevuto;
            }
        }

        if(richiestaAzione.inviaAckOTermina) {
        Serial.println("   inviaAck");
            clear(richiestaAzione.inviaAckOTermina);

            if(ultimoMessaggio.intestazione.bit.richiestaAck) {
                inviaAck();
                // inviaAck mette lo stato su Stato::invioAck, poi l'isr lo cambia
                // a passivo
            }
            else set(richiestaAzione.terminaProcesso);
        }

        if(richiestaAzione.annunciaMessaggio) {
            clear(richiestaAzione.annunciaMessaggio);

            // controlla che non si tratti di un ack inatteso
            if(ultimoMessaggio.intestazione.bit.ack) ++ackInattesi;
            else {
                set(messaggioRicevuto);
            }
        }

        // Un processo è terminato, torna alla modalità di default
        if(richiestaAzione.terminaProcesso) {
        Serial.println("   terminaProcesso");
            clear(richiestaAzione.terminaProcesso);

            stato = Stato::passivo;
            // introduzione di AutoModes: rx in realtà corrisponde a un modo
            // automatico particolare
            if(modalitaDefault == Modalita::rx) {
                // non serve disattivare AutoModes perché modalitaRicezione lo reimposta
                modalitaRicezione();
            }
            else {
                disattivaAutoModes();
                cambiaModalita(modalitaDefault);
            }
        }
        
    }
    
    return errore;
}






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




// ### 6. Impostazioni ### //


// Cambia la modalità di funzionamento
//
int RFM69::cambiaModalita(RFM69::Modalita mod, bool aspetta) {

    // Nota: la disattivazione e l'attivazione della modalità listen richiedono
    // procedure speciali. Le aggiunte rispetto al codice per l'impostazione delle
    // altre modalità sono marcate con "// *Listen*" (all'inizio di ogni blocco)

    // La modalità listen non è impostabile senza attesa
    if(!aspetta && (mod == Modalita::listen)) return Errore::modImpossibile;

    // Se l'ISR ha appena chiesto una disattivazione del modo Tx potrebbe darsi
    // che la radio è ancora in fase di transizione. 2 ms di attesa riducono
    // le probabilità di conflitto (già molto bassa, e comunque non nulla).
    // Al momento della scrittura di questa funzione, questo può succedere solo se
    // l'impostazione TEMPO_MINIMO_FRA_MESSAGGI è 0 in caso di più invii di seguito
    if(aspetta) delay(2);

    // Prepara il byte da scrivere nel registro
    regOpMode &= 0xE3;
    uint8_t codiceMod = 0x0;
    switch(mod) {
        case Modalita::sleep:   codiceMod = 0x0;    break;
        // per entrare in modalità listen la radio deve essere in standby
        case Modalita::listen:
        case Modalita::standby: codiceMod = 0x1;    break;
        case Modalita::fs:      codiceMod = 0x2;    break;
        case Modalita::tx:      codiceMod = 0x3;    break;
        case Modalita::rx:      codiceMod = 0x4;    break;
    }
    regOpMode |= (codiceMod << 2);

    // *Listen* (1/3) - Disattiva
    if (modalita == Modalita::listen) {
        regOpMode &= !(1 << 6); // clear ListenOn
        regOpMode |= 1 << 5; // set ListenAbort
    }

    //Scrivi il registro RegOpMode
    bus->scriviRegistro(RFM69_01_OP_MODE, regOpMode);

    // *Listen* (2/3) - Disattiva
    if (modalita == Modalita::listen) {
        regOpMode &= !(1 << 5); // clear ListenAbort
        // Scrivi di nuovo RegOpMode senza il bit ListenAbort
        bus->scriviRegistro(RFM69_01_OP_MODE, regOpMode);
    }


    // Impostazioni riguardanti la trasmissione high power (> 17dbM)

    // La radio deve entrare in modalità high power tx
    else if (highPower && (mod ==  Modalita::tx)) highPowerSettings(true);
    // La radio deve uscire dalla modalità high power tx
    if (highPower && (modalita == Modalita::tx)) highPowerSettings(false);

    if(aspetta) {
        // Aspetta che la radio sia pronta (questa flag è 0 durante il cambiamento di
        // modalità)
        // timeout: le transizioni più lunghe sembrano durare circa 1 ms (empirico)
        unsigned long inizioAttesa = millis();
        while(!(bus->leggiRegistro(RFM69_27_IRQ_FLAGS_1) & RFM69_FLAGS_1_MODE_READY)) {
            if(inizioAttesa + 3 < millis()) return Errore::modTimeout;
        }
    }

    // *Listen* (3/3) - Attiva
    // Per attivare la modalita listen basta scrivere il bit 6 dopo aver
    // messo la radio in standby
    if(mod == Modalita::listen) {
        regOpMode |=  1 << 6;
        bus->scriviRegistro(RFM69_01_OP_MODE, regOpMode);
    }

    // Imposta l'interrupt generato sul pin DIO0
    uint8_t dio0 = 0;
    switch(mod) {
        case Modalita::sleep:                 break; // non importa
        case Modalita::listen:    dio0 = 1;   break;
        case Modalita::standby:               break; // non importa
        case Modalita::fs:                    break; // non importa
        case Modalita::tx:        dio0 = 0;   break;
        case Modalita::rx:        dio0 = 1;   break;
    }

    bus->scriviRegistro(RFM69_25_DIO_MAPPING_1, dio0 << 6);

    // Ricorda la modalita attuale della radio
    modalita = mod;

    // switch(mod) {
    //     case Modalita::tx : Serial.println("_tx_"); break;
    //     case Modalita::rx : Serial.println("_rx_"); break;
    //     case Modalita::standby : Serial.println("_standby_"); break;
    //     case Modalita::sleep : Serial.println("_sleep_"); break;
    //     case Modalita::listen : Serial.println("_listen_"); break;
    //     default: break;
    // }

    return Errore::ok;
}


// Passa rapidamente in modalità standby (funzione usata dall'ISR)
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


void RFM69::autoModes(Modalita modBase, AMModInter modInter, AMEnterCond enterCond, AMExitCond exitCond) {
    uint8_t regAutoModes = ((uint8_t)modInter) | ((uint8_t)exitCond << 2) | ((uint8_t)enterCond << 5);
    bus->scriviRegistro(RFM69_3B_AUTO_MODES, regAutoModes);
    cambiaModalita(modBase, true);
}


void RFM69::disattivaAutoModes() {
    bus->scriviRegistro(RFM69_3B_AUTO_MODES, 0x0);
}

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
