/*! @file
@brief Costanti per l'impostazione della radio %RFM69

Questo file contiene le scelte possibili per tutte le impostazioni dela radio
RFM69 sotto forma di costanti o macro.
Le costanti qui definite sono usate nel file RFM69_impostazioni.h, le macro
nel file RFM69_inizializzazioe.h
*/

//! @cond 0

#ifndef RFM69_costanti_impostazione_h
#define RFM69_costanti_impostazione_h


// RFM69 hardware constants
// Fosc = 32'000'000
#define F_OSC 32000000
// Fstep = Fosc / 2^19
#define F_STEP 61.03515625


// General settings
#define OFF         0
#define ON          1




// RFM69_02_DATA_MODUL
#define DATA_MODE_PACKET                  0
#define DATA_MODE_CONTINUOUS_SYNC         2
#define DATA_MODE_CONTINUOUS_NO_SYNC      3

// il valore di MODULATION è usato anche da RX_BW_VAL
#define MODULATION_FSK                    0
#define MODULATION_OOK                    1

#define SHAPING_NONE                      0
#define SHAPING_FSK_GAUSS_1_0             1
#define SHAPING_FSK_GAUSS_0_5             2
#define SHAPING_FSK_GAUSS_0_3             3
#define SHAPING_OOK_FCUTOFF_BR            1
#define SHAPING_OOK_FCUTOFF_2BR           2

// RFM69_03_BITRATE_MSB, RFM69_04_BITRATE_LSB
#define BIT_RATE_VAL(x) ((unsigned int) (F_OSC / x))

// RFM69_05_FDEV_MSB, RFM69_06_FDEF_LSB
#define FREQ_DEV_VAL(x) ((unsigned int) (x / F_STEP))

// RFM69_07_FRF_MSB, RFM69_08_FRF_MID, RFM69_09_FRF_LSB
#define RADIO_FREQ_VAL(x) ((unsigned long) (x / F_STEP))


// RFM69_0D_LISTEN_1
#define LISTEN_RESOL_64_US                1
#define LISTEN_RESOL_4_1_MS               2
#define LISTEN_RESOL_262_MS               3

#define LISTEN_CRIT_NO_ADDR               0
#define LISTEN_CRIT_ADDR                  1

#define LISTEN_END_STOP                   0
#define LISTEN_END_GO_TO_MODE             1
#define LISTEN_END_CONTINUE               2

// RFM69_11_PA_LEVEL
#define POWER_VAL(x) ( \
    (x <= -2 ? (0x80 + (x + 18)) : \
    (x <= 13 ? (0x40 + (x + 18)) : \
    (x <= 17 ? (0x60 + (x + 14)) : \
    (x <= 20 ? (0x60 + (x + 11)) : \
    0 )))) \
)

// (classe) HIGH_POWER, la `x` deve essere la stessa passata a `POWER_VAL(x)`
#define IS_HIGH_POWER(x) (x <= 20 ? true : false)

// RFM69_12_PA_RAMP
#define PA_RAMP_3_4_MS		             0x0
#define PA_RAMP_2_MS		             0x1 
#define PA_RAMP_1_MS	                 0x2
#define PA_RAMP_500_US		             0x3
#define PA_RAMP_250_US		             0x4
#define PA_RAMP_125_US		             0x5
#define PA_RAMP_100_US		             0x6
#define PA_RAMP_62_US		             0x7
#define PA_RAMP_50_US		             0x8
#define PA_RAMP_40_US		             0x9
#define PA_RAMP_31_US		             0xA
#define PA_RAMP_25_US		             0xB
#define PA_RAMP_20_US		             0xC
#define PA_RAMP_15_US		             0xD
#define PA_RAMP_12_US		             0xE
#define PA_RAMP_10_US		             0xF

// RFM69_13_OCP
#define OCP_TRIM_VAL(x) ((x - 45) / 5)

// RFM69_18_LNA
#define LNA_IMPEND_50_OHMS              0
#define LNA_IMPEND_200_OHMS             1

#define LNA_GAIN_AGC                    0
#define LNA_GAIN_G1                     1
#define LNA_GAIN_G2                     2
#define LNA_GAIN_G3                     3
#define LNA_GAIN_G4                     4
#define LNA_GAIN_G5                     5
#define LNA_GAIN_G6                     6

// RFM69_19_RX_BW
#define DCC_FREQ_16                     0
#define DCC_FREQ_8                      1
#define DCC_FREQ_4                      2
#define DCC_FREQ_2                      3
#define DCC_FREQ_1                      4
#define DCC_FREQ_0_5                    5
#define DCC_FREQ_0_25                   6
#define DCC_FREQ_0_125                  7

