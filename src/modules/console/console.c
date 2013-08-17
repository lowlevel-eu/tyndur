#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include "syscall.h"
#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <init.h>
#include <lostio.h>

/// Konsolen-Handle
typedef struct 
{
    /// PID des Prozesses dem die Konsole ghoert
    pid_t pid;

    /// Pfad fuer stdin
    char* stdin;

    /// Pfad fuer stdout
    char* stdout;

    /// Pfad fuer stderr
    char* stderr;
} console_t;

/// Liste mit den Konsolen-Handles
list_t* consoles;

/// Aufbau des Kommandos zum setzen der Pfade von Kanaelen fuer neue oder
/// bestehende Prozesse
typedef struct {
    /// PID
    pid_t pid;

    /// Bestimmt welcher Pfad geaendert werden soll
    enum {
        STDIN,
        STDOUT,
        STDERR 
    } type;

    /// Der neue Pfad
    char path[];
} __attribute__((packed)) console_ctrl_t;

/// RPC Handler zum setzen der Pfade
void rpc_console_set(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data);



/// LostIO-Handler zum auslesen der Pfade
size_t stdin_read (lostio_filehandle_t* file, void* buf, size_t blocksize,
    size_t blockcount);

/// Enstprechendes Typehandle
typehandle_t typehandle_stdpath = {
    .id = 255,
    .not_found = NULL,
    .pre_open = NULL,
    .post_open = NULL,
    .read = &stdin_read,
    .write = NULL,
    .seek = NULL,
    .close = NULL,
    .link = NULL,
    .unlink = NULL
};


/// Standardwerte fuer die einzelnen Kanaele
char* default_stdout = NULL;
char* default_stderr = NULL;
char* default_stdin = NULL;

/// Kommandozeilenargumente verarbeiten
void parse_cmdline(int argc, char* argv[]);


/**
 * Hauptfunktion (na wer haette das gedacht? ;-))
 */
int main(int argc, char* argv[])
{
    // Kommandozeilenargumente verarbeiten
    parse_cmdline(argc, argv);

    // LostIO initialisieren
    lostio_init();

    /// LostIO-Typehandles registrieren
    lostio_type_directory_use();
    lostio_register_typehandle(&typehandle_stdpath);
    
    // VFS-Nodes fuer die einzelnen Kanaele anlegen
    vfstree_create_node("/stdin" , 255, 0, NULL, 0);
    vfstree_create_node("/stdout", 255, 1, NULL, 0);
    vfstree_create_node("/stderr", 255, 2, NULL, 0);
    

    // RPC zum setzen der Konsolen-Pfade registrieren
    register_message_handler("CONS_SET", &rpc_console_set);
    
    // Liste fuer Konsolen-Handles erstellen
    consoles = list_create();
    
    // Bei init registrieren und auf RPCs warten
    init_service_register("console");
    while(true) {
        wait_for_rpc();
    }
}

/**
 * Kommandozeilenargumente verarbeiten:
 * [stdio [stdin [stderr]]]
 */
void parse_cmdline(int argc, char* argv[])
{
    // Wenn mindestens ein Argument uebergeben wurde wird dies als standard
    // stdout und stderr-Pfad hergenommen
    if (argc > 1) {
        default_stdout = argv[1];
        default_stderr = argv[1];
        // Der 2. Parameter ist fuer stdin
        if (argc > 2) {
            default_stdin = argv[2];
            // stderr wird mit dem 3. ueberschrieben
            if (argc > 3) {
                default_stderr = argv[3];
            }
        }
    }
}

/**
 * Gibt eine Konsole fuer den uebergebenen Prozess zurueck, Wenn dem Prozess
 * bereits eine Konsole zugeordnet war, wird diese zurueckgegeben, ansonsten
 * wird eine neue erzeugt
 *
 * @param pid PID des Prozesses, dessen Konsole zurueckgegeben werden soll
 * @param create Wenn die Konsole nicht existiert, und hier true uebergeben
 *                  wird, wird eine neue Konsole erstellt
 */
