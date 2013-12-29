/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <collections.h>
#include <io.h>

typedef enum {
    // Kein Seperator (nur am Anfang und am Ende moeglich)
    NO_SEP,
    // Service Seperator ":/"
    SERVICE_SEP,
    // Normaler Pfad-Seperator "/"
    PATH_SEP,
    // Seperator fuer gepipte Pfade "|"
    PIPE_SEP
} path_sep_t;

struct path_element {
    path_sep_t left_sep;
    path_sep_t right_sep;

    char text[MAX_FILENAME_LEN + 1];
};


/**
 * Alloziert Speicher fuer eine Element-Struktur, fuellt sie, und gibt einen
 * Pointer darauf zurueck
 *
 * @param text Pointer auf den Anfang der Element-Textes
 * @param len Laenge des Element-Textes
 * @param left_sep Linker Seperator
 * @param right_sep Rechter Seperator
 *
 * @return Pointer auf neues Element
 */
static inline struct path_element* create_path_element(const char* text,
    size_t len, path_sep_t left_sep, path_sep_t right_sep)
{
    struct path_element* element = malloc(sizeof(struct path_element));
    
    // Text auf die maximal MAX_FILENAME_LEN Zeichen zurechtstutzen. Dabei wird
    // einfach alles knallhart abgeschnitten. Aber so lange Verzeichnis- und
    // Dateinamen benutzt man einfach nicht.
    if (len > MAX_FILENAME_LEN) {
        len = MAX_FILENAME_LEN;
    }
    memcpy(element->text, text, len);
    element->text[len] = '\0';

    element->left_sep = left_sep;
    element->right_sep = right_sep;

    return element;
}

/**
 * Zerstueckelt einen Pfad in die einzelnen Elemente. Dies laesst sich am
 * besten ahnhand eines Beispiels zeigen:
 * ide:/hd1a|fat:/
 *  Element 0: ide
 *  Element 1: hd1a
 *  Element 2: fat
 *
 * Dabei wird zu jedem Element abgespeichert, welche Seperatoren es links und
 * rechts von den anderen trennen. Die Elemente werden in die uebergebene Liste
 * ab {position} in umgekehrter Reihenfolge eingefuellt.
 *
 * @param path Der Pfad als String
 * @param lost Pointer auf die Liste, in die eingefuellt werden soll
 * @param position Position an die die Elemente in die Liste sollen
 */
static void get_path_elements(const char* path, list_t* list, int position)
{
    bool escaped = false;
    const char* pos = path;
    const char* last_element = path;
    path_sep_t left_sep = NO_SEP;
    struct path_element* element = NULL;

    // Pfad Zeichenweise durchgehen
    while (true) {
        // Wenn das letzte Zeichen ein Escape-Zeichen war, muss das aktuelle
        // nicht beruecksichtigt werden
        if (escaped == true) {
            escaped = false;
        } else {
            size_t cur_size = (uintptr_t) pos - (uintptr_t) last_element;
            
            switch(*pos) {
                // Wenn das aktuelle Zeichen ein Escape ist, wird escaped auf
                // true gesetzt, damit das naechste Zeichen nicht beachtet wird
                case '\\':
                    // Das abschliessende Null-Byte darf natuerlich nicht
                    // ignoriert werden, da wir sonst ueber den String
                    // rausrasseln ohne es zu merken.
                    if (*(pos + 1) != '\0') {
                        escaped = true;
                    }
                    break;
                
                // Auf Service-Seperator :/ Pruefen
                case ':':
                    if (*(pos + 1) == '/') {
                        element = create_path_element(last_element, cur_size,
                            left_sep, SERVICE_SEP);

                        // Da dieser Seperator 2 Zeichen lang ist, muss pos
                        // manuell schon einmal inkrementiert werden;
                        pos++;
                    }
                    break;

                case '/':
                    // Wenn ganz am Anfang ein Schraegstrich steht, soll kein
                    // leeres Pfadelement erstellt werden, sondern nur der
                    // linke Separator des folgenden Elements gesetzt werden.
                    if (cur_size != 0) {
                        element = create_path_element(last_element, cur_size,
                            left_sep, PATH_SEP);
                    } else {
                        left_sep = PATH_SEP;
                        last_element = pos + 1;
                    }
                    break;

                case '|':
                    element = create_path_element(last_element, cur_size,
                        left_sep, PIPE_SEP);
                    break;

                // String-Ende
                case '\0':
                    element = create_path_element(last_element, cur_size,
                        left_sep, NO_SEP);
                    break;
            }
        }

        // Wenn im aktuellen durchlauf ein Element gefunden wurde, werden die
        // Pointer fuer den Anfang des naechhsten Elements und dessen linker
        // Seperator gesetzt
        if (element != NULL) {
            left_sep = element->right_sep;
            last_element = pos + 1;
            
            // Das Element in die Liste einfuegen
            list_insert(list, position, element);

            // Pointer wieder auf NULL setzen, damit wir im naechsten Durchlauf
            // nicht wieder hier landen, obwohl gar kein Element zu Ende war.
            element = NULL;
        }
        
        // Am Ende angekommen?
        if (*pos == '\0') {
            break;
        }

        // Pointer auf das naechste Zeichen zeigen lassen
        pos++;
    }
}