// mod (modulation): 0 per FSK e 1 per OOK
#define RX_BW_VAL(x, mod) ( \
    (x >= (  1300 * (mod + 1)) ? ((2 << 3) || 7) : \
    (x >= (  1600 * (mod + 1)) ? ((1 << 3) || 7) : \
    (x >= (  2000 * (mod + 1)) ? ((0 << 3) || 7) : \
    (x >= (  2600 * (mod + 1)) ? ((2 << 3) || 6) : \
    (x >= (  3100 * (mod + 1)) ? ((1 << 3) || 6) : \
    (x >= (  3900 * (mod + 1)) ? ((0 << 3) || 6) : \
    (x >= (  5200 * (mod + 1)) ? ((2 << 3) || 5) : \
    (x >= (  6300 * (mod + 1)) ? ((1 << 3) || 5) : \
    (x >= (  7800 * (mod + 1)) ? ((0 << 3) || 5) : \
    (x >= ( 10400 * (mod + 1)) ? ((2 << 3) || 4) : \
    (x >= ( 12500 * (mod + 1)) ? ((1 << 3) || 4) : \
    (x >= ( 15600 * (mod + 1)) ? ((0 << 3) || 4) : \
    (x >= ( 20800 * (mod + 1)) ? ((2 << 3) || 3) : \
    (x >= ( 25000 * (mod + 1)) ? ((1 << 3) || 3) : \
    (x >= ( 31300 * (mod + 1)) ? ((0 << 3) || 3) : \
    (x >= ( 41700 * (mod + 1)) ? ((2 << 3) || 2) : \
    (x >= ( 50000 * (mod + 1)) ? ((1 << 3) || 2) : \
    (x >= ( 62500 * (mod + 1)) ? ((0 << 3) || 2) : \
    (x >= ( 83300 * (mod + 1)) ? ((2 << 3) || 1) : \
    (x >= (100000 * (mod + 1)) ? ((1 << 3) || 1) : \
    (x >= (125000 * (mod + 1)) ? ((0 << 3) || 1) : \
    (x >= (166700 * (mod + 1)) ? ((2 << 3) || 0) : \
    (x >= (200000 * (mod + 1)) ? ((1 << 3) || 0) : \
    (x >= (250000 * (mod + 1)) ? ((0 << 3) || 0) : \
    0 \
)))))))))))))))))))))))))

// RFM69_1B_OOK_PEAK
#define OOK_TRESH_TYPE_FIXED            0
#define OOK_TRESH_TYPE_PEAK             1
#define OOK_TRESH_TYPE_AVERAGE          2

#define OOK_PEAK_TRESH_STEP_0_5         0
#define OOK_PEAK_TRESH_STEP_1_0         1
#define OOK_PEAK_TRESH_STEP_1_5         2
#define OOK_PEAK_TRESH_STEP_2_0         3
#define OOK_PEAK_TRESH_STEP_3_0         4
#define OOK_PEAK_TRESH_STEP_4_0         5
#define OOK_PEAK_TRESH_STEP_5_0         6
#define OOK_PEAK_TRESH_STEP_6_0         7

#define OOK_PEAK_TRESH_DEC_1            0
#define OOK_PEAK_TRESH_DEC_0_5          1
#define OOK_PEAK_TRESH_DEC_0_25         2
#define OOK_PEAK_TRESH_DEC_0_125        3
#define OOK_PEAK_TRESH_DEC_2            4
#define OOK_PEAK_TRESH_DEC_4            5
#define OOK_PEAK_TRESH_DEC_8            6
#define OOK_PEAK_TRESH_DEC_16           7

// RFM69_1C_OOK_AVG
#define OOK_AVRG_TRESH_FILT_32_PI        0
#define OOK_AVRG_TRESH_FILT_8_PI         1
#define OOK_AVRG_TRESH_FILT_4_PI         2
#define OOK_AVRG_TRESH_FILT_2_PI         3

// RFM69_29_RSSI_TRESH
#define RSSI_TRESH_VAL(x) (-x * 2)

// RFM69_2E_SYNC_CONFIG
#define FIFO_FILL_COND_SYNC_ADDR         0
#define FIFO_FILL_COND_ALWAYS            1

#define SYNC_SIZE_VAL(x) (x - 1)

// RFM69_37_PACKET_CONFIG_1
#define PACKET_FORMAT_FIXED              0
#define PACKET_FORMAT_VARIABLE           1

#define ENCODING_NONE                    0
#define ENCODIMNG_MANCHESTER             1
#define ENCODING_WHITENING               2

