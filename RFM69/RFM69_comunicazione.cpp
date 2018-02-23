/*! @file

@brief Implementazione delle principali funzioni pubbliche della classe RFM69

Questo file contiene l'implementazione della maggior parte delle funzioni pubbliche
della classe. Ne sono escluse solo le funzioni di impostazione della radio, che
utilizzano le costanti definite nel file RFM69_impostazioni.h che non è incluso
in questo file (le funzioni qui non devono usare quelle costanti).

Il file è suddiviso in 5 sezioni:

1. Invio
2. Ricezione
3. ISR
4. ACK
5. Impostazioni
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
        aspettaAck();
        // controllo
        if(ricevutoAck()) break;
    }

    tentativi = i;
    return Errore::ok;
}


// [funzione privata] Invia un messaggio conoscendone già l'intestazione
//
int RFM69::inviaMessaggio(const uint8_t messaggio[], uint8_t lunghezza, uint8_t intestazione) {

    // la radio non può inviare pacchetti di lunghezza 0 (solo byte "dimensione")
    if(lunghezza == 0) return Errore::inviaMessaggioVuoto;

    // Se c'è un messaggio in uscita non si può inviarne un altro. Visto che di
    // solito la trasmissione dura pochi millisecondi, però, si può aspettare.
    // (Se si ha fretta non bisogna chiamare invia due volte di seguito)
    if(trasmissioneMessaggio || trasmissioneAck) {
        uint32_t t = millis();
        while(trasmissioneMessaggio || trasmissioneAck) {
            // Il messaggio più lungo alla bitrate minima con l'encoding più
            // dispendioso parte in una trentina di ms
            if(millis() - 50 > t) return Errore::inviaTimeoutTxPrecedente;
        }
    }

    // Se la classe sta aspettando un ACK ignoralo (se l'utente sta inviando un
    // nuovo messaggio significa che non controllerà più l'ack del precedente)
    if(attesaAck) rinunciaAck();


    cambiaModalita(Modalita::standby);

    // Il primo byte contiene la lunghezza del messaggio compresa l'intestazione
    // ma sé stesso escluso.
    // Anche le radio useranno questo valore per inviare/ricevere il pacchetto.
    spi.scriviRegistro(RFM69_00_FIFO, lunghezza + 1);

    // Il secondo byte è l'intestazione della classe
    spi.scriviRegistro(RFM69_00_FIFO, intestazione);

    // Tutti gli altri bytes sono il messaggio dell'utente
    for(int i = 0; i < lunghezza; i++) {
        spi.scriviRegistro(RFM69_00_FIFO, messaggio[i]);
    }

    cambiaModalita(Modalita::tx);
    trasmissioneMessaggio = true;

    Intestazione intest;
    intest.byte = intestazione;
    if(intest.bit.richiestaAck) attesaAck = true;
    else attesaAck = false;
    ackRicevuto = false;
    tempoUltimaTrasmissione = millis();

    return Errore::ok;

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
    // Messaggio troppo lungo per questa radio
    if(lungMaxMessEntrata < ultimoMessaggio.dimensione) return Errore::messaggioTroppoLungo;
    // Messaggio troppo lungo per l'array dell'utente
    if(lunghezza < ultimoMessaggio.dimensione) return Errore::leggiArrayTroppoCorta;
    // OK

    // Trascrivi messaggio
    lunghezza = ultimoMessaggio.dimensione;
    for(int i = 0; i < lunghezza; i++) {
        messaggio[i] = buffer[i];
    }

    // ACK
    if(ultimoMessaggio.intestazione.bit.richiestaAck) {
        inviaAck();
        // Modalita tx, cambiata poi in `modalitaDefault` dall'ISR
    }
    else {
        cambiaModalita(modalitaDefault);
    }

    messaggioRicevuto = false;

    return Errore::ok;
}



void RFM69::inviaAck() {
    // Se c'è un messaggio in uscita non si può inviare un altro. Visto che di
    // solito la trasmissione dura pochi millisecondi, però, si può aspettare.
    // Alla fine dell'attesa il messaggio uscente viene bloccato.
    if(trasmissioneMessaggio) {
        uint32_t t = millis();
        while(trasmissioneMessaggio) {
            // Il messaggio più lungo alla bitrate minima con l'encoding più
            // dispendioso parte in una trentina di ms
            if(millis() - 50 > t) break;
        }
    }

    // Lunghezza, obbligatoria perché serve alla radio (1)
    spi.scriviRegistro(RFM69_00_FIFO, 1);

    // Intestazione, segnala che il messaggio è un ACK
    Intestazione intestazione;
    intestazione.bit.ack = 1;
    spi.scriviRegistro(RFM69_00_FIFO, intestazione.byte);

    cambiaModalita(Modalita::tx);
    trasmissioneAck = true;
}


// Attiva la radio in modo che possa ricevere dei messaggi
//
int RFM69::iniziaRicezione() {
    uint32_t t = millis();
    while(trasmissioneAck || trasmissioneMessaggio) {
        // timeoutAck è un tempo arbitrario ma dell'ordine di grandezza giusto
        if(t < millis() - timeoutAck) break;
    }
    return cambiaModalita(Modalita::rx);
}


// Restituisce `true` se un nuovo messaggio è disponibile
//
bool RFM69::nuovoMessaggio() {
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

// Restituisce il valore RSSI del segnale dell'ultimo messaggio
//
int8_t RFM69::rssi() {
    return ultimoMessaggio.rssi;
}

// Restituisce l'"ora" di ricezione dell'ultimo messaggio
uint8_t RFM69::tempoRicezione() {
    return ultimoMessaggio.tempoRicezione;

}






// ### 3. ACK ### //


// Restituisce true se la classe sta asettando un ack
//
bool RFM69::aspettaAck() {
    if(attesaAck) {
        if(tempoUltimaTrasmissione < millis() - timeoutAck) {
            // timeout, termina l'attesa senza aver ricevuto l'ACK
            rinunciaAck();
        }
        // fine attesa
        return true;
    }
    // attesaAck è false
    return false;
}


// Restituisce true se la radio ha ricevuto un ACK per l'ultimo messaggio inviato
//
bool RFM69::ricevutoAck() {
    return ackRicevuto;
}


// Fingi di aver ricevuto un ACK. Se il vero ACK dovesse arrivare dopo l'esecuzione
// di questa funzione non avrà nessun effetto.
void RFM69::rinunciaAck() {
    attesaAck = false;
}






// ### 4. ISR ### //



// ISR reale, static, che chiama un interrupt handler (la funzione `isr()`) che non essendo static è legata all'istanza della radio
void RFM69::isrCaller() {
    pointerRadio->isr();
}



// Reagisce agli interrupt generati dalla radio sul suo pin DI0
//
void RFM69::isr() {
    // ## Modalità TX, interrupt su `PacketSent` ## //

    // È appena stato trasmesso un messaggio
    if(trasmissioneMessaggio) {
        spi.usaInIsr(true);
        if(attesaAck) cambiaModalita(Modalita::rx, false);
        else cambiaModalita(modalitaDefault, false);
        spi.usaInIsr(false);
        trasmissioneMessaggio = false;
        messaggiInviati++;
        return;
    }

    // È appena stato trasmesso un ACK
    if(trasmissioneAck) {
        spi.usaInIsr(true);
        cambiaModalita(modalitaDefault, false);
        spi.usaInIsr(false);
        trasmissioneAck = false;
        return;
    }


    // ## Modalità RX, interrupt su `PayloadReady` ## //

    if(modalita == Modalita::rx || modalita == Modalita::listen) {
        // # Non si sa ancora se è un vero messaggio o solo un ACK #

        spi.usaInIsr(true);

        Modalita modPrec = modalita; // rx o listen
        cambiaModalita(Modalita::standby, false);

        // Leggi i primi due bytes (lunghezza e intestazione)
        uint8_t lung = spi.leggiRegistro(RFM69_00_FIFO);
        Intestazione intest;
        intest.byte = spi.leggiRegistro(RFM69_00_FIFO);

        if(attesaAck) {
            // Il messaggio dovrebbe essere un ACK. È però possibile che l'altra
            // radio invii a sua volta un messaggio prima di inviare l'ACK
            // (l'ACK è inviato dalla funzione di lettura, chiamata dall'utente,
            // mentre il messaggio è scaricato dalla FIFO della radio dall'ISR
            // subito dopo la ricezione). Quindi se l'utente chiama prima invia()
            // e poi leggi() sull'altra radio, questa ricevereà prima un messaggio
            // e poi l'ACK atteso

            if(intest.bit.ack) {

                // # Il messaggio è un ACK # //

                // Calcola attesa attuale, ...
                durataUltimaAttesaAck = millis() - tempoUltimaTrasmissione;
                // ... massima ...
                if(durataUltimaAttesaAck > durataMassimaAttesaAck) {
                    durataMassimaAttesaAck = durataUltimaAttesaAck;
                }
                // ... e media
                nrAckRicevuti++;
                sommaAtteseAck += durataUltimaAttesaAck;

                attesaAck = false;
                ackRicevuto = true;
                cambiaModalita(modalitaDefault, false);
                spi.usaInIsr(false);
                // non c'è nient'altro da fare, un ACK non ha contenuto
                return;
            }
        }


        // # Il messaggio non è un ACK valido # //

        // Esiste anche la possibilità che un ACK arrivi inatteso (ad es. se
        // l'utente ci ha rinunciato). In tal caso la radio deve restare in
        // ascolto come se niente fosse, limitandosi a segnalare l'inutile ACK
        // (un ACK inaspettato segnala che c'è un problema nel programma).
        //
        // Siccome è possibile che la modalità fosse listen la funzione
        // `cambiaModalita()` non può essere usata in modo rapido (secondo
        // argomento `false`).
        else if(intest.bit.ack) {

            // # Il messaggio è un ACK indesiderato # //

            ackInattesi++;
            cambiaModalita(modPrec, true); // modPrec potrebbe essere `listen`
            spi.usaInIsr(false);
            return;
        }

        // # Il messaggio è un vero messaggio # //

        messaggioRicevuto = true;
        messaggiRicevuti++;

        // leggi tutti gli altri bytes
        spi.preparaTrasferimento();
        spi.trasferisciByte(RFM69_00_FIFO);
        for(int i = 0; i < lung - 1; i++) {
            buffer[i] = spi.trasferisciByte();
        }
        spi.terminaTrasferimento();

        ultimoMessaggio.tempoRicezione = millis();
        ultimoMessaggio.dimensione = lung - 1;
        ultimoMessaggio.intestazione.byte = intest.byte;
        ultimoMessaggio.rssi = -(spi.leggiRegistro(RFM69_24_RSSI_VALUE)/2);

        spi.usaInIsr(false);

        return;
    }

}






// ### 5. Impostazioni ### //


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
    uint8_t regOpMode = spi.leggiRegistro(RFM69_01_OP_MODE) & 0xE3;

    // Imposta i 3 bit che stabiliscono la modalità
    uint8_t codiceMod;
    switch(mod) {
        case Modalita::sleep:   codiceMod = 0x0;    break;
        // per entrare in modalità listen la radio deve essere in standby
        case Modalita::listen:
        case Modalita::standby: codiceMod = 0x1;    break;
        case Modalita::fs:      codiceMod = 0x2;    break;
        case Modalita::tx:      codiceMod = 0x3;    break;
        case Modalita::rx:      codiceMod = 0x4;    break;
    }
    regOpMode |= codiceMod << 2;

    // *Listen* (1/3) - Disattiva
    if (modalita == Modalita::listen) {
        regOpMode &= !(1 << 6); // clear ListenOn
        regOpMode |= 1 << 5; // set ListenAbort
    }

    //Scrivi il registro RegOpMode
    spi.scriviRegistro(RFM69_01_OP_MODE, regOpMode);

    // *Listen* (2/3) - Disattiva
    if (modalita == Modalita::listen) {
        regOpMode &= !(1 << 5); // clear ListenAbort
        // Scrivi di nuovo RegOpMode senza il bit ListenAbort
        spi.scriviRegistro(RFM69_01_OP_MODE, regOpMode);
    }


    // Impostazioni riguardanti la trasmissione high power (> 17dbM)

    // La radio deve entrare in modalità high power tx
    else if (highPower && (mod ==  Modalita::tx)) highPowerSettings(true);
    // La radio deve uscire dalla modalità high power tx
    if (highPower && (modalita == Modalita::tx)) highPowerSettings(false);


    if(aspetta) {
        // Aspetta che la radio sia pronta (questa flag è 0 durante il cambiamento di
        // modalità)
        unsigned long inizioAttesa = millis();
        while(!spi.leggiRegistro(RFM69_27_IRQ_FLAGS_1) & RFM69_FLAGS_1_MODE_READY) {
            if(inizioAttesa + 100 < millis()) return Errore::modTimeout;
        }
    }

    // *Listen* (3/3) - Attiva
    // Per attivare la modalita listen basta scrivere il bit 6 dopo aver
    // messo la radio in standby
    if(mod == Modalita::listen) {
        regOpMode = spi.leggiRegistro(RFM69_01_OP_MODE);
        regOpMode |=  1 << 6;
        spi.scriviRegistro(RFM69_01_OP_MODE, regOpMode);
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

    spi.scriviRegistro(RFM69_25_DIO_MAPPING_1, dio0 << 6);

    // Ricorda la modalita attuale della radio
    modalita = mod;

    /*switch(mod) {
    case Modalita::tx : Serial.print("_tx_"); break;
    case Modalita::rx : Serial.print("_rx_"); break;
    case Modalita::standby : Serial.print("_standby_"); break;
    case Modalita::sleep : Serial.print("_sleep_"); break;
    case Modalita::listen : Serial.print("_listen_"); break;
    default: break;
}*/

