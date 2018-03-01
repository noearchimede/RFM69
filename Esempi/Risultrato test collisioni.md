**1 marzo 2018**

Risultati del test *Collisioni*
===============================
<br>
Questo file contiene i risultati di due ripetizioni del test *collisioni messaggi*
eseguite il 1 marzo 2018.<br>
Il codice utilizzato si trova al commit `1ac8af15762fcd8c6f80e95ff1be19ce5895cca` del repository
git di questa libreria.
<br>




<br><br>
Impostazioni
------------

Il test è stato effettuato con le seguenti impostazioni:

File `RFM69_impostazioni.h`:

```
#define BIT_RATE                        19200
#define FREQ_DEV                        38400
#define RADIO_FREQ                      434000000
#define POTENZA_TX                      15
#define RSSI_TRESH                      -118
#define MODULATION                      MODULATION_FSK
#define SHAPING                         SHAPING_FSK_GAUSS_1_0
#define ENCODING                        ENCODING_WHITENING
#define CRC_EN                          ON
#define PREAMBLE_SIZE                   4
#define SYNC_EN                         ON
#define SYNC_TOL                        0
#define SYNC_SIZE                       2
#define SYNC_VAL                        0x6A,0xE5,0xA7,0x56,0x6A,0xE5,0xA7,0x56
#define SENSIVITY_MODE                  SENSIVITY_MODE_NORMAL
#define AFC_AUTOCLEAR                   OFF
#define AFC_AUTO                        OFF
#define AFC_LOW_BETA_EN                 OFF
#define LOW_BETA_AFC_OFFSET             0
#define CONTINUOUS_DAGC                 CONTINUOUS_DAGC_IMPROVED
#define LISTEN_RESOL_IDLE               LISTEN_RESOL_4_1_MS
#define LISTEN_RESOL_RX                 LISTEN_RESOL_64_US
#define LISTEN_CRIT                     LISTEN_CRIT_NO_ADDR
#define LISTEN_END                      LISTEN_END_GO_TO_MODE
#define LISTEN_COEF_IDLE                0xF5
#define LISTEN_COEF_RX                  0x20
#define PA_RAMP                         PA_RAMP_40_US
#define OCP_EN                          ON
#define OCP_TRIM                        95
#define LNA_IMPEND                      LNA_IMPEND_50_OHMS
#define LNA_GAIN                        LNA_GAIN_AGC
#define RX_BW                           BIT_RATE * 2
#define DCC_FREQ                        DCC_FREQ_0_125
#define RX_BW_AFC                       BIT_RATE * 2
#define DCC_FREQ_AFC                    DCC_FREQ_0_125
#define OOK_TRESH_TYPE                  OOK_TRESH_TYPE_PEAK
#define OOK_PEAK_TRESH_STEP             OOK_PEAK_TRESH_STEP_0_5
#define OOK_PEAK_TRESH_DEC              OOK_PEAK_TRESH_DEC_1
#define OOK_AVRG_TRESH_FILT             OOK_AVRG_TRESH_FILT_4_PI
#define OOK_FIXED_TRESH                 0x6
#define PAYLOAD_READY_ON_CRC_FAIL       OFF
#define INTER_PACKET_RX_DELAY           0
#define AES_EN                          OFF
#define AES_KEY  0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf
```

Altre impostazioni della classe:

```
Lunghezza messaggi    4    ->   inizializza(4);
Timeout ACK           100  ->   impostaTimeoutACK(100);
```

Impostazioni del test:

```
tolleranza              = 800
durataMinimaTest        = 30000
frequenzaTxIniziale     = 50
incrementoFrequenzaTx   = 25
nrTestMax               = 50
```


<br>br>
Risultato primo test
--------------------


```
Test completato in 27 minuti e 49 secondi.
Questa radio ha inviato 7524 messaggi di 4 bytes a 22 frequenze di
trasmissione diverse comprese tra 50 e 538 messaggi al minuto.

Attesa ACK: media = 19, massima = 104


   Risultati

   | #  | durata | mess inviati | mess/min | m/m esatti | successo |
   -----------------------------------------------------------------
   |  1 |   60 s |           52 |       50 |         51 |    92.30 |
   |  2 |  150 s |          197 |       75 |         78 |    75.63 |
   |  3 |  102 s |          174 |      100 |        101 |    85.63 |
   |  4 |  109 s |          218 |      125 |        119 |    84.40 |
   |  5 |   54 s |          121 |      150 |        132 |    93.38 |
   |  6 |  107 s |          305 |      175 |        170 |    83.93 |
   |  7 |  143 s |          498 |      200 |        207 |    78.51 |
   |  8 |  121 s |          475 |      225 |        234 |    79.15 |
   |  9 |  104 s |          419 |      250 |        240 |    70.40 |
   | 10 |   82 s |          390 |      275 |        284 |    72.30 |
   | 11 |   79 s |          402 |      300 |        303 |    71.14 |
   | 12 |  105 s |          625 |      325 |        356 |    68.96 |
   | 13 |   36 s |          212 |      350 |        344 |    61.79 |
   | 14 |   33 s |          191 |      375 |        347 |    64.39 |
   | 15 |   46 s |          325 |      400 |        420 |    65.23 |
   | 16 |   32 s |          251 |      425 |        461 |    55.37 |
   | 17 |   30 s |          234 |      450 |        464 |    45.29 |
   | 18 |   32 s |          297 |      475 |        549 |    42.76 |
   | 19 |   50 s |          419 |      500 |        500 |    28.40 |
   | 20 |  107 s |          926 |      525 |        514 |    18.25 |
   | 21 |   57 s |          522 |      550 |        544 |     9.96 |
   | 22 |   30 s |          271 |      575 |        538 |     4.42 |
   -----------------------------------------------------------------



   Percentuale di successo per frequenza di trasmissione

    %
100 |                                                                       
    |                                                                       
    |       *         *                                                     
    |             *  *     *                                                
 75 |          *                *  *                                        
    |                               *     * *                               
    |                                             **       *                
    |                                            *                          
 50 |                                                           *           
    |                                                            *          
    |                                                                      *
    |                                                                       
 25 |                                                                *      
    |                                                                       
    |                                                                  *    
    |                                                                      *
  0 +----------------------------------------------------------------------  mess/min
    0         100         200         300         400         500         600



Array dei dati raccolti da questo test, nell'ordine:
mpm previsti - mpm effettivi - messaggi tot - durata - successo

{{50,51,52,60,9230},{75,78,197,150,7563},{100,101,174,102,8563},{125,119,218,109,8440},{150,132,121,54,9338},{175,170,305,107,8393},{200,207,498,143,7851},{225,234,475,121,7915},{250,240,419,104,7040},{275,284,390,82,7230},{300,303,402,79,7114},{325,356,625,105,6896},{350,344,212,36,6179},{375,347,191,33,6439},{400,420,325,46,6523},{425,461,251,32,5537},{450,464,234,30,4529},{475,549,297,32,4276},{500,500,419,50,2840},{525,514,926,107,1825},{550,544,522,57,996},{575,538,271,30,442}}
```


