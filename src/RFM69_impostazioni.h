/*! @file
@brief File di configurazione del modulo radio %RFM69

Questo file contiene una lunga serie di costanti `#define`d tramite le quali è
possibile modificare tutte le impostazioni della radio. Il file è `#include`d in
RFM69_inizializzazione.cpp, dove le sue costanti saranno usate per scrivere i
registri della radio.

Tutte le costanti seguenti possono essere modificate dall'utente.

Questo file deve restare nella posizione in cui si trova (cioè assieme al resto
dell'implementazione della classe RFM69).

*/

//! @cond 0

// Costanti di impostazione di tutti i registri della radio RFM69
//
// Ogni costante è presentata da due linee di commento: la prima è una descrizione
// dell'impostazione con l'indirizzo del registro della radio in cui è
// registrata, la seconda contiene un'indicazione sul tipo di valore da
// inserire e, nel caso di impostazioni "a scelta multipla", tutti i valori
// possibili.
//
// La notazione `_VALORE1,, _VALORE2, ecc` per un'impostazione `IMPOSTAZIONE`
// significa che il suo valore deve essere una delle costanti
// `IMPOSTAZIONE_VALORE1`, `IMPOSTAZIONE_VALORE2`, ecc.
//
// `ON OFF` Significa che il valore può essere la costante `ON` o `OFF`.
//
// `x` significa che il valore richiesto è un numero che sarà scritto direttamente
// nel registro corrispondente.
//
// `x [auto]` significa che il valore è un numero che sarà convertito automaticamente
// (da una macro del preprocessore) nel valore da inscrivere nel registro.

#ifdef RFM69_impostazioni_h
#warning "È già stato incluso un altro file di impostazione"
#else
#define RFM69_impostazioni_h


#include "RFM69_registri.h"
#include "RFM69_costanti_impostazione.h"




// [0x3, 0x4] Bit Rate (Chip Rate when Manchester encoding is enabled)
// x [auto] ; FSK: 1'200 - 300'000 , OOK: 1'200 - 32'768
#define BIT_RATE                        9600
// [0x5, 0x6] FSK frequency deviation
// x [auto] ; 600 - 300'000 ; constraints:
//  1) (1/5)*BIT_RATE < FREQ_DEV < 5*BIT_RATE
//  2) 600 < FREQ_DEV < 500'000 - (BIT_RATE / 2)
#define FREQ_DEV                        30000
// [Modulation index = (2 * FREQ_DEV) / BIT_RATE]

// [0x7, 0x8, 0x9] RF carrier frequency
// x [auto] ; 433Mhz module: 424 e 510 MHz
#define RADIO_FREQ                      434000000
// [0x11] Impostazione automatica della potenza (preprocessor)
// x [auto] = potenza Tx in dBm, max: 20
#define POTENZA_TX                      15

// [0x29] RSSI trigger level for Rssi interrupt
// x [auto] ; x in dBm ed è obbligatoriamente negativo
#define RSSI_TRESH                      -118
// [0x2] Modulation scheme
// _FSK, _OOK
#define MODULATION                      MODULATION_FSK
// [0x2] Data shaping
// _NONE, _FSK_GAUSS_1_0, _FSK_GAUSS_0_5, _FSK_GAUSS_0_3, _OOK_FCUTOFF_BR, _OOK_FCUTOFF_2BR
#define SHAPING                         SHAPING_FSK_GAUSS_1_0
// [0x37] Defines DC-free encoding/decoding performed
// _NONE, _MANCHESTER, _WHITENING
#define ENCODING                        ENCODING_WHITENING
// [0x37] Enables CRC calculation/check (Tx/Rx)
// ON OFF
#define CRC_EN                          ON
// [0x2C, 0x2D] Size of the preamble to be sent
// x [bytes], 0 - 65'536
#define PREAMBLE_SIZE                   4
// [0x2E] Enables the Sync word generation and detection
// ON OFF
#define SYNC_EN                         ON
// [0x2E] Number of tolerated bit errors in Sync word
// x, 0 - 7
#define SYNC_TOL                        0
// [0x2E] Size of the Sync word
// x [auto] ; bytes
#define SYNC_SIZE                       2
// [0x2F ... 0x36] Sync bytes (bytes 2 to 8 are used only sync size is big enough)
// byte1, byte2, ... , byte8
#define SYNC_VAL                        0x6A,0xE5,0xA7,0x56,0x6A,0xE5,0xA7,0x56

// [0x58] High or normal sensivity mode
// _HIGH, _NORMAL
#define SENSIVITY_MODE                  SENSIVITY_MODE_NORMAL
// [0x1E] (Only valid if AfcAutoOn is set) AFC register auto clearing management
// ON OFF
#define AFC_AUTOCLEAR                   OFF
// [0x1E] AFC is performed each time AfcStart is set (OFF) or each time Rx mode is entered (ON)?
// ON OFF
#define AFC_AUTO                        OFF
// [0x0B] Improved AFC routine for signals with modulation index lower than 2.
// [Modulation index = (2 * FREQ_DEV) / BIT_RATE]
// ON OFF
#define AFC_LOW_BETA_EN                 OFF
// [0x71] AFC offset set for low modulation index systems, used if AFC_LOW_BETA_EN == ON
// x [offset = x * 488Hz]
#define LOW_BETA_AFC_OFFSET             0
// [0x6F] Fading margin improvement, refer to 3.4.4 (datasheet)
// _NORMAL, _IMPROVED (_Improved dipende da AFC_LOW_BETA_EN)
#define CONTINUOUS_DAGC                 CONTINUOUS_DAGC_IMPROVED

