/*! @file

@brief Implementazione delle funzioni di inizializzazione della classe RFM69

In questo file sono raggruppate tutte le funzioni di inizializzazione
Il file è suddiviso in 5 sezioni:

1. Completamento delle impostazioni
2. Interpretazione dei dati del file di impostazione
3. Definizione dei membri static
4. Constructor e destructor
5. Inizializzazione
6. Debug
7. Funzione testConnessione()

*/


#include "RFM69.h"
#include "RFM69_registri.h"
#include "RFM69_impostazioni.h"

/// @cond 0

// ### 1. Completamento delle impostazioni ### //

// In questa sezione vengono definite le costanti che rappresentano le impostazioni
// della radio non modaificabili perché richieste dalla classe oppure rese inutili
// dalla stessa, e che perciò non fanno parte del file di impostazione.

// [0x2] Data processing mode
// _PACKET, _CONTINUOUS_SYNC, _CONTINUOUS_NO_SYNC
#define DATA_MODE                       DATA_MODE_PACKET
// [0x37] Defines the packet format used
// _FIXED, _VARIABLE
#define PACKET_FORMAT                   PACKET_FORMAT_VARIABLE
// [0x38] Packet->fixed: payload length; ->variable: max length in Rx, not used in Tx.
// x ; max per la radio: 255 se AES è OFF, 64 se AES è ON
// MAX PER LA LIBRERIA: 64
#define PAYLOAD_LENGHT                  64
// [0x2E] FIFO filling condition
// _SYNC_ADDR, _ALWAYS
#define FIFO_FILL_COND                  FIFO_FILL_COND_SYNC_ADDR
// [0x37] Defines address based filtering in Rx
// _NONE, _NO_BROAD, _BROADCAST
#define ADDRESS_FILTERING               ADDRESS_FILTERING_NONE
// [0x39] Node address used in address filtering
// x
#define NODE_ADDRESS                    0x0
// [0x3A] Broadcast address used in address filtering
// x
#define BROADCAST_ADDRESS               0x0
// [0x3B] Auto modes: intermediate mode
// _SLEEP, _STANDBY, _RX, _TX
#define AUTO_MODES_MODE                 AUTO_MODES_MODE_SLEEP
// [0x3B] Interrupt condition for entering the intermediate mode
// _NONE_ FIFO_NOT_EMPTY_RISING, _FIFO_LEVEL_RISING, _CRC_OK_RISING, _PAYLOAD_READY_RISING
//_SYNC_ADDRESS_RISING, _PACKET_SENT_RISING, _FIFO_NOT_EMPTY_FALLING
#define AUTO_MODES_ENTER                AUTO_MODES_ENTER_NONE
// [0x3B] Interrupt condition for exiting the intermediate mode (TMT -> timeout)
// _NONE, _FIFO_NOT_EMPTY_FALLING, _FIFO_LEVEL_RISING_OR_TMT, _CRC_OK_RISING_OR_TMT
// _PAYLOAD_READY_RISING_OR_TMT, _SYNC_ADDRESS_RISING_OR_TMT, _PACKET_SENT_RISING
// _FIFO_NOT_EMPTY_RISING
#define AUTO_MODES_EXIT                 AUTO_MODES_EXIT_NONE
// [0x3C] Defines the condition to start packet transmission
// _FIFO_LEVEL FIFO_NOT_EMPTY
#define TX_START_COND                   TX_START_COND_FIFO_NOT_EMPTY
// [0x3C] Used to trigger FifoLevel interrupt
// x == nuber of bytes in FIFO
#define FIFO_TRESH                      0xf
// [0x2A] Timeout interrupt is generated TIMEOUT_RX_START*16*Tbit after switching to Rx
// mode if Rssi interrupt doesn’t occur.         x, OFF: interrupt is disabled
#define TIMEOUT_RX_START                OFF
// [0x2B] Timeout interrupt is generated TIMEOUT_RSSI_TRESH*16*Tbit after Rssi interrupt
// if PayloadReady interrupt doesn’t occur.     x, OFF: interrupt is disabled
#define TIMEOUT_RSSI_TRESH              OFF
// [0x3D] Enables automatic RX restart after PayloadReady and FIFO empty
// ON OFF
#define AUTO_RX_RESTART_EN              OFF