#define ADDRESS_FILTERING_NONE           0
#define ADDRESS_FILTERING_NO_BROAD       1
#define ADDRESS_FILTERING_BROADCAST      2

// RFM69_3B_AUTO_MODES
#define AUTO_MODES_ENTER_NONE                           0
#define AUTO_MODES_ENTER_FIFO_NOT_EMPTY_RISING          1
#define AUTO_MODES_ENTER_FIFO_LEVEL_RISING              2
#define AUTO_MODES_ENTER_CRC_OK_RISING                  3
#define AUTO_MODES_ENTER_PAYLOAD_READY_RISING           4
#define AUTO_MODES_ENTER_SYNC_ADDRESS_RISING            5
#define AUTO_MODES_ENTER_PACKET_SENT_RISING             6
#define AUTO_MODES_ENTER_FIFO_NOT_EMPTY_FALLING         7

#define AUTO_MODES_EXIT_NONE                            0
#define AUTO_MODES_EXIT_FIFO_NOT_EMPTY_FALLING          1
#define AUTO_MODES_EXIT_FIFO_LEVEL_RISING_OR_TMT        2
#define AUTO_MODES_EXIT_CRC_OK_RISING_OR_TMT            3
#define AUTO_MODES_EXIT_PAYLOAD_READY_RISING_OR_TMT     4
#define AUTO_MODES_EXIT_SYNC_ADDRESS_RISING_OR_TMT      5
#define AUTO_MODES_EXIT_PACKET_SENT_RISING              6
#define AUTO_MODES_EXIT_FIFO_NOT_EMPTY_RISING           7

#define AUTO_MODES_MODE_SLEEP                           0
#define AUTO_MODES_MODE_STANDBY                         1
#define AUTO_MODES_MODE_RX                              2
#define AUTO_MODES_MODE_TX                              3

// RFM69_3C_FIFO_TRESH
#define TX_START_COND_FIFO_LEVEL        0
#define TX_START_COND_FIFO_NOT_EMPTY    1

// RFM69_58_TEST_LNA
#define SENSIVITY_MODE_NORMAL           0x1B
#define SENSIVITY_MODE_HIGH             0x2D

// RFM69_6F_TEST_DAGC
#define CONTINUOUS_DAGC_NORMAL          0x00
#define CONTINUOUS_DAGC_IMPROVED        (AFC_LOW_BETA_EN ? 0x20 : 0x30)


// # ALTRO #

// Utilizzo dell'ACK. L'ordine delle costanti rispetta quello dell'enum UsaAck
// nella classe RFM69 (cfr. file RFM69.h)
#define USA_ACK_MAI                     0
#define USA_ACK_SEMPRE                  1
#define USA_ACK_SU_RICHIESTA            2




#endif
//!@endcond


