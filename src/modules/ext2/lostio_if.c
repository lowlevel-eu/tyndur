/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <lostio.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#include "lostio_if.h"
#include "libext2/ext2.h"

#define debug_print(x) puts(x)

// So lang kann der Root-Pfad maximal werden
#define ROOT_PATH_SIZE 27
#define FULL_PATH_SIZE(x) (strlen(x) + ROOT_PATH_SIZE)

/// Verzeichnis-Handle
typedef struct {
    /// Inode-Nummer
    uint32_t inode;

    /// TRUE, falls das Verzeichnis geladen ist
    bool loaded;

    /// Root-Handle
    ext2_root_handle_t* root_handle;
} dir_handle_t;

/**
 * Diese Funktion generiert den Root-Pfad zu einem Source-Handle
 *
 * @param source Source-Handle
 * @param buffer Puffer, in dem der Pfad abgelegt werden soll (muss mindestens
 *               MAX_ROOT_PATH_SIZE Bytes lang sein.
 */
static inline void get_root_path(io_resource_t* source, char* buffer)
{
    snprintf(buffer, ROOT_PATH_SIZE, "/%08d_%016d", source->pid, (dword)
        source->resid);
}

/**
 * Diese Funktion generiert den vollen Pfad zu einem Source-Handle
 *
 * @param source Source-Handle
 * @param path Pfad
 * @param buffer Puffer, in dem der Pfad abgelegt werden soll (muss mindestens
 *               FULL_PATH_SIZE Bytes lang sein.
 */
static inline void get_full_path(io_resource_t* source, char* path,
    char* buffer)
{
    size_t size = FULL_PATH_SIZE(path);
    // Abschliessender Slash rauswerfen
    if (path[strlen(path) - 1] == '/') {
        size--;
    }

    snprintf(buffer, size, "/%08d_%016d%s", source->pid,
        (dword) source->resid, path);
}

/**
 * Kindknoten erstellen
 *
 * @param parent Elternknoten
 * @param name Name
 * @param inode Inodenummer
 * @param type 1 fuer Normale Datei; 2 fuer Verzeichnis; 3 fuer Symlink
 */
static void create_child_node(ext2_root_handle_t* root_handle,
    vfstree_node_t* parent, const char* name, uint32_t inode, int type)
{
    // Vezeichnishandle erstellen
    dir_handle_t* dir_handle = malloc(sizeof(dir_handle_t));
    dir_handle->inode = inode;
    dir_handle->loaded = FALSE;
    dir_handle->root_handle = root_handle;
    
    // Je nach Typ kann jetzt der richtige Knoten erstellt werden
    if (type == 2) {
        vfstree_create_child(parent, (char*) name, EXT2_LOSTIO_TYPE_DIR, 0,
            dir_handle, LOSTIO_FLAG_BROWSABLE);
    } else if (type == 3) {
        // Um bei Symlinks die Groesse eintragen zu koennen, muss erst der
        // Inode geladen werden.
        ext2_ih_t inode_struct;
        ext2_ih_init(root_handle, &inode_struct, inode);

        vfstree_create_child(parent, (char*) name, EXT2_LOSTIO_TYPE_SYMLINK,
            inode_struct.data->size, dir_handle, LOSTIO_FLAG_SYMLINK);
        ext2_ih_destroy(root_handle, &inode_struct);
    } else {
        // Um bei Dateien die Groesse eintragen zu koennen, muss erst der Inode
        // geladen werden.
        ext2_ih_t inode_struct;
        ext2_ih_init(root_handle, &inode_struct, inode);

        vfstree_create_child(parent, (char*) name, EXT2_LOSTIO_TYPE_FILE,
            inode_struct.data->size, dir_handle, 0);
        ext2_ih_destroy(root_handle, &inode_struct);
    }
}

/**
 * Hilfsfunktion um Verzeichnisse zu parsen
 */
