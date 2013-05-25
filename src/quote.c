/* splitQuotedString
 *
 * Copyright (C) 2011 Oliver Meissner <o.meissner@comlab-computer.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "quote.h"

//hier nun die passenden Funktionen fuer den Zugriff auf die einzelnen Bits
#define SETBIT(a, b) (a |= (1 << b))
#define CLRBIT(a, b) (a &= ~(1 << b))
#define SWPBIT(a, b) (a ^= (1 << b))
#define GETBIT(a, b) ( (( a & (1 << b)) == (1 << b)) ? 1 : 0)

/*
 * splitQuotedStr
 * zerlegt den in str übergebenen String an allen Vorkommen von in delims
 * übergebenen Zeichen aber beachtet dabei strikt das Quoting. Zeichenfolgen
 * innerhalb von doppelten oder einfachen Anführungszeichen ["'] werden niemals
 * zerlegt. Die Escape-Sequenz (backslash) [\] kann -- wie gewohnt -- verwendet
 * werden: Ein Zeichen dem ein Backslash voransteht wird als Teil der Zeichen-
 * folge interpretiert und seine mögliche Bedeutung als Delimiter wird
 * ignoriert.
 * Es ist möglich Anführungszeichen zu verschachteln. (Doppelte innerhalb von
 * einfachen -- und umgekehrt. )
 * Beispiele: "foo'bar'" ergibt:   foo'bar'
 *            'foo"bar"' ergibt:   foo"bar"
 * Wird in maxWrdCnt (maxWordCount) ein Wert > 0 übergeben, wird die
 * Verarbeitung nach der angegebenen Anzahl von gefundenen Wörtern oder am Ende
 * von str beendet -- je nachdem, was zuerst eintritt.
 * Ergebnis-Wörter werden in words gespeichert.
 * für den String-Vector words wird innerhalb der Funktion Speicher allokiert,
 * welcher später manuell wieder freigegeben werden muss (für jedes Element von
 * words[n])
 *
 */

#ifdef DEBUG
void splitQuotedStr_DebugFlags(uint flgVal) {
  char *flags[] = {
                     "FL_ESC", "FL_DQOT",   "FL_SQOT",   "FL_QOTCHGD",
                     "FL_DLM", "FL_DLMFLD", "FL_WRDFIN", "FL_MAXWRD"
                   };
  int i = 0;
  fprintf(stderr, "     >>flags: 0x%x = ", (int)flgVal);
  for (i = sizeof(flags) / sizeof(char *); i > 0; i--) {
     if (GETBIT(flgVal, i) == 1) {
        fprintf(stderr, "(%d)%s ", i, flags[i]);
     } else {
        //printf("%s ", strlwr(*flags[i]));
     }
  }
  fprintf(stderr, "\n");
}
#endif