// ### 2. Interpretazione dei dati del file di impostazione ### //

// (Le costanti create in quel file sono usate esclusivamente in questa sezione)



// # Preimpostazioni #

// array usate nella creazione di `registri`
// devono essere constexpr (non const) perché sono usate per inizializzare un'array
// salvata nella memoria del programma
static constexpr uint8_t sync_val [8] = {SYNC_VAL};
#if AES_EN == ON
static constexpr uint8_t aes_key [16] = {AES_KEY};
#endif




//Macro per l'accesso all'array valoreRegistri
#define VALORE_REGISTRI(i) pgm_read_byte_near(valoreRegistri + i)

// # Creazione dell'array di impostazione #

static const PROGMEM uint8_t valoreRegistri[80] = {
    0,                                                          // 00 | RFM69_00_FIFO
    0x4,                                                        // 01 | RFM69_01_OP_MODE
    (DATA_MODE << 5) | (MODULATION << 3) | (SHAPING),           // 02 | RFM69_02_DATA_MODUL
    (uint8_t)(BIT_RATE_VAL(BIT_RATE) >> 8),                     // 03 | RFM69_03_BITRATE_MSB
    (uint8_t)BIT_RATE_VAL(BIT_RATE),                            // 04 | RFM69_04_BITRATE_LSB
    (uint8_t)(FREQ_DEV_VAL(FREQ_DEV) >> 8),                     // 05 | RFM69_05_FDEV_MSB
    (uint8_t)FREQ_DEV_VAL(FREQ_DEV),                            // 06 | RFM69_06_FDEF_LSB
    (uint8_t)(RADIO_FREQ_VAL(RADIO_FREQ) >> 16),                // 07 | RFM69_07_FRF_MSB
    (uint8_t)(RADIO_FREQ_VAL(RADIO_FREQ) >> 8),                 // 08 | RFM69_08_FRF_MID
    (uint8_t)RADIO_FREQ_VAL(RADIO_FREQ),                        // 09 | RFM69_09_FRF_LSB
    0,                                                          // 10 | RFM69_0A_OSC_1
    (AFC_LOW_BETA_EN << 5),                                     // 11 | RFM69_0B_AFC_CTRL
    //[...]
    (LISTEN_RESOL_IDLE << 6) | (LISTEN_RESOL_RX << 4) | (LISTEN_CRIT << 3) | (LISTEN_END << 1),// 12 | RFM69_0D_LISTEN_1
    LISTEN_COEF_IDLE,                                           // 13 | RFM69_0E_LISTEN_2
    LISTEN_COEF_RX,                                             // 14 | RFM69_0F_LISTEN_3
    0,                                                          // 15 | RFM69_10_VERSION
    (uint8_t)POWER_VAL(POTENZA_TX),                             // 16 | RFM69_11_PA_LEVEL
    PA_RAMP,                                                    // 17 | RFM69_12_PA_RAMP
    (OCP_EN << 4) | (OCP_TRIM_VAL(OCP_TRIM)),                   // 18 | RFM69_13_OCP
        //[...]
    (LNA_IMPEND << 7) | (LNA_GAIN),                             // 19 | RFM69_18_LNA
    (DCC_FREQ << 5) | RX_BW_VAL(RX_BW, MODULATION),             // 20 | RFM69_19_RX_BW
    (DCC_FREQ_AFC << 5) | RX_BW_VAL(RX_BW_AFC, MODULATION),     // 21 | RFM69_1A_AFC_BW
    (OOK_TRESH_TYPE << 6) | (OOK_PEAK_TRESH_STEP << 3) | (OOK_PEAK_TRESH_DEC),// 22 | RFM69_1B_OOK_PEAK
    (OOK_AVRG_TRESH_FILT << 6),                                 // 23 | RFM69_1C_OOK_AVG
    OOK_FIXED_TRESH,                                            // 24 | RFM69_1D_OOK_FIX
    (AFC_AUTOCLEAR << 3) | (AFC_AUTO << 2),                     // 25 | RFM69_1E_AFC_FEI
    0,                                                          // 26 | RFM69_1F_AFC_MSB
    0,                                                          // 27 | RFM69_20_AFC_LSB
    0,                                                          // 28 | RFM69_21_FEI_MSB
    0,                                                          // 29 | RFM69_22_FEI_LSB
    0,                                                          // 30 | RFM69_23_RSSI_CONFIG
    0,                                                          // 31 | RFM69_24_RSSI_VALUE
    0,                                                          // 32 | RFM69_25_DIO_MAPPING_1
    0x7,                                                        // 33 | RFM69_26_DIO_MAPPING_2
    0,                                                          // 34 | RFM69_27_IRQ_FLAGS_1
    0,                                                          // 35 | RFM69_28_IRQ_FLAGS_2
    (uint8_t)RSSI_TRESH_VAL(RSSI_TRESH),                        // 36 | RFM69_29_RSSI_TRESH
    TIMEOUT_RX_START,                                           // 37 | RFM69_2A_RX_TIMEOUT_1
    TIMEOUT_RSSI_TRESH,                                         // 38 | RFM69_2B_RX_TIMEOUT_2
    (uint8_t)(PREAMBLE_SIZE >> 8),                              // 39 | RFM69_2C_PREAMBLE_MSB
    (uint8_t) PREAMBLE_SIZE,                                    // 40 | RFM69_2D_PREAMBLE_LSB
    (SYNC_EN << 7) | (FIFO_FILL_COND << 6) | (SYNC_SIZE_VAL(SYNC_SIZE) << 3) | SYNC_TOL,// 41 | RFM69_2E_SYNC_CONFIG
    sync_val[0],                                                // 42 | RFM69_2F_SYNC_VALUE_1
    sync_val[1],                                                // 43 | RFM69_30_SYNC_VALUE_2
    sync_val[2],                                                // 44 | RFM69_31_SYNC_VALUE_3
    sync_val[3],                                                // 45 | RFM69_32_SYNC_VALUE_4
    sync_val[4],                                                // 46 | RFM69_33_SYNC_VALUE_5
    sync_val[5],                                                // 47 | RFM69_34_SYNC_VALUE_6
    sync_val[6],                                                // 48 | RFM69_35_SYNC_VALUE_7
    sync_val[7],                                                // 49 | RFM69_36_SYNC_VALUE_8
    (PACKET_FORMAT << 7) | (ENCODING << 5) | (CRC_EN << 4) | (PAYLOAD_READY_ON_CRC_FAIL << 3) | (ADDRESS_FILTERING << 1),// 50 | RFM69_37_PACKET_CONFIG_1
    PAYLOAD_LENGHT,                                              // 51 | RFM69_38_PAYLOAD_LENGHT
    NODE_ADDRESS,                                                // 52 | RFM69_39_NODE_ADRS
    BROADCAST_ADDRESS,                                           // 53 | RFM69_3A_BROADCAST_ADRS
    (AUTO_MODES_ENTER << 5) | (AUTO_MODES_EXIT << 2) | AUTO_MODES_MODE,// 54 | RFM69_3B_AUTO_MODES
    (TX_START_COND << 7) | FIFO_TRESH,                           // 55 | RFM69_3C_FIFO_TRESH
    (INTER_PACKET_RX_DELAY << 4) | (AUTO_RX_RESTART_EN << 1) | AES_EN,// 56 | RFM69_3D_PACKET_CONFIG_2
    #if AES_EN == ON
    aes_key[0],                                                  // 57 | RFM69_3E_AES_KEY_1
    aes_key[1],                                                  // 58 | RFM69_3F_AES_KEY_2
    aes_key[2],                                                  // 59 | RFM69_40_AES_KEY_3
    aes_key[3],                                                  // 60 | RFM69_41_AES_KEY_4
    aes_key[4],                                                  // 61 | RFM69_42_AES_KEY_5
    aes_key[5],                                                  // 62 | RFM69_43_AES_KEY_6
    aes_key[6],                                                  // 63 | RFM69_44_AES_KEY_7
    aes_key[7],                                                  // 64 | RFM69_45_AES_KEY_8
    aes_key[8],                                                  // 65 | RFM69_46_AES_KEY_9
    aes_key[9],                                                  // 66 | RFM69_47_AES_KEY_10
    aes_key[10],                                                 // 67 | RFM69_48_AES_KEY_11
    aes_key[11],                                                 // 68 | RFM69_49_AES_KEY_12
    aes_key[12],                                                 // 69 | RFM69_4A_AES_KEY_13
    aes_key[13],                                                 // 70 | RFM69_4B_AES_KEY_14
    aes_key[14],                                                 // 71 | RFM69_4C_AES_KEY_15
    aes_key[15],                                                 // 72 | RFM69_4D_AES_KEY_16
    #else
    0,                                                           // 57 | RFM69_3E_AES_KEY_1
    0,                                                           // 58 | RFM69_3F_AES_KEY_2
    0,                                                           // 59 | RFM69_40_AES_KEY_3
    0,                                                           // 60 | RFM69_41_AES_KEY_4
    0,                                                           // 61 | RFM69_42_AES_KEY_5
    0,                                                           // 62 | RFM69_43_AES_KEY_6
    0,                                                           // 63 | RFM69_44_AES_KEY_7
    0,                                                           // 64 | RFM69_45_AES_KEY_8
    0,                                                           // 65 | RFM69_46_AES_KEY_9
    0,                                                           // 66 | RFM69_47_AES_KEY_10
    0,                                                           // 67 | RFM69_48_AES_KEY_11
    0,                                                           // 68 | RFM69_49_AES_KEY_12
    0,                                                           // 69 | RFM69_4A_AES_KEY_13
    0,                                                           // 70 | RFM69_4B_AES_KEY_14
    0,                                                           // 71 | RFM69_4C_AES_KEY_15
    0,                                                           // 72 | RFM69_4D_AES_KEY_16
    #endif
    0,                                                           // 73 | RFM69_4E_TEMP_1
    0,                                                           // 74 | RFM69_4F_TEMP_2
    //[...]
    SENSIVITY_MODE,                                              // 75 | RFM69_58_TEST_LNA
    //[...]
    0x55,                                                        // 76 | RFM69_5A_TEST_PA_1
    //[...]
    0x70,                                                        // 77 | RFM69_5C_TEST_PA_2
    //[...]
    CONTINUOUS_DAGC,                                             // 78 | RFM69_6F_TEST_DAGC
    //[...]
    LOW_BETA_AFC_OFFSET                                          // 79 | RFM69_71_TEST_AFC
};