static int parse_dir(void* handle, ext2_dir_entry_t* dir_entry)
{
    vfstree_node_t* node = (vfstree_node_t*) handle;
    dir_handle_t* parent_handle = (dir_handle_t*) node->data;
    /*// Verzeichnis-Handle allozieren und fuellen
    dir_handle_t* dir_handle = malloc(sizeof(dir_handle_t));
    dir_handle->inode = dir_entry->inode;
    dir_handle->loaded = FALSE;
    dir_handle->root_handle = parent_handle->root_handle;
    //printf("direntry:'%s'\n", dir_entry->name);
    // Typ korrekt setzen und Knoten erstellen
    if (dir_entry->type == DIRECTORY) {
        vfstree_create_child(node, dir_entry->name, EXT2_LOSTIO_TYPE_DIR, 0,
            dir_handle, LOSTIO_FLAG_BROWSABLE);
    } else {
        ext2_inode_t inode;
        ext2_inode_read(dir_handle->root_handle, dir_handle->inode, &inode);

        vfstree_create_child(node, dir_entry->name, EXT2_LOSTIO_TYPE_FILE,
            inode.size, dir_handle, 0);
    }*/
    int type;
    if (dir_entry->type == DIRECTORY) {
        type = 2;
    } else if (dir_entry->type == SYMLINK) {
        type = 3;
    } else {
        type = 1;
    }

    create_child_node(parent_handle->root_handle, node, dir_entry->name,
        dir_entry->inode, type);
    return 0;
}

/**
 * Erstellt das Root-Verzeichnis
 *
 * @param source Source-Handle
 */

static bool create_root_dir(io_resource_t* source,
    ext2_root_handle_t* root_handle)
{
    bool result = TRUE;

    // Zuerst mal den Root-Pfad holen
    char buf[ROOT_PATH_SIZE];
    get_root_path(source, buf);
    
    // Verzeichnis-Handle alloziieren
    dir_handle_t* dir_handle = malloc(sizeof(dir_handle_t));
    // FIXME
    dir_handle->inode = EXT2_ROOTDIR_INODE;
    dir_handle->loaded = FALSE;
    dir_handle->root_handle = root_handle;

    // Knoten im LostIO-Baum erstellen
    vfstree_create_node(buf, EXT2_LOSTIO_TYPE_DIR, 0, dir_handle,
        LOSTIO_FLAG_BROWSABLE);
    vfstree_node_t* node = vfstree_get_node_by_path(buf);
    
    // Verzeichnis fuellen
    ext2_ih_t inode;
    ext2_ih_init(root_handle, &inode, dir_handle->inode);
    if (ext2_dir_foreach_entry(root_handle, &inode, parse_dir, node) != 0)
    {
        result = FALSE;
    }
    ext2_ih_destroy(root_handle, &inode);

    return result;
}

/**
 * Liest Verzeichniseintraege in den LostIO-Baum ein.
 *
 * @param path Pfad
 * @param type Einer der folgenden Werte:
 *              1: Datei
 *              2: Verzeichnis
 *              3: Symlink
 * @param create TRUE wenn das Verzeichnis/ die Datei neu erstellt werden darf
 *
 * @return TRUE bei Erfolg, FALSE im Fehlerfall
 */
