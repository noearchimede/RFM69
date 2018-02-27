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
const uint16_t tolleranza = 1000;
// Durata minima del test di una singola frequenza
const uint16_t durataMinimaTest = 20000;
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
uint16_t riassunto[nrTestMax][5];
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


    Serial.println("Questa radio conduce l'esperimento.");
    Serial.println();
    Serial.print("Aspetto altra radio... ");
    // Aspetta per 30 secondi che l'altra radio risponda (10 tentativi, uno al secondo)
    uint8_t mess[2] = {0,0};
    radio.impostaTimeoutAck(1000);
    int x = radio.inviaFinoAck(30, mess, 2);
    radio.stampaErroreSerial(Serial,x);
    if(x != 0) {
        Serial.println("nessuna radio trovata.");
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
    uint32_t deltaT = (micros() - microsInviaPrec);
    // Numero casuale compreso tra 0 e 60 milioni (60'000'000 microsecondi = 1 minuto)
    uint32_t n = (micros() * microsInviaPrec + micros()) % 60000000;
    // Non servirà più, aggiorna il tempo
    microsInviaPrec = micros();
    // `decisione` vale `true` con una probabilita di [messPerMin * deltaT / 1 min]
    bool decisione = ((messPerMin * deltaT) > n);

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
    while(radio.aspettaAck());

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




void stampaRiassunto() {

    Serial.println();
    Serial.println();
    for(int i = 0; i < 25; i++) Serial.print('*');
    Serial.println();

    // Calcolo totali
    uint32_t tTot = 0;
    for(int i = 0; i < nrTest; i++) tTot += riassunto[i][(int)elemRiass::durata];
    uint32_t messTot = 0;
    for(int i = 0; i < nrTest; i++) messTot += riassunto[i][(int)elemRiass::messTot];

    Serial.print("Test completato in ");
    Serial.print(tTot/60);
    Serial.print(" minuti e ");
    Serial.print(tTot%60);
    Serial.println(" secondi.");
    Serial.print("Questa radio ha inviato ");
    Serial.print(messTot);
    Serial.print(" messaggi di");
    Serial.print(lunghezzaMessaggi);
    Serial.println(" bytes.");

    Serial.println();
    Serial.println();
    //             0---------1---------2---------3---------4---------5---------6
    //             -123456789-123456789-123456789-123456789-123456789-123456789-
    Serial.println("| #  | durata | mess inviati | mess/min | m/m esatti | successo |");

    for(int i = 0; i < 65; i++) Serial.print('-'); Serial.println();
    for(int i = 0; i < nrTest; i++) {
        Serial.print("| ");
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
    for(int i = 0; i < 65; i++) Serial.print('-'); Serial.println();

    Serial.println();
    Serial.println();
    Serial.println();

    // Stampa grafico percentuale/frequenzaTx
    const uint8_t larghezza = 80;
    const uint8_t altezza = 16; // multiplo di 2, 3 o 4 (meglio 4)
    const uint16_t mpmMax = riassunto[nrTest][(int)elemRiass::mpmEffettivi];
    uint8_t divisoreAltezza;
    if      (altezza % 4 == 0) divisoreAltezza = 25;
    else if (altezza % 3 == 0) divisoreAltezza = 33;
    else if (altezza % 2 == 0) divisoreAltezza = 50;

    Serial.println("% successo");
    Serial.println();
    for(int y = altezza; y > 0; y--) {

        if((y * 100 / altezza) % divisoreAltezza == 0 || y == altezza)
        stampaLarghezzaFissa(y * 100 / altezza, 3);
        else Serial.print("   ");
        Serial.print(" | ");

        for(int x = 0, test = 0; x < larghezza; x++) {
            // Se ci troviamo a un'ascissa per cui ci sono dati
            if(x == larghezza * riassunto[test][(int)elemRiass::mpmEffettivi] / mpmMax) {
                test++;
                // Se siamo nel punto in cui i dati sono maggiori per quella ascissa
                if(y == (altezza * (riassunto[test][(int)elemRiass::successo] / 100))/100) {
                    // Stampa un punto
                    Serial.print("*");
                    continue;
                }
            }
            // se non è stata stampato il punto
            Serial.print(" ");
        }
        Serial.println();
    }

    Serial.print("  0 +");
    for(int x = 0; x < larghezza; x++) Serial.print("-");
    Serial.println("  mess/min");

    Serial.print("    0");

    uint8_t distanzaPunti = larghezza/(mpmMax/50);
    for(int x = 1; x < larghezza - distanzaPunti; x++)
    if(x % distanzaPunti == 0)
    Serial.print(((x * mpmMax / larghezza)) - ((x * mpmMax / larghezza) % 50));
    else
    Serial.print(" ");

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

    // Per al massimo un minuto cerca di spegnere l'altra radio
    uint8_t mess[3] = {0,0,0x7B};
    radio.impostaTimeoutAck(1000);
    radio.inviaFinoAck(60,mess,3);

    radio.sleep();

    bool statoLed;
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
