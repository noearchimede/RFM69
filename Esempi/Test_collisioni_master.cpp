/*! @file
@brief Test collisioni tra messaggi

Questo programma permette di stimare il numero di messaggi persi in un canale di
comunicazione tra due radio molto utilizzato (centinaia di messaggi al minuto).

Le due radio svolgono lo stesso compito: inviano messaggi a intervalli casuali,
che tendono però ad una frequenza determinata, e controllano se ci sono messaggi
da leggere. Tutti i messaggi trasmessi richiedono un ACK.
Questo programma, dato un certo numero di messaggi al minuto, stabilisce la
percentuale di messaggi persi. Trovata la percentuale per una certa frequenza di
trasmissione (cioè quando ogni calcolo della percentuale da un risultato simile
ai precedenti) aumenta questo ultimo valore ed esegue lo stesso calcolo. Prosegue
così fino a che le due radio emettono senza sosta e non possono quindi ricevere
i messaggi dell'altra e trasmettere un ACK in risposta. A quel punto stampa una
tabella riassuntiva.

Questo programma è per la radio "master". Per leggere i risultati del test è
necessario connettere un monitor seriale a questa radio (a 115200 Baud).

*/

#include <Arduino.h>
#include "RFM69.h"


// Impostazioni
//------------------------------------------------------------------------------

const uint8_t lunghezzaMessaggi = 4; // deve essere uguale sull'altra radio
const uint8_t timeoutAck = 50;       // deve essere uguale sull'altra radio

// Più è piccolo più il test sarà preciso (e lungo)
const uint16_t tolleranza = 6000;
// Durata minima del test di una singola frequenza
const uint32_t durataMinimaTest = 300;
// Frequenza di trasmissione iniziale (messaggi al minuto)
const uint16_t frequenzaTxIniziale = 50;
// Incremento della frequenza ad ogni test
const uint16_t incrementoFrequenzaTx = 50;
// Numero massimo di test
const uint8_t nrTestMax = 50;

// Pin SS, pin Interrupt, (eventualmente pin Reset)
RFM69 radio(2, 3);

#define LED_TX 5
#define LED_ACK 4

//------------------------------------------------------------------------------



// ### Variabili e costanti globali ### //


uint32_t tAccensioneRx, tAccensioneTx;
uint32_t tInizio;
uint32_t microsInviaPrec;
uint16_t messInviati = 0, messNonInviati = 0, messaggiRicevuti = 0;
uint16_t indiceSuccesso = 0;
uint8_t  nrElaborazioni = 0;
uint16_t indiciSuccessoPrec[10];
uint32_t messPerMin, messPerMinEffettivi;
float deriv;

uint8_t nrTest = 0;
//uint16_t riassunto[nrTestMax][5];
enum class elemRiass {mpmPrevisti,mpmEffettivi,messTot,durata,successo};

bool novita = false;
bool statStabili = false;
bool fineTest = false;



// ### Prototipi ### //

void invia();
void leggi();
void accendiLed(uint8_t);
void spegniLed();
void stampaNovita();
void elaboraStatistiche();
void salvaStatistiche();
void imposta(uint32_t);
bool numeroCasuale(uint32_t);
void stampaRiassunto();
void stampaLarghezzaFissa(uint32_t, uint8_t, char = ' ');
void fineProgramma();

// ### Funzioni ### //

 uint16_t riassunto[17][5] = {
 {50,95,6,3,10000},
 {100,83,16,11,9375},
 {150,258,6,1,10000},
 {200,105,6,3,10000},
 {250,580,6,0,10000},
 {300,344,98,17,8061},
 {350,411,10,1,6000},
 {400,677,6,0,10000},
 {450,390,150,23,7400},
 {500,468,151,19,7350},
 {550,470,36,4,5277},
 {600,661,61,5,6393},
 {650,498,57,6,5964},
 {700,627,35,3,4571},
 {750,811,44,3,4772},
 {800,880,73,4,547},
 {850,980,6,0,0}};