// Esecuzione di una macro per la definizione dell'unica impostazione del file
// di impostazione della radio che non va nei registri ma serve alla classe
#define HIGH_POWER      IS_HIGH_POWER(POTENZA_TX)


// ### da qui in poi saranno usate solo le costanti etichettate come [software] nel
// file di impostazione ###


/// @endcond


// ### 3. Definizione dei membri static ### //



// Definizione dei membri `static`di questa classe
unsigned int RFM69::nrIstanze = 0;
RFM69* RFM69::pointerRadio = nullptr;


// ### 4. Constructor e destructor ### //



RFM69::RFM69(uint8_t pinSS, uint8_t pinInterrupt, uint8_t pinReset) :
pinReset(pinReset),
numeroInterrupt(digitalPinToInterrupt(pinInterrupt)),
haReset(pinReset == 0xff ? false : true),
highPower(HIGH_POWER)
{
    nrIstanze++;

    // Impostazioni SPI: la velocità è arbitraria, MSBfirst e cpol0cpha0 sono
    // richiesti dalla radio
    bus = new Spi(pinSS, 200000, Spi::BitOrder::MSBFirst, Spi::DataMode::cpol0cpha0);

}


RFM69::RFM69(uint8_t indirizzo, uint8_t numeroSS, uint8_t pinInterrupt, uint8_t pinReset) :
pinReset(pinReset),
numeroInterrupt(digitalPinToInterrupt(pinInterrupt)),
haReset(pinReset == 0xff ? false : true),
highPower(HIGH_POWER),
buffer()
{
    nrIstanze++;
    
    bus = new SC18IS602B(indirizzo, numeroSS);
}