return Errore::ok;
}


// Scrive le impostazioni "high power" (per l'utilizzo del modulo con una potenza
// superiore a 17 dBm).
// Vedi il commento della funzione di inizializzazione 'impostaPotenzaTx()' per
// dettagli.
//
void RFM69::highPowerSettings(bool attiva) {

    if(attiva) {
        spi.scriviRegistro(RFM69_13_OCP, 0x0F); // RegOcp (0x13) -> 0x0F
        spi.scriviRegistro(RFM69_5A_TEST_PA_1, 0x5D); // RegTestPa1 (0x5A) -> 0x5D
        spi.scriviRegistro(RFM69_5C_TEST_PA_2, 0x7C); // RegTestPa2 (0x5C) -> 0x7C
    }
    else {
        spi.scriviRegistro(RFM69_13_OCP, 0x1A); // RegOcp (0x13) -> 0x1A
        spi.scriviRegistro(RFM69_5A_TEST_PA_1, 0x55); // RegTestPa1(0x5A) -> 0x55
        spi.scriviRegistro(RFM69_5C_TEST_PA_2, 0x70); // RegTestPa2(0x5C) -> 0x70
    }
}




int  RFM69::listen() {
    return cambiaModalita(Modalita::listen);
}
int  RFM69::standby() {
    return cambiaModalita(Modalita::standby);
}
int  RFM69::sleep() {
    return cambiaModalita(Modalita::sleep);
}

