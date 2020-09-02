/*! @file
@brief Implementazione della comunicazione I2C con la radio %RFM69 tramite il
chip SC18IS602B

Questo file contiene l'implementazione della classe che gestisce la
comunicazione tra la radio %RFM69 e il microcontrollore mediata dal convertitore
da I2C a SPI SC18IS602B. 

*/

#include "RFM69.h"
#include "Wire.h" // dal framework di Arduino


// ### Lista di "function IDs" per SC18IS602B ###

#define FID_SPI_RW_0    0x0 // leggi/scrivi sul dispositivo SPI connesso a "SS0"
#define FID_SPI_RW_1    0x1
#define FID_SPI_RW_2    0x2
#define FID_SPI_RW_3    0x3
// tutte le combinazioni (FID_SPI_RW_X & FID_SPI_RW_Y) sono anche possibili
#define FID_SPI_CONF    0xf // configura l'interfaccia SPI (parametri vedi sotto)
#define FID_CLEAR_INT   0xf1 // rimuovi l'interrupt flag
#define FID_IDLE        0xf2 // entra in modalità riposo (esce automaticamente
//  se chiamato sul bus i2c)

// nota: oltre a queste Function ID ci sono quelle per l'utilizzo dei quattro
// pin SS (Slave Select) come GPIO (General Purpose Input/Output), che non sono
// però implementate in questa classe 


// ### Lista dei parameteri di configurazione SPI (cfr. p. 7 del datasheet) ###

// bitshift
#define SHIFT_ORDER         5
#define SHIFT_MODE          2
#define SHIFT_FREQUENCY     0
// valori 
#define ORDER_MSB           0 << SHIFT_ORDER
#define ORDER_LSB           1 << SHIFT_ORDER
#define MODE_CPOL0CPHA0     0 << SHIFT_MODE
#define MODE_CPOL0CPHA1     1 << SHIFT_MODE
#define MODE_CPOL1CPHA0     2 << SHIFT_MODE
#define MODE_CPOL1CPHA1     3 << SHIFT_MODE
#define FREQUENCY_1843KHZ   0 << SHIFT_FREQUENCY
#define FREQUENCY_461KHZ    1 << SHIFT_FREQUENCY
#define FREQUENCY_115KHZ    2 << SHIFT_FREQUENCY
#define FREQUENCY_58KHZ     3 << SHIFT_FREQUENCY



// Constructor
RFM69::SC18IS602B::SC18IS602B(uint8_t indirizzo, uint8_t numeroSS)
:
indirizzo(indirizzo),
codiceCS(numeroSS)
{

}



// Inizializzazione 
// Chiamato una sola volta, all'interno di `init()`
//
bool RFM69::SC18IS602B::inizializza() {
    
    Wire.begin();

    // Imposta l'interfaccia SPI 
    Wire.beginTransmission(indirizzo);
    Wire.write(FID_SPI_CONF);
    // ordine e modalità sono richiesti dalla radio, la frequenza è arbitraria
    Wire.write(ORDER_MSB & MODE_CPOL0CPHA0 & FREQUENCY_1843KHZ);
    Wire.endTransmission();

    usaInIsr(false);

    return true;

}




// Legge un registro della radio
//
uint8_t RFM69::SC18IS602B::leggiRegistro(uint8_t addr) {

    Wire.beginTransmission(indirizzo);
    Wire.write(codiceCS & 0xF); // maschera i 4 MSB, riservati ad altri comandi
    Wire.write(addr);
    Wire.write(0); // durante questa trasmissione la radio invia i dati richiesti
    Wire.endTransmission();
    Wire.requestFrom(indirizzo, (uint8_t)2); // cast per evitare un avviso del compilatore
    Wire.read();
    return Wire.read();

}



// Scrive su un registro della radio
//
void RFM69::SC18IS602B::scriviRegistro(uint8_t addr, uint8_t val) {

    Wire.beginTransmission(indirizzo);
    Wire.write(codiceCS & 0xF); // maschera i 4 MSB, riservati ad altri comandi
    Wire.write(addr | 0x80);
    Wire.write(val);
    Wire.endTransmission();

}


// Legge una sequenza di bytes adiacenti
//
void RFM69::SC18IS602B::leggiSequenza(uint8_t addr0, uint8_t len, uint8_t* data) {

    Wire.beginTransmission(indirizzo);
    Wire.write(codiceCS & 0xF);
    Wire.write(addr0); // * (vedi sotto)
    for(unsigned int i = 0; i < len; i++) {
        Wire.write(0); // la radio sta mandando dati
    }
    Wire.endTransmission();

    Wire.requestFrom(indirizzo, (uint8_t)(len + 1)); // +1 è il byte corrispondente a *
    Wire.read(); // scarta il byte corrispondente a *
    for(unsigned int i = 0; i < len; i++) {
        data[i] = Wire.read();
    }

}