// Destructor
RFM69::~RFM69() {
    // il buffer è in una classe wrapper che si occupa di liberare la memoria
    delete bus;
    nrIstanze--;
}


// ### 5. Inizializzazione ### //


// Inizializzazione della radio
// Cerca di inizializzare la radio e restituisce ErroreInit::ok (-> 0) se ci riesce
//
int RFM69::inizializza(uint8_t lunghezzaMaxMessaggio) {

    // Questa versione della classe ha un'unica ISR, dichiarata come static,
    // quindi in un programma ne può esistere una sola instanza.
    // Per aggiungerne altre bisogna aggiungere una o più altre funzioni isrCaller()
    // (static void isrCaller1(); static void isrCaller2(); ...  ) e collegare
    // ad ognuna di esse un'istanza differente (pointerRadio non sarà più uno solo
    // ma uno per ogni isrCaller, e tutti diversi fra loro)
    if(nrIstanze > 1) return Errore::initTroppeRadio;


    // ## SPI ## //

    // Inizializzazione di SPI
    if(!bus->inizializza()) return Errore::initInitSPIFallita;


    // ## RESET ## //

    // Il pin RESET della radio può facoltativamente essere collegato al microcontrollore
    if(haReset) {
        pinMode(pinReset, OUTPUT);
        digitalWrite(pinReset, HIGH);
        delay(10); //almeno 100 us (microsecondi)
        digitalWrite(pinReset, LOW);
        delay(50);//almeno 5 ms (millisecondi)
    }

    // ## ATTESA ATTIVAZIONE ## //

    // RFM69HCW richiede un'attesa di almeno 10 ms tra l'accensione e il primo
    // messaggio. Questo delay li garantisce.
    delay(20);


    // ## CONTROLLO DELLA CONNESSIONE ## //

    // Il registo 0x10 della radio contiene la versione del chip. Per la radio
    // per cui ho scritto questo programma (RFM69HCW ISM TRANSCEIVER MODULE v1.1)
    // il valore è 0x24. Se il valore è differente non è garantito che questo
    // programma funzioni.
    uint8_t versioneChip = bus->leggiRegistro(RFM69_10_VERSION);

    // Se la versione risulata un byte uniforme, probabilmente nessun dispositivo
    // ha risposto sul bus SPI
    if(versioneChip == 0 || versioneChip == 0xff) return Errore::initNessunaRadioConnessa;

    // Se la versione non è quella attesa segnalalo e blocca.
    else if (versioneChip != 0x24) return Errore::initVersioneRadioNon0x24;
    // Se poi a un'analisi del programma il nuovo dispositivo risulterà compatibile
    // con quello per cui ho scritto questo programma (versione == 0x24) basterà
    // aggiungere qui sotto il nuovo numero come valore valido.


    // ## INTERRUPT ## //

    // se il pin scelto come interrupt non ha questa capacità blocca l'inizializzazione
    if(numeroInterrupt == NOT_AN_INTERRUPT) return Errore::initPinInterruptNonValido;

    // Ora il pin è per forza un interrupt; collegalo alla funzione ISR di questa classe
    attachInterrupt(numeroInterrupt, isrCaller, RISING);

    // pointer a disposizione dell'ISR per riferire a questa classe
    pointerRadio = this;



    // ## IMPOSTAZIONE DELLA RADIO ## //

    if(!caricaImpostazioni()) return Errore::initErroreImpostazione;



    // ## INIZIALIZZAZIONE DI VARIABILI ## //

    buffer.init(lunghezzaMaxMessaggio);
    lungMaxMessEntrata = lunghezzaMaxMessaggio;


    messaggiInviati = 0;
    messaggiRicevuti = 0;

    durataUltimaAttesaAck = 0;
    durataMassimaAttesaAck = 0;
    sommaAtteseAck = 0;
    nrAckRicevuti = 0;

    defaultStandby();
    standby();

    return Errore::ok;

}