void setup() {

    Serial.begin(115200);



    nrTest = 16;
    messPerMinEffettivi = 980;

    stampaRiassunto();


    while(true);












    Serial.println("\n\n\n\nRFM69 - Test collisione messaggi\n\n\n");

    if(LED_TX) pinMode(LED_TX, OUTPUT);
    if(LED_ACK) pinMode(LED_ACK, OUTPUT);

    // Inizializza la radio. Deve essere chiamato una volta all'inizio del programma.
    // Se come secondo argomento si fornisce un riferimento a Serial, la funzione
    // stampa il risultato dell'inizializzazione (riuscita o no).
    radio.inizializza(lunghezzaMessaggi, Serial);

    // con `defaultRicezione()` basta chiamare una volta sola `iniziaRicezione()`
    radio.defaultRicezione();

    radio.iniziaRicezione();


    Serial.println("Questa radio conduce l'esperimento.");
    Serial.println();
    Serial.print("Aspetto altra radio..");
    // Aspetta per 30 secondi che l'altra radio risponda (10 tentativi, uno al secondo)
    uint8_t mess[2] = {0,0};
    bool connessioneOk = false;
    for(int i = 0; i < 20; i++) {
        Serial.print(".");
        radio.inviaConAck(mess, 2);
        delay(1000);
        if(radio.ricevutoAck()) {
            connessioneOk = true;
            break;
        }
    }

    if(!connessioneOk) {
        Serial.println(" nessuna radio trovata.");
        while(true);
    }

    Serial.print("ok, inizio... ");
    for(int i = 3; i; i--) {
        Serial.print(i);
        Serial.print(" ");
        delay(1000);
    }
    Serial.print("0");


    radio.impostaTimeoutAck(timeoutAck);

    imposta(frequenzaTxIniziale);

    //Invia alcuni messaggi per "svegliare" l'altra radio
    radio.inviaConAck(mess, 2);
    delay(100);
    radio.inviaConAck(mess, 2);
    delay(100);
    radio.inviaConAck(mess, 2);
    delay(100);

    randomSeed(micros());
}



void loop() {

    leggi();
    invia();


    if(novita) {
        novita = false;
        elaboraStatistiche();
        stampaNovita();
    }

    if(statStabili) {
        statStabili = false;
        salvaStatistiche();
        if(fineTest) {
            stampaRiassunto();
            fineProgramma();
        }
        imposta(messPerMin += incrementoFrequenzaTx);
    }



    spegniLed();
    delay(1);
}





void invia() {

    // Decidi  caso se inviare o no, con una probabilità tale da avvicinarsi alla
    // frequenza di invio `messAlMinuto`.

    // Tempo trascorso dall'ultima chiamata ad `invia()`, in microsecondi
    uint32_t t = micros();
    uint32_t deltaT = (t - microsInviaPrec);
    microsInviaPrec = t;
    // `decisione` vale `true` con una probabilita di [messPerMin * deltaT / 1 min]
    bool decisione = ((messPerMin * deltaT) > random(60000000));

    if(!decisione) return;

    invia();

    novita = true;

    // crea un messaggio e inserisci la frequenza e poi numeri casuali
    uint8_t mess[lunghezzaMessaggi];
    mess[0] = messPerMin >> 8;
    mess[1] = messPerMin;
    for(int i = 2; i < lunghezzaMessaggi; i++) mess[i] = (uint8_t)(micros() % (500 - i));

    // Invia
    accendiLed(LED_TX);
    radio.inviaConAck(mess, lunghezzaMessaggi);
    while(radio.ackInSospeso());

    if(radio.ricevutoAck()) {
        messInviati++;
        accendiLed(LED_ACK);
    }
    else {
        messNonInviati++;
    }

}





void leggi() {

    // c'è un nuovo messaggio? se non c'è return
    if(!radio.nuovoMessaggio()) return;

    messaggiRicevuti++;

    // ottieni la dimensione del messaggio ricevuto
    uint8_t lung = radio.dimensioneMessaggio();
    // crea un'array in cui copiarlo
    uint8_t mess[lung];

    accendiLed(LED_ACK);

    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);

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



void imposta(uint32_t mpm) {
    messPerMin = mpm;

    messaggiRicevuti = 0;
    messInviati = 0;
    messNonInviati = 0;
    indiceSuccesso = 0;
    nrElaborazioni = 0;

    Serial.println();
    Serial.println();
    Serial.print(nrTest + 1);
    Serial.print(". Messaggi/minuto: ");
    Serial.println(mpm);
    Serial.println();

    tInizio = millis();

}




