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
    uint8_t ret;
    sc18_inviaDati(codiceCS, addr, 1, nullptr);
    sc18_richiediDati(1, &ret);
    return ret;
}



// Scrive su un registro della radio
//
void RFM69::SC18IS602B::scriviRegistro(uint8_t addr, uint8_t val) {
    sc18_inviaDati(codiceCS, addr | 0x80, 1, &val);
}


// Legge una sequenza di bytes adiacenti
//
void RFM69::SC18IS602B::leggiSequenza(uint8_t addr0, uint8_t len, uint8_t* data) {
    sc18_inviaDati(codiceCS, addr0, len, nullptr);
    sc18_richiediDati(len, data);
}



void RFM69::SC18IS602B::sc18_inviaDati(uint8_t byte1, uint8_t byte2,
                            uint8_t nrAltriByte, uint8_t* altriByte) {
    
    Wire.beginTransmission(indirizzo); 
    Wire.write(byte1);
    Wire.write(byte2);
    if(altriByte == nullptr) { // scrivi zeri "che la radio sostituirà con i suoi dati"
        for(uint8_t i = 0; i < nrAltriByte; i++) {
            Wire.write(0);
        } 
    }
    else { // data != nullptr, scrivi i dati da inviare
        for(uint8_t i = 0; i < nrAltriByte; i++) {
            Wire.write(*(altriByte + i));
        }
    }
    Wire.endTransmission();
}


void RFM69::SC18IS602B::sc18_richiediDati(uint8_t dataLen, uint8_t * data) {

    Wire.requestFrom(indirizzo, dataLen + 1); // +1: vedi commento sotto

    // uint8_t byte1; // il primo byte non serve perché ogni comunicazione SPI
    //                // inizia con l'indirizzo di un registro (e la radio
    //                // non risponde durante la trasmissione dell'indirizzo)
    // byte1 = Wire.read();
    Wire.read(); // scarta il primo byte (vedi commento sopra)
    
    for(unsigned int i = 0; i < dataLen; i++) {
        *(data + i) = Wire.read();
    }

}