/**
 * Ueberarbeitet einen Pfad, der in die einzelnen Elemente zerlegt als liste
 * uebergeben wird, so dass keine Dot und Dotdot-Elemente mehr vorkommen. Diese
 * Funktion funkioniert aber nur mit Absoluten Pfaden, denn bei einem relativen
 * Pfad mit .. am Anfang wuerde die Funktion das einfach abschneiden.
 *
 * @param list Liste mit den Elementen
 */
static void eliminate_dot_elements(list_t* list)
{
    int i = 0;
    int dotdot = 0;
    struct path_element* element;
    while ((element = list_get_element_at(list, i))) {
        // Bei Dotdot wird das aktuelle geloescht und dotdot inkrementiert,
        // damit die naechsten Elemente entsprechend geloescht werden
        if (strcmp(element->text, "..") == 0) {
            list_remove(list, i);
            free(element);
            dotdot++;
        }
        // Dot-Elemente und leere werden einfach verworfen
        else if ((strcmp(element->text, ".") == 0) || (strlen(element->text)
            == 0))
        {
            list_remove(list, i);
            free(element);
        }
        // Alles andere bleibt gleich
        else {
            // Wenn Dotdot-Elemente da waren, muss das Element geloescht werden
            // und dotdot dekrementiert. Das erste Pfadelement (der Service)
            // darf dabei nie geloescht werden. Eine Pipe ist nicht das Ziel
            // von einem .. - in dem Fall muss man noch eins weiter gehen
            if ((dotdot != 0) && (i < list_size(list) - 1)) {
                list_remove(list, i);
                if (element->left_sep != PIPE_SEP) {
                    dotdot--;
                }
                free(element);
            } else {
                i++;
            }
        }
    }
}

/**
 * Verarbeitet einen Pfad relativ zum aktuellen Verzeichnis
 *
 * @param list Pfad als Liste
 */
static inline void resolve_relative_path(list_t* list)
{
    char* cwd = getcwd(NULL, 0);
    struct path_element* element;
    int last_element = list_size(list);

    // CWD-Elemente holen und an Liste anhaengen
    get_path_elements(cwd, list, last_element);

    // Beim letzten Element muss noch der Seperator angepasst werden
    // TODO: Stimmt das?
    element = list_get_element_at(list, last_element);
    element->right_sep = PATH_SEP;

    free(cwd);
}

/**
 * Verarbeitet einen Pfad relativ zum aktuellen Service verarbeiten. Dabei
 * werden von CWD erst alle verzeichnisse bis zum hintersten Service abgetrennt
 * werden, und danach wird das was uebrig bleibt vor den anderen Pfad
 * geschrieben (Achtung: Auch hier wieder ist die Liste in der umgekehrten
 * Reihenfolge).
 *
 * @param list Pfad als Liste
 */
static inline void resolve_relative_serv_path(list_t* list)
{

    // Temporaere Liste fuer die CWD-Elemente erstellen
    list_t* cwd_list = list_create();

    // CWD-Elemente holen und an Liste anhaengen
    char* cwd = getcwd(NULL, 0);
    get_path_elements(cwd, cwd_list, 0);
    free(cwd);
    struct path_element* element;
    
    
    // Jetzt werden alle Elemente bis zum Service-Element verworfen
    while ((element = list_get_element_at(cwd_list, 0)) && (element->
        right_sep != SERVICE_SEP))
    {
        list_remove(cwd_list, 0);
        free(element);
    }

    // Die uebrig gebliebenen Elemente werden an die Pfad-Liste angehaengt
    int last_element = list_size(list);
    int i = list_size(cwd_list);
    while ((element = list_get_element_at(cwd_list, --i))) {
        list_insert(list, last_element, element);
    }
    
    list_destroy(cwd_list);
}