void elaboraStatistiche() {

    // Calcolo messaggi al minuto
    messPerMinEffettivi = ((uint32_t)(messInviati + messNonInviati)*60000)/(millis() - tInizio);

    // Calcolo percentuale di successo
    // Nelle variabili sottostanti il valore 100 rappresenta un punto percentuale
    if((messInviati + messNonInviati) > 0)
    indiceSuccesso = (uint32_t)messInviati * 10000 / (messInviati + messNonInviati);

    // Determinazione della situazione di stabilità (usa il metodo Savitzky–Golay
    // per calcolare la derivata della funzione del successo nel tempo)
    deriv = 0;
    if(nrElaborazioni >= 5) {
        deriv += (float)indiciSuccessoPrec[0] *  1;
        deriv += (float)indiciSuccessoPrec[1] * -8;
        //deriv += indiciSuccessoPrec[2] * 0;
        deriv += (float)indiciSuccessoPrec[3] *  8;
        deriv += (float)indiceSuccesso        * -1;
        deriv /= 12;
        deriv /= 1000/(float)messPerMin;
        if(deriv < 0) deriv = -deriv;

        if(deriv < (float)tolleranza/1000 && (millis() - tInizio) > durataMinimaTest)
        statStabili = true;
    }


    // Aggiornamento dell'array
    for(int i = 0; i < 4; i++)
    indiciSuccessoPrec[i] = indiciSuccessoPrec[i+1];
    indiciSuccessoPrec[4] = indiceSuccesso;


    nrElaborazioni++;
}



void salvaStatistiche()  {
    // Frequenza prevista
    riassunto[nrTest][(int)elemRiass::mpmPrevisti] = messPerMin;
    // Frequenza effettiva
    riassunto[nrTest][(int)elemRiass::mpmEffettivi] = messPerMinEffettivi;
    // Messaggi totali
    riassunto[nrTest][(int)elemRiass::messTot] = messInviati + messNonInviati;
    // Durata del test
    riassunto[nrTest][(int)elemRiass::durata] = (millis() - tInizio) / 1000;
    // Percentuale successo
    riassunto[nrTest][(int)elemRiass::successo] = indiceSuccesso;

    nrTest++;

    if(indiceSuccesso == 0 || nrTest == nrTestMax)
    fineTest = true;
}



void stampaNovita() {
    Serial.print("tx: ");
    stampaLarghezzaFissa((messInviati + messNonInviati), 4);
    Serial.print("  |  ");
    Serial.print("rx: ");
    stampaLarghezzaFissa(messaggiRicevuti, 4);
    Serial.print("  |  ");

    Serial.print("t: ");
    stampaLarghezzaFissa((millis() - tInizio)/1000, 3);
    Serial.print("s  |  ack: ");
    stampaLarghezzaFissa(messInviati, 3);
    Serial.print("  |  mess/min:");
    stampaLarghezzaFissa(messPerMinEffettivi, 5);
    Serial.print(" -> ");
    stampaLarghezzaFissa(messPerMin, 5);
    Serial.print("  |  % ack: ");
    Serial.print((float)indiceSuccesso/100);
    Serial.println("%");
}