int splitQuotedStr (
                     char *str,
                     char *delims,
                     uint flgs,
                     int maxWrdCnt,
                     char *(**words)
                  ) {
  int wrdcnt = 0;
  uint flags = 0;

  char *Buffer = (char *)malloc(sizeof(char) * (strlen(str) + 1));
  char *bufPos = Buffer;
  char *ptr;
  char *p;

  //take the given flags... for now it's only FL_DLMFLD
  if (GETBIT(flgs, FL_DLMFLD))
      SETBIT(flags, FL_DLMFLD);

  if (!(*words))
     *words=(char **)malloc(sizeof(char*));

  for (ptr = str; *ptr && (GETBIT(flags, FL_MAXWRD) == 0); ptr++) {
    if ((*ptr == '\\') && (GETBIT(flags, FL_ESC)==0)) {
       SETBIT(flags, FL_ESC);
       #ifdef DEBUG
          fprintf(stderr, "Continue due to escape-char\n");
       #endif
       continue;
    }
    if (*ptr == '\"') {
        //Wenn 1. das vorhergehende Zeichen kein \ ist
        // und 2. wir nicht inmitten eines '-Wortes sind (singlequote)
       if ((GETBIT(flags, FL_ESC) == 0) &&
           (GETBIT(flags, FL_SQOT) == 0) ) {
           //...dann beginnt oder endet hier nun ein "-Wort (dblquote)
         SWPBIT(flags, FL_DQOT);
         SETBIT(flags, FL_QOTCHGD); //indikator für quote-grenze setzen
       }
    }
    if (*ptr == '\'') {
        //Wenn 1. das vorhergehende Zeichen kein \ ist
        // und 2. wir nicht inmitten eines "-Wortes sind (dblquote)
       if ((GETBIT(flags, FL_ESC) == 0) &&
           (GETBIT(flags, FL_DQOT) == 0) ) {
           //...dann beginnt oder endet hier nun ein '-Wort (singlequote)
         SWPBIT(flags, FL_SQOT);
         SETBIT(flags, FL_QOTCHGD); //indikator für wort-grenze setzen
       }
    }

    //so, nun sollte alles geklaert sein. wenn wir nicht inmitten eines
    //wortes sind, dann schauen wir mal, ob wir es mit einem
    //delimiter zu tun haben dem auch kein escape-Zeichen vorangeht:
    if (
            (GETBIT(flags, FL_SQOT) == 0)
         && (GETBIT(flags, FL_DQOT) == 0)
         && (GETBIT(flags, FL_ESC)  == 0)
       ) {
        for (p = delims; *p; p++)
           if (*ptr == *p) {
               SETBIT(flags, FL_DLM);
           }
    }

    //So, nun sind wir fast fertig: Wenn wir nicht auf einem delimiter stehen,
    //dann kopieren wir das aktuelle Zeichen in den puffer
    if (GETBIT(flags, FL_DLM) == 0)  {
      if (GETBIT(flags, FL_QOTCHGD) == 0) {
         *bufPos++ = *ptr;
         *bufPos = '\0';
      }
    } else {
        //Irgendwie müssen wir hier nun den Delimiter bearbeiten.
        if (GETBIT(flags, FL_DLMFLD) == 1) {
            //wenn delimiterfolding (FL_DLMFLD) aktiv ist, dann
            //dürfen wir keine leeren Wörter ausgeben:
            if (strlen(Buffer))
                SETBIT(flags, FL_WRDFIN);
        } else {
            //ansonsten ist uns das egal was in dem Wort steht.
            SETBIT(flags, FL_WRDFIN);
        }
        //ggf beginnen wir nun ein neues Wort und resetten den Buffer fuer
        //das naechste Wort:
        if (GETBIT(flags, FL_WRDFIN)) {
           *words = (char **)realloc(*(words), (wrdcnt + 1) * sizeof(char *));
           ((*words)[wrdcnt++]) = strdup(Buffer);
           bufPos = Buffer;
           *bufPos = '\0';
           //wenn maxWordCount definiert (>0) und erreicht ist, dann
           //setzen wir mal das Flag zum Iterationsabbruch
           if ((maxWrdCnt > 0) && (wrdcnt == maxWrdCnt)) {
              SETBIT(flags, FL_MAXWRD);
              #ifdef DEBUG
                 fprintf(stderr, "Break due to maxWrdCnt == %d\n", maxWrdCnt);
              #endif
           }
        }
    }

    #ifdef DEBUG
    fprintf(stderr, "%3d: %c: (0x%x = %d%d%d%d%d%d%d%d) %s\n",
                     wrdcnt, *ptr, (int)flags,
                     GETBIT(flags, FL_MAXWRD),
                     GETBIT(flags, FL_WRDFIN),
                     GETBIT(flags, FL_DLMFLD),
                     GETBIT(flags, FL_DLM),
                     GETBIT(flags, FL_QOTCHGD),
                     GETBIT(flags, FL_SQOT),
                     GETBIT(flags, FL_DQOT),
                     GETBIT(flags, FL_ESC),
                     Buffer
    );
    splitQuotedStr_DebugFlags(flags);
    #endif

    //Iterations-Abschluss:
    //nun alle noetigen Flags für den nächsten Durchlauf neu initialisieren
    CLRBIT(flags, FL_WRDFIN);
    CLRBIT(flags, FL_DLM);
    CLRBIT(flags, FL_ESC);
    CLRBIT(flags, FL_QOTCHGD); //indikator für wort-grenze setzen
  } //for ptr...

  //Wenn bereits innerhalb der Iteration die max. Anzahl von Wörtern erreicht
  //wurde, dann brauchen wir uns um ggf gelesenen Rest nicht mehr kuemmern:
  if (GETBIT(flags, FL_MAXWRD) == 0) {
    //...ansonsten schauen wir uns das doch gleich mal etwas genauer an...
    //es sei: das letzte Wort speichern, wenn was im Buffer ist, oder,
    //wenn Delimiterfolding inaktiv ist.
    if (
         strlen(Buffer) || ((GETBIT(flags, FL_DLMFLD) == 0) &&
                             GETBIT(flags, FL_WRDFIN == 1))
       ) {
       #ifdef DEBUG
          fprintf(stderr, "at the end: create last word from buffer.\n");
       #endif
       *(words) = (char **)realloc(*(words), (wrdcnt + 1) * sizeof(char*));
       ((*words)[wrdcnt++]) = strdup(Buffer);
    }
  }
  #ifdef DEBUG
   fprintf(stderr, "final flag-status: ");
   splitQuotedStr_DebugFlags(flags);
  #endif
  //so, zum Schluss noch etwas housekeeping...
  free(Buffer);
  if (GETBIT(flags, FL_DQOT) == 0 && GETBIT(flags, FL_SQOT) == 0) {
      return (wrdcnt);
  } else {
      int i = 0;
      for (i = 0; i < wrdcnt; i++)
        free((*words)[i]);
      return (-1);
  }
}


int quote_test (int argc, char *argv[]) {
  char str[] = "\'das, soll (mit \"qu\'o\"t\"i\'ng\")\' \"ein\'wirklich , "
               "\\\"wirklich\\\" \",\"langer und \'unsinniger\'\",,\'\"test-\"text"
               "\'und\\ \\,,, \\  so \"sein aber mit \'anfuehrungszeichen\' und esc\\\'s\",";
  char del[] = ", ";
  char **words = NULL;
  uint i = 0;


  if (argc > 0) {
    if (*argv[1] == '1')
       SETBIT(i, FL_DLMFLD);
  }
  int wrdcnt = splitQuotedStr(str, del, i, -1, &words);

  if (wrdcnt >= 0) {
     fprintf(stdout, "\nstr=[%s]\n", str);
     fprintf(stdout, "result follows\n");
     for (i = 0; i < wrdcnt; i++)
       fprintf(stdout, "words[%d]: [%s]\n", i, words[i]);
     //clean up...
       for (i = 0; i < wrdcnt; i++)
         free(words[i]);
  } else {
       fprintf(stdout, "\nERROR processing line: %d\n", wrdcnt);
  }
  free(words);
  return(0);
}