/**
 * Berechnet wieviel Speicher ein Pfad in Listenform benoetigt, wenn er in
 * einen String umgewandelt wird
 *
 * @param list Liste mit den Elementen
 *
 * @return Anzahl der notwendigen Bytes (ohne abschliessendes Nullbyte)
 */
static inline size_t calc_path_length(list_t* list)
{
    size_t size = 0;
    int i = 0;
    struct path_element* element;
    while ((element = list_get_element_at(list, i++))) {
        size += strlen(element->text);
        // Berechnen, wieviel Speicher der Seperator belegen wird
        switch (element->right_sep) {
            case SERVICE_SEP:
                size += strlen(":/");
                break;

            case PATH_SEP:
            case PIPE_SEP:
                // strlen("/") resp. strlen("|");
                size += 1;
                break;

            case NO_SEP:
                // Der brauch garnichts, aber sonst warnt gcc
                break;
        }
    }
    
    return size;
}

/**
 * Wandelt den Pfad von der Listen-Form in einen String um, und gibt alle
 * Elemente Frei. 
 *
 * @param list Pfad als Liste
 * @param buffer Puffer in dem der String gespeichert werden soll
 * @param free_element true, falls die elemente gefreet werden sollen
 */
static inline void create_path_string(list_t* list, char* buffer,
    bool free_element)
{
    // Jetzt muss der Pfad rueckwaerts abgearbeitet werden, damit er im String
    // in der richtigen reihenfolge ist
    int i = list_size(list);
    struct path_element* element;
    while ((element = list_get_element_at(list, --i))) {
        // Element-Text kopieren
        strcpy(buffer, element->text);
        buffer += strlen(element->text);
        
        // Seperator einfuegen
        switch (element->right_sep) {
            case SERVICE_SEP:
                *(buffer++) = ':';
                *(buffer++) = '/';
                break;

            case PATH_SEP:
                *(buffer++) = '/';
                break;

            case PIPE_SEP:
                *(buffer++) = '|';
                break;

            case NO_SEP:
                // Nur um Warnungen zu verhindern
                break;
        }
        
        if (free_element == true) {
            free(element);
        }
    }
    *buffer = '\0';
}

/**
 * Erstellt aus einem Beliebigen Pfad einen absoluten
 *
 * @param path Pointer auf dem Pfad
 *
 * @return Pointer auf ein neu alloziertes Stueck Speicher in dem der neue Pfad
 *          liegt. Also free() nicht vergessen ;-)
 */
char* io_get_absolute_path(const char* path)
{
    struct path_element* element;
    // Liste fuer die Elemente des Pfades
    list_t* element_stack;

    // Ein leerer Pfad muss garnicht erst behandelt werden
    if (!*path) {
        return NULL;
    }

    element_stack = list_create();
    // Elemente holen, und an Liste anhaengen
    get_path_elements(path, element_stack, 0);

    
    // Jetzt werden relative Pfade aufgeloest
    // Dafuer gibt es 2 verschiedene Moeglichkeiten. Die eine sind die Pfade
    // relativ zum aktuellen Verzeichnis, wie man sie zum Beispiel von Linux
    // kennt, die anderen beginnen mit einem / und sind dadurch relativ zum
    // aktuellen Service.
    //
    // Beispiel:
    // CWD sei ide:/hda1|fat:/data
    // Dann wird ein "foo/bar" zu "ide:/hda1|fat:/data/foo/bar"
    // Und ein "/test" zu "ide:/hda1|fat:/test"
    //
    // Entschieden wird das anhand des ersten Elements des Pfades. Wenn kein
    // Serperator links ist, handelt es sich um einen Pfad relativ zum
    // aktuellen Verzeichnis oder um einen absoluten, wenn der Seperator rechts
    // kein Service-Seperator ist, denn dann ist er relativ zum Verzeichnis,
    // sonst absolut. Die Pfade relativ zum aktuellen Service werden einfach
    // anhand des Pfad-Seperators Links erkannt.
    
    // Das erste Element ist aufgrund der Reihenfolge in der Liste das letzte,
    // also das mit dem hoechsten Index.
    int last_element = list_size(element_stack) - 1;
    element = list_get_element_at(element_stack, last_element);
    
    if ((element->left_sep == NO_SEP) && (element->right_sep != SERVICE_SEP)) {
        // Pfad relativ zum aktuellen Verzeichnis
        resolve_relative_path(element_stack);
    } else if (element->left_sep == PATH_SEP) {
        // Pfad relativ zum aktuellen Serivice
        resolve_relative_serv_path(element_stack);
    } 
    
    // Dot und Dotdot Elemente eliminieren
    eliminate_dot_elements(element_stack);

    // Ueberflussigen Schraegstrich am Ende streichen
    element = list_get_element_at(element_stack, 0);
    if (element && (element->right_sep == PATH_SEP)) {
        element->right_sep = NO_SEP;
    }
    
    // Berechnen, wieviel Speicher der fertige Pfad belegt
    size_t size = calc_path_length(element_stack);
    
    // Jezt ist es soweit, der neue Pfad kann kopiert werden
    char* new_path = malloc(size + 1);
    create_path_string(element_stack, new_path, true);

    list_destroy(element_stack);
    return new_path;
}

