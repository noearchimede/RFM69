
// L'elenco di `#include` seguente permettere di compilare qualsiasi dei programmi
// di test o di esempio presenti nella cartella RFM69/Esempi.
// Chiaramente è possibile selezionare un solo programma alla volta.

// Numero del programma da compilare
#define PROGRAMMA 2


#if PROGRAMMA == 1
#include "../Esempi/Esempio_base.cpp"
#endif

#if PROGRAMMA == 2
#include "../Esempi/Test_collisioni_master.cpp"
#endif
#if PROGRAMMA == 3
#include "../Esempi/Test_collisioni_assistente.cpp"
#endif