static console_t* get_console(pid_t pid, bool create)
{
    int consolectr = 0;
    console_t* console = NULL;

    // Pruefen, ob der Prozess bereits eine Konsole hat
    while ((console = list_get_element_at(consoles, consolectr++)) != NULL) {
        if (console->pid == pid) {
            return console;
        }
    }
    
    // Wenn keine erstellt werden soll, wird abgebrochen
    if (create == false) {
        return NULL;
    }

    // Wenn nicht, dann muss ein neue Konsole her
    console = malloc(sizeof(console_t));
    console->pid = pid;

    // Wenn moeglich werden die Infos vom Elternprozess kopiert
    console_t* parent_console = get_console(get_parent_pid(pid), false);
    if (parent_console != NULL) {
        // FIXME: Sobald beim Ableben von anderen Prozessen die Konsole
        // geschlossen und gefreet wird, muss man hier aufpassen...
        console->stdout = parent_console->stdout;
        console->stdin = parent_console->stdin;
        console->stderr = parent_console->stderr;
    } else {
        // FIXME: Diese Werte gehoeren hier nicht hartkodiert hin
        console->stdout = default_stdout;
        console->stdin  = default_stdin;
        console->stderr = default_stderr;
    }
    
    list_push(consoles, console);

    return console;
}

/**
 * LostIO-Handler der von den Prozessen benutzt wird, um an ihre
 * standard Ein- und Ausgabekanaele zu kommen.
 */
size_t stdin_read (lostio_filehandle_t *file, void* buf, size_t blocksize,
    size_t blockcount)
{
    console_t* console = get_console(file->pid, true);
    char* path = NULL;

    // Im Feld fuer die Groesse des LostIO-Nodes Speichern wir eine
    // Identifikationszahl um herausfinden zu können, welches Filehandle
    // ausgelesen werden soll
    switch (file->node->size) {
        case 0:
            path = console->stdin;
            break;

        case 1:
            path = console->stdout;
            break;

        case 2:
            path = console->stderr;
            break;
    }

    // Wenn kein Pfad gesetzt ist muss auch nichts zurückgeben werden. ;-)
    if (path == NULL) {
        return 0;
    }

    p();

    // Groesse berechnen
    size_t totalsize = blocksize * blockcount;
    size_t path_length = strlen(path);

    if (file->pos >= path_length) {
        totalsize = 0;
        goto out;
    } else if (file->pos + totalsize > path_length) {
        totalsize = path_length - file->pos;
    }

    // Falls der Platz ausreicht werden die noetigen Daten gesendet
    memcpy(buf + file->pos, path + file->pos, totalsize);
    file->pos += totalsize;

out:
    if (file->pos >= path_length) {
        file->flags |= LOSTIO_FLAG_EOF;
    }

    v();
    return totalsize;
}

/**
 * RPC um die Pfade zu setzen, die ein Prozess fuer die einzelnen Ein- und
 * Ausgabe Kanaele benutzen wird/soll.
 */
void rpc_console_set(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    console_ctrl_t* command = data;
    // Die Kommandostruktur in den RPC-Daten ueberspringen um an den Pfad zu
    // kommen, der direkt dahniter liegt.
    data_size -= sizeof(console_ctrl_t);

    // Bei PID -1 wird die eigene Konsole gesetzt
    if (command->pid == -1) command->pid = pid;
    
    // Konsolen-Handle suchen und ggf. erstellen
    console_t* console = get_console(command->pid, true);
    
    // Je nach Befehlstyp wird das entsprechende Handle gesetzt
    switch (command->type) {
        case STDIN:
            console->stdin = malloc(data_size + 1);
            strncpy(console->stdin, command->path, data_size);
            console->stdin[data_size] = '\0';
            break;

        case STDOUT:
            console->stdout = malloc(data_size + 1);
            strncpy(console->stdout, command->path, data_size);
            console->stdout[data_size] = '\0';
            break;

        case STDERR:
           console->stderr = malloc(data_size + 1);
           strncpy(console->stderr, command->path, data_size);
           console->stderr[data_size] = '\0';
           break;

        default:
            // Fehler, false zurueck senden
            rpc_send_dword_response(pid, correlation_id, 0);
            return;
    }

    // true antworten
    rpc_send_dword_response(pid, correlation_id, 1);
}