static bool read_dir(const char* path, int type, bool create,
    ext2_root_handle_t* root_handle, vfstree_node_t* root_node)
{
    // String elementweise durchgehen
    // TODO: Koennte hier eventuell etwas aus modules/lib/lost_path.c benutzt
    // werden?
    vfstree_node_t* node = root_node;
    vfstree_node_t* subnode;

    char* element = strtok((char*) path, "/");
    while (element != NULL) {
        // Kindknoten durchsuchen
        int i;
        for (i = 0; (subnode = list_get_element_at(node->children, i)); i++) {
            // Wenn der richtige Kindknoten gefunden wurde, ist die Suche
            // beendet
            if (strcmp(element, subnode->name) == 0) {
                break;
            }
        }
        
        // Falls kein passender Knoten gefunden wurde, und wir entweder nicht
        // auf der untersten Ebene sind, oder keine Eintraege erstellt werden
        // duerfen ist fertig.
        if ((subnode == NULL) && (create == TRUE)) {
            // Jetzt wird das aktuelle Element gespeichert da wir das naechste
            // brauchen um fest zu stellen, ob wir auf der obersten Ebene sind.
            size_t element_len = strlen(element);
            char old_element[element_len + 1];
            memcpy(old_element, element, element_len + 1);

            // Naechstes Element suchen
            element = strtok(NULL, "/");

            // Wenn wir noch nicht auf der obersten Ebene sind, dann reicht es
            // nicht einen Eintrag zu erstellen
            if (element != NULL) {
                break;
            }
            element = old_element;

            // Sonst wird jetzt der Eintrag erstellt:
            dir_handle_t* parent_handle = (dir_handle_t*) node->data;
            bool res = TRUE;
            ext2_ih_t parent_inode;
            ext2_ih_init(root_handle, &parent_inode, parent_handle->inode);
            if (type == 2) {
                // Verzeichnis erstellen
                uint32_t inode_num;
                if (ext2_dir_create(root_handle, &parent_inode, element,
                    &inode_num) != 0)
                {
                    print_error("Fehler beim Erstellen des Verzeichnisses");
                    res = FALSE;
                } else {
                    create_child_node(root_handle, node, element, inode_num,
                        2);
                }
            } else if (type == 3) {
                // Symlink erstellen
                uint32_t inode_num;
                if (ext2_symlink_create(root_handle, &parent_inode, element,
                    &inode_num) != 0)
                {
                    print_error("Fehler beim Erstellen des Verzeichnisses");
                    res = FALSE;
                } else {
                   create_child_node(root_handle, node, element, inode_num, 3);
                }
            } else {
                // Datei erstellen
                uint32_t inode_num;
                if (ext2_file_create(root_handle, &parent_inode, element,
                    &inode_num) != 0)
                {
                    print_error("Fehler beim Erstellen der Datei");
                    res = FALSE;
                } else {
                    create_child_node(root_handle, node, element, inode_num,
                        1);
                }
            }
            ext2_ih_destroy(root_handle, &parent_inode);
        
            return res;
       } else if (subnode == NULL) { 
            // Ganz einfach: Eintrag nicht gefunden ;-)
            return FALSE;
        }

        //printf("Subnode: %x\n", (uintptr_t) subnode);
        dir_handle_t* dir_handle = (dir_handle_t*) subnode->data;
        if ((dir_handle->loaded == FALSE) && ((subnode->flags &
            LOSTIO_FLAG_BROWSABLE) != 0))
        {
            ext2_ih_t inode;
            ext2_ih_init(dir_handle->root_handle, &inode, dir_handle->inode);

            // Wenn einer gefunden wurde, der aber noch nicht geladen ist,
            // wird's jetzt hoechste Zeit
            int result = ext2_dir_foreach_entry(dir_handle->root_handle,
                &inode, parse_dir, subnode);
            ext2_ih_destroy(dir_handle->root_handle, &inode);
            if (result == 0) {
                dir_handle->loaded = TRUE;
            } else {
                print_error("Fehler beim Laden des Verzeichnisses");
                return FALSE;
            }
        }

        element = strtok(NULL, "/");
        node = subnode;
    }

    return TRUE;
}

static size_t dev_read(struct ext2_root_handle_t* rh, uint64_t start,
    size_t size, void* buffer)
{
    io_resource_t* res = rh->opaque;

    if (!lio_seek(res, start, SEEK_SET)) {
        return 0;
    }
    
    return fread(buffer, 1, size, res);
}