<br><br>
Risultato secondo test
----------------------

```
Test completato in 27 minuti e 25 secondi.
Questa radio ha inviato 7812 messaggi di 4 bytes a 24 frequenze di
trasmissione diverse comprese tra 50 e 547 messaggi al minuto.

Attesa ACK: media = 18, massima = 100


   Risultati

   | #  | durata | mess inviati | mess/min | m/m esatti | successo |
   -----------------------------------------------------------------
   |  1 |  101 s |          107 |       50 |         63 |    83.17 |
   |  2 |   78 s |          104 |       75 |         79 |    88.46 |
   |  3 |  151 s |          246 |      100 |         97 |    78.86 |
   |  4 |  112 s |          254 |      125 |        135 |    81.49 |
   |  5 |   77 s |          204 |      150 |        157 |    88.23 |
   |  6 |   80 s |          246 |      175 |        182 |    87.39 |
   |  7 |  117 s |          398 |      200 |        203 |    82.91 |
   |  8 |   44 s |          150 |      225 |        201 |    64.66 |
   |  9 |  133 s |          555 |      250 |        250 |    76.03 |
   | 10 |   58 s |          281 |      275 |        286 |    61.92 |
   | 11 |   79 s |          387 |      300 |        293 |    66.92 |
   | 12 |   41 s |          213 |      325 |        309 |    36.15 |
   | 13 |   36 s |          214 |      350 |        352 |    63.08 |
   | 14 |   41 s |          252 |      375 |        366 |    32.93 |
   | 15 |   32 s |          210 |      400 |        389 |    49.04 |
   | 16 |   36 s |          278 |      425 |        459 |    40.64 |
   | 17 |   30 s |          225 |      450 |        444 |    46.22 |
   | 18 |   36 s |          296 |      475 |        481 |    32.77 |
   | 19 |  107 s |          862 |      500 |        480 |    23.31 |
   | 20 |  102 s |          907 |      525 |        530 |    17.75 |
   | 21 |   40 s |          368 |      550 |        548 |     6.79 |
   | 22 |   42 s |          385 |      575 |        545 |     7.27 |
   | 23 |   41 s |          385 |      600 |        551 |     7.27 |
   | 24 |   31 s |          285 |      625 |        547 |     5.26 |
   -----------------------------------------------------------------


   Percentuale di successo per frequenza di trasmissione

    %
100 |                                                                       
    |                                                                       
    |           *        *                                                  
    |         *              * *                                            
 75 |             *    *             *                                      
    |                                                                       
    |                          *           *      *                         
    |                                     *                                 
 50 |                                                                       
    |                                                  *      *             
    |                                                           *           
    |                                        *      *              *        
 25 |                                                                       
    |                                                              *        
    |                                                                    *  
    |                                                                      *
  0 +----------------------------------------------------------------------  mess/min
    0         100         200         300         400         500         600



Array dei dati raccolti da questo test, nell'ordine:
mpm previsti - mpm effettivi - messaggi tot - durata - successo

{{50,63,107,101,8317},{75,79,104,78,8846},{100,97,246,151,7886},{125,135,254,112,8149},{150,157,204,77,8823},{175,182,246,80,8739},{200,203,398,117,8291},{225,201,150,44,6466},{250,250,555,133,7603},{275,286,281,58,6192},{300,293,387,79,6692},{325,309,213,41,3615},{350,352,214,36,6308},{375,366,252,41,3293},{400,389,210,32,4904},{425,459,278,36,4064},{450,444,225,30,4622},{475,481,296,36,3277},{500,480,862,107,2331},{525,530,907,102,1775},{550,548,368,40,679},{575,545,385,42,727},{600,551,385,41,727},{625,547,285,31,526}}
```
