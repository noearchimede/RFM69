/*! @file
@brief Programma di test con lo stesso codice per entrambe le radio

Questo programma è identico per le due radio (tranne che per l'inizializzazione
dei pin, ovviamente). Ogni radio in ogni istante ha una certa probabilità di inviare
un messaggio (impostabile dall'utente come numero di messaggi in media al minuto).
Tutti i messaggi contengono una richiesta di ACK e sono quindi considerati persi
se l'altra radio non risponde.
Il programma stampa sul monitor seriale il numero di messaggi trasmessi con successo
(cioé a cui la radio ricevente ha risposto con un ACK), il numero di messaggi persi
e la percentuale di successo (mess. trasmessi / mess. totali).

*/

#include <Arduino.h>
#include "RFM69.h"


// #define MODULO_1 o MODULO_2 per compilare uno dei due programmi
//------------------------------------------------------------------------------
//#define MODULO_1
#define MODULO_2
//------------------------------------------------------------------------------


// Impostazioni
//------------------------------------------------------------------------------

// Numero medio di messaggi che ogni radio invierà ogni minuto. Usa AUTO per
// entrare in un programma automatico di test a varie frequenze di trasmissione.
#define AUTO 0

const uint16_t messaggiPerMin = AUTO;
const uint8_t lunghezzaMessaggi = 4;
const uint8_t timeoutAck = 50;
const uint16_t tolleranza = 10;
const uint16_t durataMinimaTest = 20000;
// Numero del modulo radio che condurrà lesperimento (bisogna leggerne l'output seriale)
#define BOSS 2


// t
#ifdef MODULO_1
RFM69 radio(2, 3); // Pin SS, pin Interrupt, (eventualmente pin Reset)
#define LED_TX 5
#define LED_ACK 4
#endif

// q
#ifdef MODULO_2

RFM69 radio(A2, 3, A3); // Pin SS, pin Interrupt, (eventualmente pin Reset)
#define LED_TX 8
#define LED_ACK 7
#endif
//------------------------------------------------------------------------------

#ifdef MODULO_1
#define ID_RADIO 1
#endif
#ifdef MODULO_2
#define ID_RADIO 2
#endif


// ### Variabili e costanti globali ### //


// il capo è la radio a cui È connesso l'utente tramite monitor serial. Non fa
// di diverso dall'altra, salvo che le impone la variabile messPerMin. I dati
// generati dall'altra (che potrebbero essere letti con una seconda connessione
// seriale) non hanno senso.
bool isBoss;

uint32_t tAccensioneRx, tAccensioneTx;

uint32_t tInizio;

uint16_t messInviati = 0, messNonInviati = 0, messaggiRicevuti = 0;
uint16_t indiceSuccesso = 0;
uint8_t  nrElaborazioni = 0;
uint16_t indiciSuccessoPrec[10];
uint32_t messPerMin, messPerMinEffettivi;
float deriv;

uint8_t nrTest = 0;
constexpr uint8_t nrTestMax = 20;
uint16_t riassunto[nrTestMax][4];
enum class elemRiass {mpmPrevisti,mpmEffettivi,messTot,durataMsb,durataLsb,successo};

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
bool deveInviare();
void stampaRiassunto();
void stampaLarghezzaFissa(uint32_t, uint8_t, char = ' ');


// ### Funzioni ### //


void setup() {

    Serial.begin(115200);

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

    if(BOSS == ID_RADIO) {

        isBoss = true;

        Serial.println("Questa radio conduce l'esperimento.");
        Serial.println();
        Serial.print("Aspetto altra radio... ");
        // Aspetta per 30 secondi che l'altra radio risponda (10 tentativi, uno al secondo)
        uint8_t mess[1];
        radio.impostaTimeoutAck(1000);
        if(radio.inviaFinoAck(30, mess, 1) != 0) {
            Serial.println("nessuna radio trovata.");
            while(true);
        }
        Serial.print("ok, inizio... ");
        for(int i = 3; i; i--) {
            Serial.print(i);
            Serial.print("-");
            delay(1000);
        }
        Serial.print("0");
    }
    else {
        Serial.println("I risultati del test sono stampati dall'altra radio.");
        isBoss = false;
        while(!radio.nuovoMessaggio());
    }

    radio.impostaTimeoutAck(timeoutAck);

    if(messaggiPerMin == AUTO) imposta(50);
    else imposta(messaggiPerMin);
}



void loop() {

    leggi();

    if(deveInviare()) invia();

    if(isBoss && novita) {
        novita = false;
        elaboraStatistiche();
        stampaNovita();
    }

    if(isBoss && statStabili) {
        // Modalità automatica
        if(messaggiPerMin == AUTO) {
            statStabili = false;
            salvaStatistiche();
            imposta(messPerMin += 50);
        }
        // Modalità manuale, test a una sola frequenza di trasmissione
        else if(statStabili) fineTest = true;

    }

    if(isBoss && fineTest) {
        stampaRiassunto();
        digitalWrite(LED_TX, LOW);
        digitalWrite(LED_ACK, LOW);
        radio.sleep();
        while(true);
    }

    spegniLed();
    delay(1);
}