void RFM69::defaultSleep() {
    modalitaDefault = Modalita::sleep;
}
void RFM69::defaultStandby() {
    modalitaDefault = Modalita::standby;
}
void RFM69::defaultListen() {
    modalitaDefault = Modalita::listen;
}
void RFM69::defaultRx() {
    modalitaDefault = Modalita::rx;
}

void RFM69::impostaTimeoutAck(uint16_t tempoMs) {
    timeoutAck = tempoMs;
}





//Imposta la potenza di trasmissione del segnale radio
//
bool RFM69::impostaPotenzaTx(int dBm) {


    // Controlla che la potenza sia all'interno dei limiti
    if(dBm < -18) return false;
    if(dBm > 20) return false;

    bool pa0, pa1, pa2;
    int outPow; //solo i primi 5 bit possono essere usati.


    if(dBm <= -2) {      //opzione 1
        pa0 = 1;
        pa1 = 0;
        pa2 = 0;
        outPow = dBm + 18;
        highPower = false;
    }
    else if(dBm <= 13) { //opzione 2
        pa0 = 0;
        pa1 = 1;
        pa2 = 0;
        outPow = dBm + 18;
        highPower = false;
    }
    else if(dBm <= 17) { //opzione 3
        pa0 = 0;
        pa1 = 1;
        pa2 = 1;
        outPow = dBm + 14;
        highPower = false;
    }
    else if(dBm <= 20) { //opzione 4
        pa0 = 0;
        pa1 = 1;
        pa2 = 1;
        outPow = dBm + 11;
        highPower = true;
    }

    //prepara il byte che sarà scritto nel registro e invia alla radio
    uint8_t paLevel = 0;
    paLevel = (pa0 << 7) | (pa1 << 6) | (pa2 << 5) | outPow;

    spi.scriviRegistro(RFM69_11_PA_LEVEL, paLevel);

    return true;
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
            case Errore::inviaTimeoutTxPrecedente :
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
        serial.println(F("ok!")); break;
        case Errore::errore :
        serial.println(F("errore")); break;

        case Errore::initTroppeRadio :
        serial.println(F("troppe radio")); break;
        case Errore::initInitSPIFallita :
        serial.println(F("init SPI fallita")); break;
        case Errore::initNessunaRadioConnessa :
        serial.println(F("nessuna radio connessa")); break;
        case Errore::initVersioneRadioNon0x24 :
        serial.println(F("versione radio non 0x24")); break;
        case Errore::initPinInterruptNonValido :
        serial.println(F("pin interrupt non valido")); break;
        case Errore::initErroreImpostazione :
        serial.println(F("errore impostazione")); break;

        case Errore::inviaMessaggioVuoto :
        serial.println(F("messaggio vuoto")); break;
        case Errore::inviaTimeoutTxPrecedente :
        serial.println(F("il mess. prec. non è ancora partito")); break;

        case Errore::leggiNessunMessaggio :
        serial.println(F("nessun messaggio")); break;
        case Errore::leggiArrayTroppoCorta :
        serial.println(F("array troppo corta")); break;

        case Errore::modImpossibile:
        serial.println(F("impossibile cambiare")); break;
        case Errore::modTimeout :
        serial.println(F("timeout")); break;
        default:
        serial.print(F("Errore sconosciuto ["));
        serial.print(errore);
        serial.println(F("]"));
        break;
    }
}
