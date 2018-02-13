/*! @file
@brief Registri della radio %RFM69

Questo file contiene l'elenco `#define`d dei registri della radio %RFM69.
*/


// ELENCO DEI REGISTRI

// I registri con un asterisco a fianco hanno un valore di reset differente
// da quello di default.

#define RFM69_00_FIFO   		   0x00
#define RFM69_01_OP_MODE		   0x01
#define RFM69_02_DATA_MODUL		   0x02
#define RFM69_03_BITRATE_MSB	   0x03
#define RFM69_04_BITRATE_LSB	   0x04
#define RFM69_05_FDEV_MSB		   0x05
#define RFM69_06_FDEF_LSB		   0x06
#define RFM69_07_FRF_MSB		   0x07
#define RFM69_08_FRF_MID		   0x08
#define RFM69_09_FRF_LSB		   0x09
#define RFM69_0A_OSC_1		       0x0A
#define RFM69_0B_AFC_CTRL		   0x0B
//[...]
#define RFM69_0D_LISTEN_1		   0x0D
#define RFM69_0E_LISTEN_2		   0x0E
#define RFM69_0F_LISTEN_3		   0x0F
#define RFM69_10_VERSION		   0x10
#define RFM69_11_PA_LEVEL		   0x11
#define RFM69_12_PA_RAMP		   0x12
#define RFM69_13_OCP			   0x13
//[...]
#define RFM69_18_LNA			   0x18 // *
#define RFM69_19_RX_BW		       0x19 // *
#define RFM69_1A_AFC_BW		       0x1A // *
#define RFM69_1B_OOK_PEAK          0x1B
#define RFM69_1C_OOK_AVG		   0x1C
#define RFM69_1D_OOK_FIX		   0x1D
#define RFM69_1E_AFC_FEI		   0x1E
#define RFM69_1F_AFC_MSB		   0x1F
#define RFM69_20_AFC_LSB		   0x20
#define RFM69_21_FEI_MSB		   0x21
#define RFM69_22_FEI_LSB		   0x22
#define RFM69_23_RSSI_CONFIG	   0x23
#define RFM69_24_RSSI_VALUE		   0x24
#define RFM69_25_DIO_MAPPING_1	   0x25
#define RFM69_26_DIO_MAPPING_2	   0x26 // *
#define RFM69_27_IRQ_FLAGS_1	   0x27
#define RFM69_28_IRQ_FLAGS_2	   0x28
#define RFM69_29_RSSI_TRESH		   0x29 // *
#define RFM69_2A_RX_TIMEOUT_1	   0x2A
#define RFM69_2B_RX_TIMEOUT_2	   0x2B
#define RFM69_2C_PREAMBLE_MSB	   0x2C
#define RFM69_2D_PREAMBLE_LSB	   0x2D
#define RFM69_2E_SYNC_CONFIG	   0x2E
#define RFM69_2F_SYNC_VALUE_1	   0x2F // *
#define RFM69_30_SYNC_VALUE_2	   0x30 // *
#define RFM69_31_SYNC_VALUE_3	   0x31 // *
#define RFM69_32_SYNC_VALUE_4	   0x32 // *
#define RFM69_33_SYNC_VALUE_5	   0x33 // *
#define RFM69_34_SYNC_VALUE_6	   0x34 // *
#define RFM69_35_SYNC_VALUE_7	   0x35 // *
#define RFM69_36_SYNC_VALUE_8	   0x36 // *
#define RFM69_37_PACKET_CONFIG_1   0x37
#define RFM69_38_PAYLOAD_LENGHT	   0x38
#define RFM69_39_NODE_ADRS		   0x39
#define RFM69_3A_BROADCAST_ADRS	   0x3A
#define RFM69_3B_AUTO_MODES		   0x3B
#define RFM69_3C_FIFO_TRESH		   0x3C // *
#define RFM69_3D_PACKET_CONFIG_2   0x3D
#define RFM69_3E_AES_KEY_1		   0x3E
#define RFM69_3F_AES_KEY_2		   0x3F
#define RFM69_40_AES_KEY_3		   0x40
#define RFM69_41_AES_KEY_4		   0x41
#define RFM69_42_AES_KEY_5		   0x42
#define RFM69_43_AES_KEY_6		   0x43
#define RFM69_44_AES_KEY_7		   0x44
#define RFM69_45_AES_KEY_8		   0x45
#define RFM69_46_AES_KEY_9		   0x46
#define RFM69_47_AES_KEY_10		   0x47
#define RFM69_48_AES_KEY_11		   0x48
#define RFM69_49_AES_KEY_12		   0x49
#define RFM69_4A_AES_KEY_13		   0x4A
#define RFM69_4B_AES_KEY_14		   0x4B
#define RFM69_4C_AES_KEY_15		   0x4C
#define RFM69_4D_AES_KEY_16		   0x4D
#define RFM69_4E_TEMP_1		       0x4E
#define RFM69_4F_TEMP_2		       0x4F
//[...]
#define RFM69_58_TEST_LNA		   0x58
//[...]
#define RFM69_5A_TEST_PA_1		   0x5A
//[...]
#define RFM69_5C_TEST_PA_2		   0x5C
//[...]
#define RFM69_6F_TEST_DAGC		   0x6F // *
//[...]
#define RFM69_71_TEST_AFC		   0x71


//vale `true` se x È l'indirizzo di un registro riservato o inesistente
#define RFM69_RESERVED(x) ( \
    x == 0x0C || \
    (0x13 < x && x < 0x18) || \
    (0x4F < x && x < 0x58) || \
    x == 0x59 || \
    x == 0x5B || \
    (0x5c < x && x < 0x6F) || \
    (0x6F < x && x < 0x71) || \
0x71 < x )

// Indirizzo dell'ultimo registro
#define RFM69_ULTIMO_REGISTRO      0x71



// ELENCO DELLE 'FLAGS'


// Registro RegIrqFlags1 (0x27)
#define RFM69_FLAGS_1_MODE_READY            0x80
#define RFM69_FLAGS_1_RX_READY              0x40
#define RFM69_FLAGS_1_TX_READY              0X20
#define RFM69_FLAGS_1_PLL_LOCK              0X10
#define RFM69_FLAGS_1_RSSI                  0X08
#define RFM69_FLAGS_1_TIMEOUT               0X04
#define RFM69_FLAGS_1_AUTO_MODE             0X02
#define RFM69_FLAGS_1_SYNC_ADDR_MATCH       0X01

// Registro RegIrqFlags2 (0x28)
#define RFM69_FLAGS_2_FIFO_FULL             0x80
#define RFM69_FLAGS_2_FIFO_NOT_EMPTY        0x40
#define RFM69_FLAGS_2_FIFO_LEVEL            0x20
#define RFM69_FLAGS_2_FIFO_OVERRUN          0x10
#define RFM69_FLAGS_2_PACKET_SENT           0x08
#define RFM69_FLAGS_2_PAYLOAD_READY         0x04
#define RFM69_FLAGS_2_CRC_OK                0x02
//bit 0: unused                             0x01