static size_t dev_write(struct ext2_root_handle_t* rh, uint64_t start,
    size_t size, const void* buffer)
{
    io_resource_t* res = rh->opaque;

    if (!lio_seek(res, start, SEEK_SET)) {
        return 0;
    }

    return fwrite(buffer, 1, size, res);
}
 

bool if_mount(io_resource_t* source)
{
    // Root-Handle fuer Mount vorbereiten
    ext2_root_handle_t* root_handle = malloc(sizeof(ext2_root_handle_t));

    // FIXME ;-)
    root_handle->opaque = malloc(sizeof(*source));
    *((io_resource_t*) root_handle->opaque) = *source;
    
    root_handle->dev_read = dev_read;
    root_handle->dev_write = dev_write;

    // Versuchen zu initialisieren
    if (ext2_mount(root_handle) == FALSE) {
        free(root_handle);
        return FALSE;
    }

    // Root-Verzeichnis erstellen
    bool result = create_root_dir(source, root_handle);
    list_push(root_handle_list, root_handle);
    return result;
}

/**
 * Dieser Handler wird bei jedem fopen() auf Fat aufgerufen, da das immer
 * schief gehen wird weil ja die daten nicht direkt auf Root liegen. Dabei
 * werden nur die Pfade richtig umgebeogen und es wird geprueft ob alle
 * notwendigen Verzeichnisse geladen sind
 */
bool lostio_not_found_handler(char** path, byte args, pid_t pid,
    io_resource_t* source)
{
    p();

    // Root-Pfad generieren
    char root_path[ROOT_PATH_SIZE + 1];
    get_root_path(source, root_path);
    
    // Jetzt wird der komplette Pfad generiert
    char full_path[FULL_PATH_SIZE(*path) + 1];
    get_full_path(source, *path, full_path);


    // Wenn der Knoten nicht existiert muss gemountet werden
    vfstree_node_t* root_node = vfstree_get_node_by_path(root_path);
    if (root_node == NULL) {
        if (if_mount(source) == FALSE) {
            // Wenn das mounten nicht klappt wird abgebrochen
            printf("Fehler beim Mount!");
            v();
            return FALSE;
        }

        root_node = vfstree_get_node_by_path(root_path);
    }
    
    // Wenn das Root-Verzeichnis noch nicht gefuellt ist, wird das jetzt
    // nachgeholt
    dir_handle_t* dir_handle = (dir_handle_t*) root_node->data;
    /*if ((dir_handle->loaded == FALSE) && (read_dir(root_path, dir_handle->
        root_handle, root_node) == FALSE))
    {
        printf("fehler beim Laden");
        return FALSE;
    }*/

    // Testen ob ein Verzeichnis oder eine Datei geoeffnet werden soll
    int type = 1;
    if ((args & IO_OPEN_MODE_DIRECTORY) != 0) {
        type = 2;
    } else if ((args & IO_OPEN_MODE_LINK) != 0) {
        type = 3;
    }

    // Pruefen ob elemente erstellt werden sollen
    bool create = FALSE;
    if ((args & IO_OPEN_MODE_CREATE) != 0) {
        create = TRUE;
    }
    
    // Verzeichnisse laden, falls noetig
    if (read_dir(*path, type, create, dir_handle->root_handle, root_node)
        == FALSE) 
    {
        v();
        //debug_print("read_dir failed");
        return FALSE;
    }
    
    // Pfad aendern
    *path = malloc(strlen(full_path) + 1);
    memcpy(*path, full_path, strlen(full_path) + 1);
    
    // Jetzt wird der Knoten geholt, damit geprueft werden kann, ob auch kein
    // Verzeichnis als Datei geoeffnet werden soll und sowas ;-)
    vfstree_node_t* node = vfstree_get_node_by_path(*path);
    if (node == NULL) {
        v();
        return FALSE;
    } else if ((node->type == EXT2_LOSTIO_TYPE_DIR) && (type != 2)) {
        v();
        return FALSE;
    } else if ((node->type == EXT2_LOSTIO_TYPE_FILE) && (type != 1)) {
        v();
        return FALSE;
    }
    v();
    return TRUE;
}