// [0xD] Resolution of Listen mode Idle time
// LISTEN_RESOL: _64_US, _4_1_MS, _262_MS  -> [Idle time = LISTEN_COEF_IDLE * LISTEN_RESOL_IDLE]
#define LISTEN_RESOL_IDLE               LISTEN_RESOL_4_1_MS
// [0xD] Resolution of Listen mode Rx time
// LISTEN_RESOL: _64_US, _4_1_MS, _262_MS  -> [Idle time = LISTEN_COEF_RX * LISTEN_RESOL_RX]
#define LISTEN_RESOL_RX                 LISTEN_RESOL_64_US
// [0xD] Criteria for packet acceptance in Listen mode (Rssi Treshold always included)
// _NO_ADDR, _ADDR
#define LISTEN_CRIT                     LISTEN_CRIT_NO_ADDR
// [0xD] Action taken after acceptance of a packet in Listen mode
// _STOP, _GO_TO_MODE, _CONTINUE
#define LISTEN_END                      LISTEN_END_GO_TO_MODE
// [0xE] Duration of the Idle phase in Listen mode
// x ; [Idle time = LISTEN_COEF_IDLE * LISTEN_RESOL_IDLE]
#define LISTEN_COEF_IDLE                0xF5
// [0xF] Duration of the Idle phase in Listen mode
// x ; [Idle time = LISTEN_COEF_RX * LISTEN_RESOL_RX]
#define LISTEN_COEF_RX                  0x20


// [0x12] Rise/Fall time of ramp up/down in FSK
// [_3.4, _2, _1]_MS  [_500, _250, _125, _100, _62, _50, _40, _31, _25, _20, _15, _12, _10]_US
#define PA_RAMP                         PA_RAMP_40_US
// [0x13] Enables overload current protection (OCP) for the PA:
// ON OFF
#define OCP_EN                          ON
// [0x13] Trimming of OCP current
// x [auto] ; x = max current [mA], default: 95
#define OCP_TRIM                        95
// [0x18] LNA’s input impedance
// _50_OHMS, _200_OHMS
#define LNA_IMPEND                      LNA_IMPEND_50_OHMS
// [0x18] Current LNA gain, set either manually, or by the AGC
// _AGC, _G1, _G2, _G3, _G4, _G5, _G6
#define LNA_GAIN                        LNA_GAIN_AGC

// [0x19] Channel filter bandwidth
// x ; 2'600 - 500'000
// RX_BW > 2*FREQ_DEV + BIT_RATE + CryTol*RADIO_FREQ[MHz]*2, dove CryTol
//  (crystal tolerance) è compresa tra 50 e 100 ppm (cfr. datasheet p. 31)
#define RX_BW                           (FREQ_DEV * 2) + BIT_RATE + ((RADIO_FREQ/1000000) * 70 * 2)

// [0x19] Cut-off frequency of the DC offset canceller (DCC), in % of RxBw
// _16, _8, _4, _2, _1, _0_5, _0_25, _0_125
#define DCC_FREQ                        DCC_FREQ_0_125
// [0x1A] Channel filter bandwidth used during AFC
// x ; 2'600 - 500'000
#define RX_BW_AFC                       RX_BW
// [0x1A] Cut-off frequency of the DC offset canceller (DCC), in % of RxBw
// DCC_FREQ: _16, _8, _4, _2, _1, _0_5, _0_25, _0_125
#define DCC_FREQ_AFC                    DCC_FREQ

// [0x1B] Selects type of threshold in the OOK data slicer
// _FIXED, _PEAK, _AVERAGE
#define OOK_TRESH_TYPE                  OOK_TRESH_TYPE_PEAK
// [0x1B] Size of each decrement of the RSSI threshold in the OOK demodulator
// _0_5, _1_0, _1_5, _2_0, _3_0, _4_0, _5_0, _6_0
#define OOK_PEAK_TRESH_STEP             OOK_PEAK_TRESH_STEP_0_5
// [0x1B] Period of decrement of the RSSI threshold in the OOK demodulator (times per chip)
// _0_125, _0_25, _0_5, _1, _2, _4, _8, _16
#define OOK_PEAK_TRESH_DEC              OOK_PEAK_TRESH_DEC_1
// [0x1C] Filter coefficients in average mode of the OOK demodulator
// _32_PI, _8_PI, _4_PI, _2_PI (filter coefficient = chip rate / OOK_AVG_TRESH_FILT)
#define OOK_AVRG_TRESH_FILT             OOK_AVRG_TRESH_FILT_4_PI
// [0x1D] Fixed threshold value (in dB) in the OOK demodulator
// x [dB]
#define OOK_FIXED_TRESH                 0x6


// [0x37] Defines the behavior of the packet handler when CRC check fails
// ON OFF
#define PAYLOAD_READY_ON_CRC_FAIL       OFF
// [0x3D] After PayloadReady, delay between FIFO empty and the next RSSI phase
// x
#define INTER_PACKET_RX_DELAY           0

// [0x3D] Enables AES encryption/decryption
// ON OFF
#define AES_EN                          OFF
// [0x3E ... 0x04D] AES key (16 valori separati da virgole, usati solo se AES_EN == ON)
// byte1(MSB), byte2, byte3, ... , byte15, byte16
#define AES_KEY  0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf



#endif
//!@endcond