void invia() {

    novita = true;

    // crea un messaggio e inserisci la frequenza e poi numeri casuali
    uint8_t mess[lunghezzaMessaggi];
    mess[0] = messPerMin >> 8;
    mess[1] = messPerMin;
    for(int i = 2; i < lunghezzaMessaggi; i++) mess[i] = (uint8_t)(micros() % (500 - i));

    // Invia
    radio.inviaConAck(mess, lunghezzaMessaggi);
    while(radio.aspettaAck());

    accendiLed(LED_TX);

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
    // leggi il messaggio
    int erroreLettura = radio.leggi(mess, lung);

    // La radio non-Boss deve conosce i primi due bytes
    if(!isBoss) messPerMin = ((uint16_t) mess[0] << 8) | mess[1];

    // stampa un eventuale errore
    if(erroreLettura) radio.stampaErroreSerial(Serial, erroreLettura);

}





void accendiLed(uint8_t led) {
    if (led) digitalWrite(led, HIGH);
    if(led == LED_TX) tAccensioneTx = millis();
    if(led == LED_ACK) tAccensioneRx = millis();
}





void spegniLed() {
    if(millis() - tAccensioneRx > 25) digitalWrite(LED_TX, LOW);
    if(millis() - tAccensioneTx > 25) digitalWrite(LED_ACK, LOW);
}





void stampaNovita() {
    Serial.print("tx: ");
    stampaLarghezzaFissa((messInviati + messNonInviati), 4);
    Serial.print(" | ");
    Serial.print("rx: ");
    stampaLarghezzaFissa(messaggiRicevuti, 4);
    Serial.print(" | ");

    Serial.print("t: ");
    stampaLarghezzaFissa((millis() - tInizio)/1000, 3);
    Serial.print("s | ack: ");
    stampaLarghezzaFissa(messInviati, 3);
    Serial.print(" | mess/min:");
    stampaLarghezzaFissa(messPerMinEffettivi, 5);
    Serial.print(" -> ");
    stampaLarghezzaFissa(messPerMin, 5);
    Serial.print(" | % ack: ");
    Serial.print((float)indiceSuccesso/100);
    Serial.println("%");
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

        if(deriv < 0) deriv = -deriv;

        if(deriv < tolleranza && (millis() - tInizio) > durataMinimaTest)
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
    uint32_t t = millis() - tInizio;
    riassunto[nrTest][(int)elemRiass::durataMsb] = t >> 16;
    riassunto[nrTest][(int)elemRiass::durataLsb] = t;
    // Percentuale successo
    riassunto[nrTest][(int)elemRiass::successo] = indiceSuccesso;

    nrTest++;

    if(!isBoss)
    if(indiceSuccesso < 500 || nrTest == nrTestMax)
    fineTest = true;
}





void imposta(uint32_t mpm) {
    messPerMin = mpm;

    messaggiRicevuti = 0;
    messInviati = 0;
    messNonInviati = 0;
    indiceSuccesso = 0;
    nrElaborazioni = 0;

    Serial.println();
    Serial.print(nrTest);
    Serial.print(". Messaggi/minuto: ");
    Serial.println(mpm);
    Serial.println();

    tInizio = millis();

}





bool deveInviare() {
    return ((((micros() >> 3) + (micros() << 3)) % 60000) < messPerMin);

}





void stampaRiassunto() {

    // calcolo durata totale test in ms
    uint32_t tTot = 0;
    for(int i = 0; i < nrTest; i++) tTot += ((uint32_t)(riassunto[i][(int)elemRiass::durataMsb]) << 16) | riassunto[i][(int)elemRiass::durataLsb];

    Serial.println();
    Serial.println();
    for(int i = 0; i < 25; i++) Serial.print('*');
    Serial.println();

    Serial.print("Test completato in ");
    Serial.print(tTot/60000);
    Serial.print("minuti e ");
    Serial.print((tTot/1000)%60);
    Serial.println(" secondi.");
    Serial.print("Tutti i messaggi erano lunghi ");
    Serial.print(lunghezzaMessaggi);
    Serial.println(" bytes.");

    Serial.println();
    Serial.println();
    //             0---------1---------2---------3---------4---------5---------6
    //             -123456789-123456789-123456789-123456789-123456789-123456789-
    Serial.println(" # | mess/min | m/m esatti | durata | mess inviati | successo | byte/min |");

    for(int i = 0; i < 57; i++) Serial.print('-');
    for(int i = 0; i < nrTest; i++) {

        stampaLarghezzaFissa(nrTest, 2);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[nrTest][(int)elemRiass::mpmPrevisti], 8);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[nrTest][(int)elemRiass::mpmEffettivi], 10);
        Serial.print(" | ");
        stampaLarghezzaFissa(((uint32_t)(riassunto[i][(int)elemRiass::durataMsb]) << 16) | riassunto[nrTest][(int)elemRiass::durataLsb], 6);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[nrTest][(int)elemRiass::messTot], 12);
        Serial.print(" | ");
        stampaLarghezzaFissa(riassunto[nrTest][(int)elemRiass::successo] / 100, 5);
        Serial.print(".");
        stampaLarghezzaFissa(riassunto[nrTest][(int)elemRiass::successo] % 100, 2, '0');
        Serial.print(" | ");
        stampaLarghezzaFissa((riassunto[nrTest][(int)elemRiass::messTot] * riassunto[nrTest][(int)elemRiass::successo] * riassunto[nrTest][(int)elemRiass::mpmEffettivi] * lunghezzaMessaggi) / 10000, 8);
        Serial.print(" |");
    }
    for(int i = 0; i < 57; i++) Serial.print('-');

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
