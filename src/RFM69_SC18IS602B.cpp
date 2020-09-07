/*! @file
@brief Implementazione della comunicazione I2C con la radio %RFM69 tramite il
chip SC18IS602B

Questo file contiene l'implementazione della classe che gestisce la
comunicazione tra la radio %RFM69 e il microcontrollore mediata dal convertitore
da I2C a SPI SC18IS602B. 

*/

#include "RFM69.h"
#include "Wire.h" // dal framework di Arduino



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