void stampaRiassunto() {

    for(int i = 6; i; i--) Serial.println();
    for(int i = 71; i; i--) Serial.print('*');
    Serial.println();

    // Calcolo totali
    uint32_t tTot = 0;
    for(int i = 0; i < nrTest; i++) tTot += riassunto[i][(int)elemRiass::durata];
    uint32_t messTot = 0;
    for(int i = 0; i < nrTest; i++) messTot += riassunto[i][(int)elemRiass::messTot];
    const uint16_t mpmMax = messPerMinEffettivi;

    Serial.print("Test completato in ");
    Serial.print(tTot/60);
    Serial.print(" minuti e ");
    Serial.print(tTot%60);
    Serial.print(" secondi.");
    Serial.println();
    Serial.print("Questa radio ha inviato ");
    Serial.print(messTot);
    Serial.print(" messaggi di ");
    Serial.print(lunghezzaMessaggi);
    Serial.print(" bytes a ");
    Serial.print(nrTest);
    Serial.print(" frequenze di\ntrasmissione diverse comprese tra ");
    Serial.print(frequenzaTxIniziale);
    Serial.print(" e ");
    Serial.print(mpmMax);
    Serial.print(" messaggi al minuto");
    Serial.println();

    Serial.println();
    Serial.println();
    Serial.println("Risultati");
    Serial.println();
    //             0---------1---------2---------3---------4---------5---------6---------7
    //             -123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    Serial.println("   | #  | durata | mess inviati | mess/min | m/m esatti | successo |");

    for(int i = 0; i < 68; i++) Serial.print('-'); Serial.println();
    for(int i = 0; i < nrTest; i++) {
        Serial.print("   | ");
        stampaLarghezzaFissa(i+1, 2);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::durata], 4);
        Serial.print(" s | ");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::messTot], 12);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::mpmPrevisti], 8);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::mpmEffettivi], 10);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::successo] / 100, 5);
        Serial.print(".");
        stampaLarghezzaFissa(riassunto[i][(int)elemRiass::successo] % 100, 2, '0');
        Serial.print(" |");
        Serial.println();
    }
    for(int i = 0; i < 68; i++) Serial.print('-'); Serial.println();

    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println("Percentuale di successo per frequenza di trasmissione");
    Serial.println();

    // Stampa grafico percentuale/frequenzaTx

    const uint8_t larghezza = 60;
    const uint8_t altezza = 16; // multiplo di 2, 3 o 4 (meglio 4)
    uint8_t divisoreAltezza;
    if      (altezza % 4 == 0) divisoreAltezza = 25;
    else if (altezza % 3 == 0) divisoreAltezza = 33;
    else if (altezza % 2 == 0) divisoreAltezza = 50;

    Serial.println("    %");

    // Stampa grafico
    for(int y = altezza; y > 0; y--) {

        if((y * 100 / altezza) % divisoreAltezza == 0 || y == altezza)
        stampaLarghezzaFissa(y * 100 / altezza, 3);
        else Serial.print("   ");
        Serial.print(" | ");

        for(int x = 0, test = 0; x < larghezza; x++) {

            // Controlla se uno qualsiasi dei test è stato effettuato ai mpm su questa ascissa
            for(int i = 0; i < nrTest; i++) {
                uint8_t ascissa = larghezza * riassunto[i][(int)elemRiass::mpmEffettivi] / mpmMax;

                if(x == ascissa) {
                    // Se siamo nel punto in cui i dati sono maggiori per quella ascissa
                    if(y == (altezza * (riassunto[i][(int)elemRiass::successo] / 100)) /100) {
                        // Stampa un punto
                        Serial.print("*");
                    }
                }
            }

            // se non è stato stampato il punto
            Serial.print(" ");

            test++;
        }
        Serial.println();
    }

    Serial.print("  0 +");
    for(int x = 0; x < larghezza; x++) Serial.print("-");
    Serial.println("  mess/min");

    Serial.print("    0");

    uint8_t distanzaPunti;
    uint16_t moltiplicatore;
    moltiplicatore = 50;
    distanzaPunti = larghezza *  moltiplicatore / mpmMax;
    if(distanzaPunti < 7) {
        moltiplicatore = 100;
        distanzaPunti = larghezza * moltiplicatore / mpmMax;
    }
    if(distanzaPunti < 7) {
        moltiplicatore = 200;
        distanzaPunti = larghezza * moltiplicatore / mpmMax;
    }
    if(distanzaPunti < 7) {
        moltiplicatore = 500;
        distanzaPunti = larghezza * moltiplicatore / mpmMax;
    }

    for(int x = 1; x * distanzaPunti < larghezza; x++) {
        for(int i = 0; i < distanzaPunti - 3; i++)
        Serial.print(" ");
        stampaLarghezzaFissa(x * moltiplicatore, 3);
    }

    /*
    Serial.println();
    Serial.println();
    Serial.print("{");
    for(int a = 0; a < nrTest; a++) {
        Serial.print("{");
        for(int b = 0; b < 5; b++) {
            Serial.print(riassunto[a][b]);
            if(b < 4) Serial.print(",");
        }
        Serial.print("}");
        if(b < nrTest - 1) Serial.print(",");
    }
    Serial.print("}");
    Serial.println();
    Serial.println();
    */
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
}




void stampaLarghezzaFissa(uint32_t numero, uint8_t larghezza, char riempimento) {
    char str[10]; // 10 è il numero massimo di cifre in un uint32_t
    itoa (numero, str, 10);
    int cifre = strlen(str);
    for(int i = 0; i < (larghezza - cifre); i++) Serial.print(riempimento);
    Serial.print(str);
}



void fineProgramma() {

    Serial.println();
    Serial.print("Spengo l'altra radio..");
        // Per al massimo 10 secondi cerca di spegnere l'altra radio
    uint8_t mess[3] = {0,0,0x7B};
    bool ok = false;
    for(int i = 0; i < 20; i++) {
        Serial.print(".");
        radio.inviaConAck(mess, 3);
        delay(1000);
        if(radio.ricevutoAck()) {
            ok = true;
            break;
        }
    }
    if (ok) {
        Serial.println(" spenta, spengo questa... ");
    }
    else {
        Serial.println(" comunicazione intrrotta, spegnere l'altra radio manualmente.");
        Serial.print("Spengo questa radio... ");
    }

    radio.sleep();

    Serial.println(" spenta.");

    Serial.println();
    Serial.println();
    Serial.println(" Fine test collisioni messaggi.");
    Serial.println();
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
