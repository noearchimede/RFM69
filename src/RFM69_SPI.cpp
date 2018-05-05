/*! @file
@brief Implementazione della comunicazione SPI con la radio %RFM69

Il file contiene l'implementazione della classe che gestisce la comunicazione
tra la radio %RFM69 e il microcontrollore. Questa classe è un membro privato
della classe RFM69.

*/

#include "RFM69.h"


// Constructor
RFM69::Spi::Spi(uint8_t pinSlaveSelect, uint32_t frequenzaHz, BitOrder bitOrder, DataMode dataMode)
:
ss(pinSlaveSelect),
frequenza(frequenzaHz),
bitOrder(bitOrder),
dataMode(dataMode)
{}

// Inizializzazione di SPI
// Chiamato una sola volta, all'interno di `init()`
//
bool RFM69::Spi::inizializza() {

    if(!SPI_HAS_TRANSACTION) return false;

    // Inizializza la classe SPI di Arduino
    SPI.begin();

    // A questo punto l'interfaccia SPI è quasi pronta, manca solo l'impostazione
    // del pin SS (Slave Select) collegato alla radio.
    pinMode(ss, OUTPUT);
    digitalWrite(ss, HIGH);

    // L'interfaccia SPI è pronta, e non ci si dovrà più occupare della sua impotazione
    // se non si usa un altro dispositivo con requisiti diversi dalla radio.
    // Ma...

    // ...ATTENZIONE!
    // L'interfaccia SPI di ATMega328p può funzionare sia come master sia come
    // slave. Per gestire questa seconda funzione deve possedere un pin SS (per
    // ATMega328p è il pin che Arduino chiama 10). Se SS viene messo a terra
    // l'interfaccia passerà automaticamente ed immediatamente in modo slave.
    // Quindi se il pin SS è usato come input da qualche altra parte del programma
    // (il che non costituisce un problema in sé) bisognerà, prima di ogni utilizzo
    // di SPI, assicurarsi che la modalità selezionata sia ancora master, e rendere
    // il pin SS un OUTPUT durante la comunicazione per evitare l'interruzzione
    // inaspettata di quest'ultima.

    delay(20);

    usaInIsr(false);

    return true;
}




// Legge un registro della radio
//
uint8_t RFM69::Spi::leggiRegistro(uint8_t addr) {

    preparaTrasferimento();

    // Il byte di indirizzo deve avere uno 0 come MSB per dire alla radio check
    // si tratta di una lettura (-> che la radio deve inviare il contenuto delay
    // registro)
    trasferisciByte(addr & 0x7F);
    //il valore scritto (ad es. 0) è ignorato dalla radio
    uint8_t val = trasferisciByte();

    terminaTrasferimento();

    return val;
}



// Scrive su un registro della radio
//
void RFM69::Spi::scriviRegistro(uint8_t addr, uint8_t val) {

    preparaTrasferimento();

    // Se il MSB dell'indirizzo è 1 la radio si aspetta come secondo byte dei dati
    // da scrivere nel registro specificato
    trasferisciByte(addr | 0x80); //0x80: 1000 0000
    trasferisciByte(val);

    terminaTrasferimento();

}



// "Incapsulamento" della funzione `SPI.tranfer`, con l'aggiunta di un valore di
// default per il byte da inviare (utile se si è interessati solo alla ricezione)
uint8_t RFM69::Spi::trasferisciByte(uint8_t byte) {
    return SPI.transfer(byte);
}


// Prepara la comunicazione SPI.
// Il valore restituito deve essere passato come argomento alla funzione `terminaTransfer()`
//
void RFM69::Spi::preparaTrasferimento() {

    // Blocca gli interrupt
    if(gestisciInterrupt) cli();

    // # Impedisci al pin SS (10 su Arduino UNO) di mettere improvvisamente SPI in
    // modalità slave rendendolo temporeaneamente OUTPUT (se non lo è già)
    // Attenzione: si tratta del pin SS, #defined da qualche parte da Arduino,
    // non del pin ss, membro privato di questa classe!
    // stato del pin #

    // Trova l'indirizzo del bit corrispondente a SS in DDR (Data Direction Register)
    uint8_t bit = digitalPinToBitMask(SS); //SS, non ss
    volatile uint8_t *reg = portModeRegister(digitalPinToPort(SS)); //SS, non ss

    // Se il bit non è impostato, la porta è usata come INPUT e potrebbe causare
    // un cambio di modalità di SPI
    if(!(*reg & bit)) {
        pinSSInput = true;
        pinMode(SS, OUTPUT); //SS, non ss
    }
    else pinSSInput = false;


    // Inizia la transiszione
    SPI.beginTransaction(SPISettings(frequenza, bitOrder, dataMode));

    // Attiva il pin SS scelto per comunicare con la radio (potrebbe essere anche
    // quello trattato sopra)
    digitalWrite(ss, LOW);

}



// Termina la comunicazione SPI.
// L'argomento è il valore restituito da `preparaTransfer()`
//
void RFM69::Spi::terminaTrasferimento() {

    // Disattiva la comunicazione (SS high)
    digitalWrite(ss, HIGH);

    // Termina la transiszione
    SPI.endTransaction();

    // Reimposta la direzione di pinSS come era prima della comunicazione
    if(pinSSInput) pinMode(SS, INPUT);

    // Ripristina gli interrupt
    if(gestisciInterrupt) sei();
}