/*
### Tutte le impostazioni in ordine di apparizione nei registri ###

// [0x2] Modulation scheme
// _FSK, _OOK
#define MODULATION                      MODULATION_FSK
// [0x2] Data shaping
// _NONE, _FSK_GAUSS_1_0, _FSK_GAUSS_0_5, _FSK_GAUSS_0_3, _OOK_FCUTOFF_BR, _OOK_FCUTOFF_2BR
#define SHAPING                         SHAPING_FSK_GAUSS_1_0
// [0x2] Data processing mode
// _PACKET, _CONTINUOUS_SYNC, _CONTINUOUS_NO_SYNC
#define DATA_MODE                       DATA_MODE_PACKET
// [0x3, 0x4] Bit Rate (Chip Rate when Manchester encoding is enabled)
// x [auto] ; FSK: 1'200 - 300'000 , OOK: 1'200 - 32'768
#define BIT_RATE                        9600
// [0x5, 0x6] FSK frequency deviation
// x [auto] ; 600 - 300'000 ; constraints:
//  1) (1/5)*BIT_RATE < FREQ_DEV < 5*BIT_RATE
//  2) 600 < FREQ_DEV < 500'000 - (BIT_RATE / 2)
#define FREQ_DEV                        12000
// [0x7, 0x8, 0x9] RF carrier frequency
// x [auto] ; 433Mhz module: 424 e 510 MHz
#define RADIO_FREQ                      434000000
// [0x0B] Improved AFC routine for signals with modulation index lower than 2.
// [Modulation index = (2 * FREQ_DEV) / BIT_RATE]
// ON OFF
#define AFC_LOW_BETA_EN                 OFF
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
// [0x11] Impostazione automatica della potenza (preprocessor)
// x [auto] = potenza Tx in dBm, max: 20
#define POTENZA_TX                      15
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
// [0x19] Cut-off frequency of the DC offset canceller (DCC), in % of RxBw
// _16, _8, _4, _2, _1, _0_5, _0_25, _0_125
#define DCC_FREQ                        DCC_FREQ_0_125
// [0x19] Channel filter bandwidth
// x ; 2'600 - 500'000 ; ~[2 * bitRate] (trovato sperimentalmente)
#define RX_BW                           18000
// [0x1A] Cut-off frequency of the DC offset canceller (DCC), in % of RxBw
// DCC_FREQ: _16, _8, _4, _2, _1, _0_5, _0_25, _0_125
#define DCC_FREQ_AFC                    DCC_FREQ_0_125
// [0x1A] Channel filter bandwidth used during AFC
// x ; 2'600 - 500'000
#define RX_BW_AFC                       18000
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
// [0x1E] (Only valid if AfcAutoOn is set) AFC register auto clearing management
// ON OFF
#define AFC_AUTOCLEAR                   OFF
// [0x1E] AFC is performed each time AfcStart is set (OFF) or each time Rx mode is entered (ON)?
// ON OFF
#define AFC_AUTO                        OFF
// [0x29] RSSI trigger level for Rssi interrupt
// x [auto] ; x in dBm ed è obbligatoriamente negativo
#define RSSI_TRESH                      -118
// [0x2A] Timeout interrupt is generated TIMEOUT_RX_START*16*Tbit after switching to Rx
// mode if Rssi interrupt doesn’t occur.         x, OFF: interrupt is disabled
#define TIMEOUT_RX_START                OFF
// [0x2B] Timeout interrupt is generated TIMEOUT_RSSI_TRESH*16*Tbit after Rssi interrupt
// if PayloadReady interrupt doesn’t occur.     x, OFF: interrupt is disabled
#define TIMEOUT_RSSI_TRESH              OFF
// [0x2C, 0x2D] Size of the preamble to be sent
// x [bytes], 0 - 65'536
#define PREAMBLE_SIZE                   4
// [0x2E] FIFO filling condition
// _SYNC_ADDR, _ALWAYS
#define FIFO_FILL_COND                  FIFO_FILL_COND_SYNC_ADDR
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
#define SYNC_VAL                        0x88,0x2D,0xD4,0x1,0x1,0x1,0x1,0x1
// [0x37] Defines DC-free encoding/decoding performed
// _NONE, _MANCHESTER, _WHITENING
#define ENCODING                        ENCODING_WHITENING
// [0x37] Enables CRC calculation/check (Tx/Rx)
// ON OFF
#define CRC_EN                          ON
// [0x37] Defines the behavior of the packet handler when CRC check fails
// ON OFF
#define PAYLOAD_READY_ON_CRC_FAIL       OFF
// [0x37] Defines address based filtering in Rx
// _NONE, _NO_BROAD, _BROADCAST
#define ADDRESS_FILTERING               ADDRESS_FILTERING_NONE
// [0x37] Defines the packet format used
// _FIXED, _VARIABLE
#define PACKET_FORMAT                   PACKET_FORMAT_VARIABLE
// [0x38] Packet->fixed: payload length; ->variable: max length in Rx, not used in Tx.
// x ; max per la radio: 255 se AES è OFF, 64 se AES è ON
// MAX PER LA LIBRERIA: 64
#define PAYLOAD_LENGHT                  64
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
// [0x3D] After PayloadReady, delay between FIFO emptyx and the next RSSI phase
// x
#define INTER_PACKET_RX_DELAY           0
// [0x3D] Enables automatic RX restart after PayloadReady and FIFO empty
// ON OFF
#define AUTO_RX_RESTART_EN              OFF
// [0x3D] Enables AES encryption/decryption
// ON OFF
#define AES_EN                          OFF
// [0x3E ... 0x04D] AES key (16 valori separati da virgole, usati solo se AES_EN == ON)
// byte1(MSB), byte2, byte3, ... , byte15, byte16
#define AES_KEY  0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf
// [0x58] High or normal sensivity mode
// _HIGH, _NORMAL
#define SENSIVITY_MODE                  SENSIVITY_MODE_NORMAL
// [0x6F] Fading margin improvement, refer to 3.4.4 (datasheet)
// _NORMAL, _IMPROVED (_Improved dipende da AFC_LOW_BETA_EN)
#define CONTINUOUS_DAGC                 CONTINUOUS_DAGC_IMPROVED
// [0x71] AFC offset set for low modulation index systems, used if AFC_LOW_BETA_EN == ON
// x [offset = x * 488Hz]
#define LOW_BETA_AFC_OFFSET             0

*/