/**
 * Trennt aus einem Pfad den Dateinamen (d.h. das letzte Pfadelement) heraus.
 * Es wird keine Ueberpruefung vorgenommen, ob die Datei existiert.
 *
 * @param path Zu splittender Pfad
 * @return Name der durch den Pfad angesprochenen Datei
 */
char* io_split_filename(const char* path)
{
    struct path_element* element;
    
    // Liste fuer die Elemente des Pfades erstellen
    list_t* element_stack = list_create();
    get_path_elements(path, element_stack, 0);
    
    // Dot und Dotdot Elemente eliminieren
    eliminate_dot_elements(element_stack);

    // Das letzte nichleere Element des Pfads (erstes in der Liste)
    // herausgreifen
    element = list_get_element_at(element_stack, 0);
    while (element && !*element->text && element->left_sep == PATH_SEP) {
        element = list_pop(element_stack);
        free(element);
        element = list_get_element_at(element_stack, 0);
    }

    if (element != NULL) {
        char* result;
        size_t length;

        // Den Ergebnisstring anlegen
        length = strnlen(element->text, MAX_FILENAME_LEN);
        if ((result = malloc(length + 1))) {
            strncpy(result, element->text, length);
            result[length] = '\0';
        }

        // Freigeben der Liste
        while ((element = list_pop(element_stack))) {
            free(element);
        }
        list_destroy(element_stack);

        return result;
    } else {
        list_destroy(element_stack);
        return strdup("");
    }
}

/**
 * Trennt aus einem Pfad den Pfad zum Verzeichnis ab, in dem das oberste
 * Element liegt. (d.h. das letzte Pfadelement wird abgeschnitten) und wandelt
 * ihn gegebenenfalls in einen absoluten Pfad um.
 * Es wird keine Ueberpruefung vorgenommen, ob die Datei existiert.
 *
 * @param path Zu splittender Pfad
 *
 * @return Pfad zur angegebenen Datei
 */
char* io_split_dirname(const char* path)
{
    char* dirname;
    list_t* list = list_create();
    size_t size;
    struct path_element* element;

    get_path_elements(path, list, 0);

    element = list_pop(list);
    if (list_size(list) == 0) {
        // Spezialfall, Pfad mit nur einem Element
        if (element->left_sep == PATH_SEP) {
            // Ein Service-relativer Pfad wird zu /
            element->left_sep = NO_SEP;
            element->right_sep = PATH_SEP;
            element->text[0] = 0;
        } else if (element->left_sep == NO_SEP) {
            // Ein Verzeichnis-relativer Pfad zu .
            element->text[0] = '.';
            element->text[1] = 0;
        }
        list_push(list, element);
    } else {
        free(element);

        // Bei Service-relativen Pfaden muessen wir noch sicherstellen, dass da
        // am Anfang auch ein / drin steht (create_path_string benutzt
        // right_sep).
        element = list_get_element_at(list, list_size(list) - 1);
        if (element->left_sep == PATH_SEP) {
            element = malloc(sizeof(*element));
            element->left_sep = NO_SEP;
            element->right_sep = PATH_SEP;
            element->text[0] = 0;
            list_insert(list, list_size(list), element);
        }
    }



    size = calc_path_length(list);
    dirname = malloc(size + 1);
    create_path_string(list, dirname, true);
    list_destroy(list);
    return dirname;
}