/**
 * Handler der dazu benutzt wird, die Pfade umzubiegen
 */
bool lostio_pre_open_handler(char** path, byte args, pid_t pid,
    io_resource_t* source)
{
    if ((strcmp(*path, "/") == 0) || (vfstree_get_node_by_path(*path) == NULL))
    {
        if (lostio_not_found_handler(path, args, pid, source) == FALSE) {
            return FALSE;
        }
    }
    //printf("Pfad:'%s'\n", *path);
    return TRUE;
}

/**
 *
 */
void lostio_post_open_handler(lostio_filehandle_t* filehandle)
{
    // Bei Verzeichnissen muss nichts Besonderes unternommen werden
    if ((filehandle->flags & IO_OPEN_MODE_DIRECTORY) != 0) {
        return;
    }

    // Spezielle Modi anwenden
    if ((filehandle->flags & IO_OPEN_MODE_TRUNC) != 0) {
        // Groesse auf 0 setzen und Daten loeschen
        dir_handle_t* dir_handle = (dir_handle_t*) filehandle->node->data;
        filehandle->node->size = 0;

        // Inode beschneiden
        ext2_ih_t inode;
        ext2_ih_init(dir_handle->root_handle, &inode, dir_handle->inode);
        ext2_inode_data_truncate(dir_handle->root_handle, &inode, 0);
        ext2_ih_destroy(dir_handle->root_handle, &inode);
    }

}

/**
 * Handler fuer Lesezugriff auf dateien
 */
read_hdl_reply lostio_read_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount)
{
    p();
    //printf("read_start");
    // Rueckgabewert, ein grosser Hack
    static read_hdl_reply reply = {.data = NULL};
    size_t size = blocksize * blockcount;
    
    dir_handle_t* dir_handle = (dir_handle_t*) filehandle->node->data;
    //printf("[  EXT2  ] Lese %d Bytes von %lld an\n", size, filehandle->pos);
    // Zuerst brauchen wir mal den Inode
    ext2_ih_t inode;
    if (!ext2_ih_init(dir_handle->root_handle, &inode, dir_handle->inode)) {
        v();
        printf("earlyend1");
        reply.size = 0;
        return reply;
    }
    
    // Datenmenge wenn noetig beschneiden
    if ((filehandle->pos + size) > inode.data->size) {
        //printf("%d beschneide %d => %d (%d, %d) %d %d!\n", dir_handle->inode, size, (int) (inode.data->size - filehandle->pos), inode.data->size, (int) filehandle->pos, blocksize, blockcount);
        size = inode.data->size - filehandle->pos;
    }
    
    
    // Wenn's nichts zu lesen gibt ist hier Endstation
    if (size == 0) {
        ext2_ih_destroy(dir_handle->root_handle, &inode);
        reply.size = 0;
        v();
        return reply;
    }

    // Buffer anpassen
    reply.data = realloc(reply.data, size);
    
    // Daten einlesen
    reply.size = ext2_inode_data_read(dir_handle->root_handle, &inode,
        filehandle->pos, size, reply.data);
    //printf("Effektif gelesen: %d\n", reply.size);
    // Position erneuern
    filehandle->pos += reply.size;
    
    // Wenn noetig EOF-Flag setzen
    if(filehandle->pos == inode.data->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }
    
    ext2_ih_destroy(dir_handle->root_handle, &inode);
    v();
    return reply;
}

/**
 * Handler fuer Schreibzugriff auf Dateien
 */
