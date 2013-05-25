#ifndef _quote_h
#define _quote_h

//wir brauchen eine Menge Flags. Dazu bedienen wir uns eines uint und definieren
//die einzelnen Bits und deren Positionen wie folgt:
#define FL_ESC     0 //1. bit: flag ob in escape-sequenz
#define FL_DQOT    1 //2. bit: flag in mitten im quote
#define FL_SQOT    2 //3. bit: flag in mitten im quote
#define FL_QOTCHGD 3 //4. bit: flag ob am aktuellen Zeichen entweder FL_DQOT
                     //        oder FL_SQOT verändert wurde.
#define FL_DLM     4 //5. bit: flag ob das aktuelle Zeichen ein delimiter ist.
#define FL_DLMFLD  5 //6. bit: flag ob delimiter-folding stattfinden soll oder
                     //        nicht wenn nicht, dann werden keine leeren wörter
                     //        "" ausgegeben, wenn zwei delimiter direkt aufein-
                     //        anderfolgen, weil diese "zusammengefaltet" oder
                     //        "zusammengefasst" werden.
#define FL_WRDFIN  6 //7. bit: flag ob ein neues wort begonnen werden
                     //        kann (vgl FL_DLMcat)
#define FL_MAXWRD  7 //8. bit: flag ob die Schleife wegen MaxWordCount vorzeitig
                     //        verlassen wurde


int splitQuotedStr (
                     char *str,
                     char *delims,
                     uint flgs,
                     int maxWrdCnt,
                     char *(**words)
                  );

#endif
