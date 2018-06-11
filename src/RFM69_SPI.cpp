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
ss(pinSlaveSelect)
{
    // ### Calcola i valori per i registri SPCR e SPSR ### //
    // Questa porzione di codice è persa dall'impementazione della classe
    // SPISettings nella libreria SPI di Arduino.

    // Find the fastest clock that is less than or equal to the given clock
    // rate
    uint8_t clockDiv;
    uint32_t clockSetting = F_CPU / 2;
    clockDiv = 0;
    while (clockDiv < 6 && frequenzaHz < clockSetting) {
        clockSetting /= 2;
        clockDiv++;
    }
    // Compensate for the duplicate fosc/64
    if (clockDiv == 6)
    clockDiv = 7;
    // Invert the SPI2X bit
    clockDiv ^= 0x1;

    // Pack into the SPISettings class
    spcr = _BV(SPE) | _BV(MSTR) | ((bitOrder == BitOrder::LSBFirst) ? _BV(DORD) : 0) |
    (dataMode & 0x0C) | ((clockDiv >> 1) & 0x03);
    spsr = clockDiv & 0x01;
}



// Inizializzazione di SPI
// Chiamato una sola volta, all'interno di `init()`
//
bool RFM69::Spi::inizializza() {

    // ### Inizializza l'intefaccia SPI ###

    // L'interfaccia SPI di ATMega328p può funzionare sia come master sia come
    // slave. Per gestire questa seconda funzione deve possedere un pin SS (per
    // ATMega328p è il pin che Arduino chiama 10). Se SS viene messo a terra
    // l'interfaccia passerà automaticamente ed immediatamente in modo slave.
    // Quindi se si intende usare quel pin come input altrove nel programma SPI
    // deve essere disabilitato e riabilitato solo durante i trasferimenti.
    //
    // Questo programma permette all'utente di usare il pin SS come input.


    // Set direction register for SCK and MOSI pin.
    // MISO pin automatically overrides to INPUT.
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);

    // A questo punto l'interfaccia SPI è quasi pronta, manca solo l'impostazione
    // del pin SS (Slave Select) collegato alla radio.
    pinMode(ss, OUTPUT);
    digitalWrite(ss, HIGH);


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



// Esegue una transizione SPI,c ioè invia un byte e ne riceve uno contemporaneamente
//
uint8_t RFM69::Spi::trasferisciByte(uint8_t byte) {
    SPDR = byte;
    while (!(SPSR & _BV(SPIF))) ; // wait
    return SPDR;
}


// Prepara la comunicazione SPI.
//
void RFM69::Spi::preparaTrasferimento() {

    // Blocca gli interrupt
    if(gestisciInterrupt) cli();

    // Trova lo stato attuale del pin SS per reimpostarlo alla fine del
    // trasferimento
    uint8_t port = digitalPinToPort(SS);
    uint8_t bit = digitalPinToBitMask(SS);
    volatile uint8_t *reg = portModeRegister(port);
    if(!(*reg & bit)) pinSSInput = true;
    else pinSSInput = false;

    if(pinSSInput) pinMode(SS, OUTPUT);

    SPCR = spcr;
    SPSR = spsr;

    // Attiva il pin SS scelto per comunicare con la radio (potrebbe essere anche
    // quello trattato sopra)
    digitalWrite(ss, LOW);

}



// Termina la comunicazione SPI.
//
void RFM69::Spi::terminaTrasferimento() {

    // Disattiva la comunicazione (SS high)
    digitalWrite(ss, HIGH);

    // Disabilita SPI per evitare che entri in slave mode (SPI entra in Slave
    // Mode se il pin SS è un input e passa al livello logico 0)
    SPCR &= ~(_BV(SPE));

    // Reimposta la direzione di pinSS come era prima della comunicazione
    if(pinSSInput) pinMode(SS, INPUT);

    // Ripristina gli interrupt
    if(gestisciInterrupt) sei();
}