size_t lostio_write_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount, void* data)
{
    p();
    size_t size = blocksize * blockcount;
    dir_handle_t* dir_handle = (dir_handle_t*) filehandle->node->data;
    size_t result;

    // Zuerst brauchen wir mal den Inode
    ext2_ih_t inode;
    if (!ext2_ih_init(dir_handle->root_handle, &inode, dir_handle->inode)) {
        print_error("Fehler beim Lesen des Inodes!");
        result = 0;
        goto end;
    }
 
    // Wenn's nichts zu schreiben gibt ist hier fertig
    if (size == 0) {
        result = 0;
        goto end_unload;
    }
    //printf("schreiben: %d %d  %d  %d\n", dir_handle->inode, size, inode.size, (int) filehandle->pos);
    // Daten schreiben
    result= ext2_inode_data_write(dir_handle->root_handle, &inode,
        filehandle->pos, size, data);

    //ext2_sync(dir_handle->root_handle);
    
    // Groesse im Node anpassen
    filehandle->node->size = inode.data->size;

    // Cursor vorwaerts bewegen
    filehandle->pos += result;
    
    // Wenn noetig EOF-Flag setzen
    if(filehandle->pos >= inode.data->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }
    
end_unload:
    ext2_ih_destroy(dir_handle->root_handle, &inode);

end:
    v();
    return result;
}

int lostio_seek_handler(lostio_filehandle_t* filehandle, uint64_t offset,
    int origin)
{
    p();
    uint64_t new_pos;
    
    switch(origin)
    {
        case SEEK_SET:
            new_pos = offset;
            break;

        case SEEK_CUR:
            new_pos = filehandle->pos + offset;
            break;

        case SEEK_END:
            new_pos = filehandle->node->size;
            break;

        default:
            return -1;
    }

    if ((new_pos > filehandle->node->size) && ((origin == SEEK_SET) ||
        (origin == SEEK_CUR)) && ((filehandle->flags & IO_OPEN_MODE_WRITE) ==
        0))
    {
        v();
        return -1;
    }
    
    filehandle->pos = new_pos;

    //Auf EOF ueberpruefen
    if(filehandle->pos >= filehandle->node->size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }

    v();
    return 0;
}

int lostio_link_handler(lostio_filehandle_t* target_handle,
    lostio_filehandle_t* dir_handle, const char* name)
{
    dir_handle_t* target_dir_handle = (dir_handle_t*) target_handle->node->data;
    dir_handle_t* dir_dir_handle = (dir_handle_t*) dir_handle->node->data;
    int result = 0;
    p();

    // Keine Links ueber verschiedene Dateisysteme
    if (target_dir_handle->root_handle != dir_dir_handle->root_handle) {
        result = -1;
        goto end;
    }

    // Pruefen ob schon ein Eintrag mit dem selben Namen existiert
    // TODO

    // Ansonsten kann jetzt gelinkt werden
    ext2_ih_t dir_inode;
    ext2_ih_t target_inode;
    ext2_ih_init(dir_dir_handle->root_handle, &dir_inode, dir_dir_handle->
        inode);
    ext2_ih_init(dir_dir_handle->root_handle, &target_inode,
        target_dir_handle->inode);
    if (ext2_dir_link(target_dir_handle->root_handle, &dir_inode,
        &target_inode, name) != 0)
    {
        result = -2;
        goto end_unload;
    }
    //ext2_sync(dir_dir_handle->root_handle);

    // Typ fuer den LostIO-Knoten festlegen
    int type = 1;
    if (target_handle->node->type == EXT2_LOSTIO_TYPE_DIR) {
        type = 2;
    }

    // LostIO-Knoten erstellen
    create_child_node(target_dir_handle->root_handle, dir_handle->node,
        name, target_dir_handle->inode, type);

end_unload:
    ext2_ih_destroy(target_dir_handle->root_handle, &dir_inode);
    ext2_ih_destroy(target_dir_handle->root_handle, &target_inode);
end:
    v();
    return result;
}