// Inizializza la radio e stampa il risultato dell'inizializzazione
//
int RFM69::inizializza(uint8_t lunghezzaMaxMessaggio, HardwareSerial& serial) {
    serial.print(F("Inizializzazione RFM69... "));
    int errore = inizializza(lunghezzaMaxMessaggio);
    if(!errore) serial.println(F("ok"));
    else stampaErroreSerial(serial, errore, false);
    return errore;
}


// Scrittura in tutti i registri della radio dei valori definiti nel file di
// impostazione
//
bool RFM69::caricaImpostazioni() {

    // Se un pin di reset è disponibile esegui un reset della radio
    if(haReset) {
        digitalWrite(pinReset, HIGH);
        delay(10); //almeno 100 us (microsecondi)
        digitalWrite(pinReset, LOW);
        delay(50);//almeno 5 ms (millisecondi)
    }

    // Scrivi tutti i valori dell'array valoreRegistri nei registri corrispondenti
    int indice = 0;
    int registro = 0;
    while(registro < RFM69_ULTIMO_REGISTRO) {
        if(!RFM69_RESERVED(registro)) {
            bus->scriviRegistro(registro, VALORE_REGISTRI(indice));
            indice++;
        }
        registro ++;
    }


    /*/ DEBUG
    // Stampa valore effettivo e valore atteso (cioé scritto nell'array
    // valoreRegistri() all'indice corrispondente) per ogni registro
    int i = 0, r = 0;
    while(r <= RFM69_ULTIMO_REGISTRO) {
        if(!RFM69_RESERVED(r)) {
            uint8_t reg = bus->leggiRegistro(r);
            uint8_t val = VALORE_REGISTRI(i);
            Serial.print("Reg 0x"); Serial.print(r, HEX);
            Serial.print(": 0x"); Serial.print(reg, HEX);
            Serial.print("\t");
            Serial.print("Atteso (ind "); Serial.print(i);
            Serial.print("): 0x"); Serial.print(val, HEX);
            Serial.print("\t");
            Serial.print(val == reg ? "" : "#");
            Serial.print("\n");
            i++;
        }
        r++;
    }
    // fine debug */


    // Controlla un solo registro. Non è una prova certa della corretta scrittura
    // di tutti i registri, ma poiché alcuni contengono anche byte il cui valore
    // non è impostabile (unused, r, w, r/wc) una lettura di tutti i registri scritti
    // richiederebbe molte eccezioni (es. leggi il registro 4 ignorando il bit 3)
    //
    // Il registro PaLevel non può avere valore 0x0 (non avrebbe senso) e tutti
    // i suoi bits sono utilizzati e impostati da questo programma
    // l'indice è 16 e non 17 perché c'è di mezzo un registro RESERVED
    if(bus->leggiRegistro(RFM69_11_PA_LEVEL) != VALORE_REGISTRI(16)) return false;

    return true;
}





