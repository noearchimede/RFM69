/*! @file

@brief Implementazione delle funzioni fondamentali per la comunicazione

Qui sono implementate funzioni come invia, leggi, controlla, e l'isr

1. Invio
2. Ricezione
3. ACK
4. ISR
5. Gestione modalità
6. Impostazioni
*/


#include "RFM69.h"
#include "RFM69_registri.h"



// ### 1. Invia ###

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




// ### 2. Ricezione ###


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
    controlla(); // solo la parte di terminaProcesso è eseguita in questa situazione

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



// ### 3. Ack ###



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



// ### 4. ISR ###




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
            // a questo punto, ack non ancora ricevuto = ack non arriverà mai
            Serial.println("Timeout ACK");
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


// ### 6. Impostazioni ### //




void RFM69::autoModes(Modalita modBase, AMModInter modInter, AMEnterCond enterCond, AMExitCond exitCond) {
    uint8_t regAutoModes = ((uint8_t)modInter) | ((uint8_t)exitCond << 2) | ((uint8_t)enterCond << 5);
    bus->scriviRegistro(RFM69_3B_AUTO_MODES, regAutoModes);
    cambiaModalita(modBase, true);
}


void RFM69::disattivaAutoModes() {
    bus->scriviRegistro(RFM69_3B_AUTO_MODES, 0x0);
}