int lostio_unlink_handler(lostio_filehandle_t* dir_handle, const char* name)
{
    dir_handle_t* dir_dir_handle = (dir_handle_t*) dir_handle->node->data;
    ext2_ih_t inode;
    int result;
    
    p();

    ext2_ih_init(dir_dir_handle->root_handle, &inode, dir_dir_handle->inode);
    
    // Versuchen Verzeichnis zu loeschen
    result = ext2_dir_delete(dir_dir_handle->root_handle, &inode, name);

    // Kein Verzeichnis
    if (result == 1)
    {
        // Ansonsten kann jetzt geloescht werden
        if (ext2_dir_unlink(dir_dir_handle->root_handle, &inode, name) != 0) {
            result = -1;
        }
    } else if (result != 0) {
        // Sonstiger fehler
        result = -1;
    }

    if (!result && vfstree_delete_child(dir_handle->node, name) == FALSE) {
        result = -2;
    }

    v();
    return result;
}


int lost_close_handler(lostio_filehandle_t* file_handle)
{
    return 0;
}

/**
 * Handler fuer Lesezugriff auf Symlinks
 */
read_hdl_reply lostio_read_symlink_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount)
{
    p();
    puts("read symlink...");
    // Rueckgabewert, ein grosser Hack
    static read_hdl_reply reply = {.data = NULL};
    dir_handle_t* dir_handle = (dir_handle_t*) filehandle->node->data;
    ext2_ih_t inode;
    
    // Zuerst brauchen wir mal den Inode
    if (!ext2_ih_init(dir_handle->root_handle, &inode, dir_handle->inode)) {
        reply.size = 0;
        goto end;
    }
    
    // Buffer anpassen
    reply.data = realloc(reply.data, inode.data->size);
    
    // Daten einlesen
    reply.size = inode.data->size;

    if (ext2_symlink_read(dir_handle->root_handle, &inode, reply.size,
        reply.data) != 0)
    {
        puts("Fehler beim einlesen des Symlinks");
        reply.size = 0;
    }

    ext2_ih_destroy(dir_handle->root_handle, &inode);
end:
    v();
    return reply;
}

/**
 * Handler fuer Schreibzugriff auf Dateien
 */
size_t lostio_write_symlink_handler(lostio_filehandle_t* filehandle,
    size_t blocksize, size_t blockcount, void* data)
{
    /*p();
    size_t size = blocksize * blockcount;
    dir_handle_t* dir_handle = (dir_handle_t*) filehandle->node->data;

    // Zuerst brauchen wir mal den Inode
    ext2_inode_t inode;
    if (ext2_inode_read(dir_handle->root_handle, dir_handle->inode, &inode)
        != 0)
    {
        print_error("Fehler beim Lesen des Inodes!");
        v();
        return 0;
    }

    // Wenn's nichts zu schreiben gibt ist hier fertig
    if (size == 0) {
        v();
        return 0;
    }
    //printf("schreiben: %d %d  %d  %d\n", dir_handle->inode, size, inode.size, (int) filehandle->pos);
    // Daten schreiben
    size_t result = ext2_inode_data_write(dir_handle->root_handle, &inode,
        filehandle->pos, size, data);

    // Inode speichern
    if (ext2_inode_write(dir_handle->root_handle, dir_handle->inode, &inode)
        != 0)
    {
        print_error("Fehler beim Schreiben des Inodes!");
        v();
        return 0;
    }
    //ext2_sync(dir_handle->root_handle);
    
    // Groesse im Node anpassen
    filehandle->node->size = inode.size;

    // Cursor vorwaerts bewegen
    filehandle->pos += result;
    
    // Wenn noetig EOF-Flag setzen
    if(filehandle->pos >= inode.size) {
        filehandle->flags |= LOSTIO_FLAG_EOF;
    } else {
        filehandle->flags &= ~LOSTIO_FLAG_EOF;
    }
    
    v();
    return result;*/
    return 0;
}