// ### 6. Impostazioni modificabili in runtime ### //





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

    bus->scriviRegistro(RFM69_11_PA_LEVEL, paLevel);

    return true;
}



// Imposta la bit rate della comunicazione radio (bit al secondo)
//
int RFM69::impostaBitRate(uint32_t bitRate) {

    //Controlla che non si eccedano i limiti PER FSK (!!per ook il limite
    // superiore è più basso, 32'768!!)
    if((1200 > bitRate) || (300000 < bitRate)) return Errore::errore;

    // La bitrate si imposta nei registri 0x03 (MSB) e 0x04 (LSB) secondo la
    // formula [valoreRegistri = Fxosc/BitRate], con [Fxosc = 32'000'000]

    // Lo 0.5 aggiunto alla fine serve per migliorare l'approssimazione.
    uint16_t val = (32000000.00 / (float)bitRate) + 0.5;

    // Scrivi i registri
    bus->scriviRegistro(RFM69_03_BITRATE_MSB, val >> 8);
    bus->scriviRegistro(RFM69_04_BITRATE_LSB, val);

    if(bitRate == (((uint16_t)bus->leggiRegistro(RFM69_03_BITRATE_MSB) << 8) | bus->leggiRegistro(RFM69_04_BITRATE_LSB)))
    return Errore::ok;

    return Errore::errore;
}



// Imposta la frequency deviation per la modulazione FSK
//
int RFM69::impostaFreqDev(uint32_t freqDev) {

    // La Frequency Deviation per la modulazione FSK si imposta nei registri
    // 0x05 (MSB) e 0x06 (LSB) secondo la formula
    // [valoreRegistri = Fdev/Fstep], con [Fstep = Fxosc/2^19 = 32'000'000/2^19]

    // Lo 0.5 aggiunto alla fine serve per migliorare l'approssimazione.
    uint16_t val = (freqDev / (32000000.0 / 524288.0) ) + 0.5;

    // Scrivi i registri e assicurati che sianon stati scritti correttamente
    bus->scriviRegistro(RFM69_05_FDEV_MSB, val << 8);
    bus->scriviRegistro(RFM69_06_FDEF_LSB, val);

    if(freqDev == (((uint16_t)bus->leggiRegistro(RFM69_05_FDEV_MSB) << 8) | bus->leggiRegistro(RFM69_06_FDEF_LSB)))
    return Errore::ok;

    return Errore::errore;
}



// Imposta la frequenza del segnale radio
//
int RFM69::impostaFrequenzaMHz(uint32_t freq) {

    // Divide il valore come indicato alla p. 17 del datasheet del modulo RFM69HCW:
    // [Valore nei registri = FreqRadioHz / Fstep] dove [Fstep = Fxosc / 2^19]
    // con  [Fxosc = 32MHz].
    freq /= 32000000 / 524288;

    // Scrive il valore ottenuto nei registri
    //Registro RegFrfMsb
    bus->scriviRegistro(RFM69_07_FRF_MSB, freq >> 16);
    //Registro RegFrfMid
    bus->scriviRegistro(RFM69_08_FRF_MID, freq >> 8);
    //Registro RegFrfLsb
    bus->scriviRegistro(RFM69_09_FRF_LSB, freq);

    if(freq == (((uint32_t)bus->leggiRegistro(RFM69_07_FRF_MSB) << 16) | (bus->leggiRegistro(RFM69_08_FRF_MID) << 8) | (bus->leggiRegistro(RFM69_09_FRF_LSB))))
    return Errore::ok;

    return Errore::errore;
}







// ### 7. Debug ### //



// Stampa il valore di tutti i registri della radio sul monitor seriale
void RFM69::stampaRegistriSerial(HardwareSerial& serial) {

    bool lineaVuota = false;

    // l'ultimo registro è 0x71
    for(int i = 0; i < 0x72; i++) {

        // Non leggere i registri riservati ma stampa al posto di ogni blocco di
        // registri riservati una linea vuota
        // La macro RFM69_RESERVED è definita in "Registri_RFM69.h"
        if(RFM69_RESERVED(i)) {
            if(!lineaVuota) serial.print("[...]\n");
            lineaVuota = true;
            continue;
        }
        else lineaVuota = false;

        //leggi il registro e stampane indirizzo e valore
        uint8_t val = bus->leggiRegistro(i);
        serial.print("0x"); serial.print(i,HEX);
        serial.print("\t\t");
        serial.print("0x"); serial.print(val, HEX);
        serial.print("\n");
    }
}



uint8_t RFM69::valoreRegistro(uint8_t indirizzo) {
    return bus->leggiRegistro(indirizzo);
}



// ### 8. testConnessione ** //

uint8_t RFM69::testConnessione() {
    bus->inizializza();
    return bus->leggiRegistro(0x10);
}