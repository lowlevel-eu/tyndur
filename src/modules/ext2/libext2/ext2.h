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
#ifndef _EXT2_H_
#define _EXT2_H_
#include <stdint.h>
#include <stdio.h>
#include "cache.h"

#ifdef __cplusplus
    extern "C" {
#endif



#define print_error(x) printf("%s  %d: %s\n", __FILE__, __LINE__, x)
#define print_error2(x, y) printf("%s  %d: %s  %d\n", __FILE__, __LINE__, x, y)
#define print_error2x(x, y) printf("%s  %d: %s  0x%x\n", __FILE__, __LINE__,\
    x, y)

// Offset des Superblocks vom Geraeteanfang
#define EXT2_SUPERBLOCK_OFFSET 1024

// Magic-Number fuer EXT2 (die einzige funktionierende Revision)
#define EXT2_MAGIC 0xEF53


// Erste Revision, wird sicher unterstuetzt
#define EXT2_REV_GOOD_OLD 0

// Zweite Revision. Hier haengt es von den Flags ab, ob wir sie unterstuetzen
// oder nicht
#define EXT2_REV_DYNAMIC 1


// Kompatible Features

/// Nur Lese-Kompatible
#define EXT2_FEATURE_ROCOMP_SPARSE_SUPER 0x1

// Inkompatible
/// Typ des Inodes im Verzeichniseintrag abspeichern
#define EXT2_FEATURE_INCOMP_FILETYPE 0x2


// Unterstuetzte Features
#define EXT2_FEATURES_SUP_ROCOMP (EXT2_FEATURE_ROCOMP_SPARSE_SUPER)
#define EXT2_FEATURES_SUP_INCOMP (EXT2_FEATURE_INCOMP_FILETYPE)

/// Haupstruktur des EXT2-Dateisystems
struct ext2_superblock {
    /// Anzahl der Inodes
    uint32_t inode_count;

    /// Anzahl der Bloecke
    uint32_t block_count;

    /// Anzahl der reservierten Bloecke
    uint32_t reserved_blocks;

    /// Anzahl der freien Bloecke
    uint32_t free_blocks;

    /// Anzahl der freien Inodes
    uint32_t free_inodes;

    /// Erster Datenblock
    uint32_t first_data_block;

    /// Blockgroesse (Achtung dieser Wert ist log2(Blockgroesse / 1024) und
    /// nicht die effektive blockgroesse)
    uint32_t block_size;

    /// Fragmentgroesse (???) Hier gilt das selbe wie oben
    int32_t fragment_size;
    
    /// Anzahl der Bloecke pro Gruppe
    uint32_t blocks_per_group;

    /// Anzahl der Fragmente pro Gruppe
    uint32_t fragments_per_group;

    /// Anzahl der Inodes pro Gruppe
    uint32_t inodes_per_group;

    /// Zeit an der das Dateisystem gemountet wurde
    uint32_t mount_time;

    /// Zeit an der auf das Dateisystem geschrieben wurde
    uint32_t write_time;

    /// Anzahl der Mounts
    uint16_t mount_count;

    /// Anzahl der maximalen Mounts
    int16_t max_mount_count;

    /// Magic zum Erkennen
    uint16_t magic;

    /// Status des Dateisystems
    uint16_t state;

    /// Behandlung von Fehlern
    uint16_t errors;
    uint16_t padding;

    /// Zeit des letzten Checks
    uint32_t last_check_time;

    /// Maximaler abstand zwischen Checks
    uint32_t check_interval;
    
    /// Betriebsystem das es erstellt hat
    uint32_t creator_os;

    /// Revision des Dateisystems
    uint32_t revision;

    /// Standard UID fuer reservierte Bloecke
    uint16_t default_res_uid;

    /// Standard GID fuer reservierte Bloecke
    uint16_t default_res_gid;
    
    
    // Diese Felder sind ab Revision 1 vorhanden
    /// Erster benutzbarer Inode
    uint32_t first_inode;

    /// Inode groesse
    uint16_t inode_size;
    
    /// Blockgruppennummer in der dieser Superblock liegt
    uint16_t blockgroup_num;

    /// Features mit denen die volle kompatibilitaet auch vorhanden ist, wenn
    /// sie nicht unterstutzt werden
    uint32_t features_compat;
    
    /// Features die dafuer sorgen, dass das Dateisytem werder gelesen noch
    /// beschrieben werden kann.
    uint32_t features_incompat;

    /// Features mit denen nur Lesen moeglich ist wenn sie nicht unterstuetzt
    /// werden
    uint32_t features_ro;

    /// Volume-Id
    uint32_t volume_id[4];

    /// Volume-Name (Nullterminiert(
    char volume_name[16];

    /// Pfad an den das Dateisystem zuletzt gemountet wurde. (Nullterminiert)
    char last_mounted[64];
    
    /// Wird bei komprimierten Datesystem benutzt, um den
    /// Kompressionsalorithmus festzustellen
    uint32_t algo_bitmap;

    // Reserviert
    uint32_t reserved[205];
} __attribute__((packed));

/// Gruppen deskriptor
typedef struct ext2_group_descriptor {
    /// Nummer des Blocks mit der Block-Allokationsbitmap
    uint32_t block_bitmap;

    /// Nummer des Blocks mit der Inode-Allokationsbitmap
    uint32_t inode_bitmap;

    /// Nummer des Blocks mit der Inode-Tabelle
    uint32_t inode_table;

    /// Anzahl der freien Blöcke
    uint16_t free_blocks;

    /// Anzahl der freien Inodes
    uint16_t free_inodes;

    /// Anzahl der Verzeichnisse
    uint16_t used_directories;
    uint16_t padding;

    /// Reserviert
    uint32_t reserved[3];
} __attribute__((packed)) ext2_group_desc_t;

/// Nummer des Inodes mit dem Root-Directory
#define EXT2_ROOTDIR_INODE 2

#define EXT2_DIR_BLOCKS 12
#define EXT2_INDIR_BLOCK EXT2_DIR_BLOCKS
#define EXT2_DINDIR_BLOCK (EXT2_INDIR_BLOCK + 1)
#define EXT2_TINDIR_BLOCK (EXT2_DINDIR_BLOCK + 1)

#define EXT2_MAX_NAME_LEN 255


// Flags fuer mode-Feld in inode

/// Maske Fuer alle Format-Optionen
#define EXT2_INODE_MODE_FORMAT 0xF000
#define EXT2_INODE_MODE_SYMLINK 0xA000
#define EXT2_INODE_MODE_REG_FILE 0x8000
#define EXT2_INODE_MODE_DIR 0x4000

// Makros zum Pruefen auf ein bestimmtes Format
#define EXT2_INODE_IS_FORMAT(i, f) (((i).mode & EXT2_INODE_MODE_FORMAT) == f)
#define EXT2_INODE_IS_SYMLINK(i) EXT2_INODE_IS_FORMAT(i, \
    EXT2_INODE_MODE_SYMLINK)
#define EXT2_INODE_IS_REG_FILE(i) EXT2_INODE_IS_FORMAT(i, \
    EXT2_INODE_MODE_REG_FILE)
#define EXT2_INODE_IS_DIR(i) EXT2_INODE_IS_FORMAT(i, \
    EXT2_INODE_MODE_DIR)

// Berechtigungen
#define EXT2_INODE_MODE_USER_MASK 0x1C0
#define EXT2_INODE_MODE_USER_READ 0x100
#define EXT2_INODE_MODE_USER_WRITE 0x80
#define EXT2_INODE_MODE_USER_EXEC 0x40

#define EXT2_INODE_MODE_GROUP_MASK 0x38
#define EXT2_INODE_MODE_GROUP_READ 0x20
#define EXT2_INODE_MODE_GROUP_WRITE 0x10
#define EXT2_INODE_MODE_GROUP_EXEC 0x08

#define EXT2_INODE_MODE_OTHER_MASK 0x7
#define EXT2_INODE_MODE_OTHER_READ 0x4
#define EXT2_INODE_MODE_OTHER_WRITE 0x2
#define EXT2_INODE_MODE_OTHER_EXEC 0x1

/// Inode
typedef struct ext2_inode_t {
    /// Dateimodus
    uint16_t mode;

    /// UID des Besitzers
    uint16_t uid;

    /// Groesse
    uint32_t size;

    /// Zeit des letzten Zugriffs
    uint32_t access_time;

    /// Erstellungszeit;
    uint32_t creation_time;

    /// Zeit der letzten Aenderungn
    uint32_t modification_time;

    /// Zeit der Loeschung
    uint32_t deletion_time;

    /// GID
    uint16_t gid;

    /// Anzahl der Links
    uint16_t link_count;

    /// Anzahl der Blocks.
    /// ACHTUNG: HIER HANDELT ES SICH UM 512-BYTE GROSSE BLOECKE!!!
    uint32_t block_count;

    /// Flags
    uint32_t flags;

    /// Reserviert
    uint32_t reserved1;

    /// Blocknummern
    uint32_t blocks[EXT2_TINDIR_BLOCK + 1];

    /// Version
    uint32_t version;

    /// TODO
    uint32_t file_acl;

    /// TODO
    uint32_t dir_acl;
    

    /// Fragment Adresse...
    uint32_t fragment_address;

    /// Fragment Nummer
    uint8_t fragment_number;

    /// Fragmentgroesse
    uint8_t fragment_size;

    uint16_t reserved2[5];
} __attribute__((packed)) ext2_inode_t;

// Dateitypen
#define EXT2_FT_UNKNOWN
#define EXT2_FT_REG_FILE 0x1
#define EXT2_FT_DIR 0x2
#define EXT2_FT_SYMLINK 0x7
/// Verzeichniseintrag wie er intern benutzt wird
typedef struct ext2_raw_dir_entry_t {
    /// Inode-Nummer
    uint32_t inode;

    /// Laenge des aktuellen Eintrags
    uint16_t record_len;

    /// Laenge des Namens
    uint8_t name_len;

    /// Dateityp
    uint8_t type;

    /// Name (_NICHT_ nullterminiert)
    char name[];
} __attribute__((packed)) ext2_raw_dir_entry_t;

/// Verzeichniseintrag wie er nach aussen benutzt wird
typedef struct ext2_dir_entry_t {
    /// Inode-Numme
    uint32_t inode;
    
    /// Typ des Eintrags
    enum {REG_FILE, DIRECTORY, SYMLINK} type;

    /// Name (Nullterminiert)
    char name[EXT2_MAX_NAME_LEN + 1];
} ext2_dir_entry_t;


/// Root-Handle
typedef struct ext2_root_handle_t {
    /// Superblock
    struct ext2_superblock superblock;

    /// Blocknummer aus der der Superblock gelesen wurde
    uint32_t superblock_block;

    /// Dirty-Flag fuer Superblock
    int superblock_dirty;
    
    /// Liste mit Gruppendeskriptoren
    struct ext2_group_descriptor* group_descs;

    /// Anzahl der Gruppendeskriptoren
    size_t group_count;

    /// Dirty-Flags fuer die Gruppendeskriptoren
    int group_descs_dirty;

    /// Dateisystem darf nur gelesen werden (1)
    int read_only;

    /// Block-Cache
    block_cache_t* block_cache;

    /// Inode-Cache
    block_cache_t* inode_cache;


    
    /// Funktion zum Lesen vom Quellmedium
    size_t (*dev_read)(struct ext2_root_handle_t* rh, uint64_t start,
        size_t size, void* buffer);

    /// Funktion zum Schreiben auf das Quellmedium
    size_t (*dev_write)(struct ext2_root_handle_t* rh, uint64_t start,
        size_t size, const void* buffer);
    

    /// Feld das frei benutzt werden kann
    void* opaque;
} ext2_root_handle_t;


// Handle fuer ein Inode im Speicher
typedef struct ext2_ih_t {
    uint32_t number;
    ext2_inode_t* data;
    int modified;
} ext2_ih_t;


/// ext2-Dateisystem mounten
int ext2_mount(ext2_root_handle_t* root_handle);

/// Saemtliche noch im Speicher liegende Aenderungen schreiben
int ext2_sync(ext2_root_handle_t* root_handle);

/// ext2-Superblock einlesen
int ext2_superblock_read(ext2_root_handle_t* root_handle);

/// ext2-Superblock schreiben
int ext2_superblock_write(ext2_root_handle_t* root_handle);


/// Pruefen ob das angegebene kompatible Feature unterstuetzt wird
int ext2_feature_comp(ext2_root_handle_t* root_handle, uint32_t feature);

/// Pruefen ob das angegebene nur lese-kompatible Feature unterstuetzt wird
int ext2_feature_rocomp(ext2_root_handle_t* root_handle, uint32_t feature);

/// Pruefen ob das angegebene inkompatible Feature unterstuetzt wird
int ext2_feature_incomp(ext2_root_handle_t* root_handle, uint32_t feature);



/// Blockcache und co vorbereiten
int ext2_block_init(ext2_root_handle_t* root_handle);

/// Blockcache synchronisieren
int ext2_block_sync(ext2_root_handle_t* root_handle);

/// Groesse eines Blocks
int ext2_block_size(ext2_root_handle_t* root_handle);

/// Unbenutzten Block allozieren
uint32_t ext2_block_allocate(ext2_root_handle_t* root_handle, uint32_t prev);

/// Block freigeben
int ext2_block_free(ext2_root_handle_t* root_handle, uint32_t block);

/// Anzahl der Bloecke die Benoetigt werden, um eine bestimmte Anzahl bytes zu
/// laden
uint32_t ext2_block_count(ext2_root_handle_t* root_handle, uint32_t size);

/// ext2-Block lesen
int ext2_block_read(ext2_root_handle_t* root_handle, uint32_t block,
    uint32_t count, void* buffer);

/// ext3-Block schreiben
int ext2_block_write(ext2_root_handle_t* root_handle, uint32_t block,
    uint32_t count, const void* buffer);



/// Gruppen laden
int ext2_groups_init(ext2_root_handle_t* root_handle);

/// Gruppen speichern
int ext2_groups_write(ext2_root_handle_t* root_handle);



/// Inode-Cache vorbereiten
int ext2_inode_init(ext2_root_handle_t* root_handle);

/// Blockcache synchronisieren
int ext2_inode_sync(ext2_root_handle_t* root_handle);

/// Inode aus dem Cache holen
int ext2_inode_cache_get(ext2_root_handle_t* root_handle, ext2_ih_t* inode);

// Inode zurueck in Cache legen
int ext2_inode_cache_put(ext2_root_handle_t* root_handle, ext2_ih_t* inode);

/// Inode in den Speicher laden
int ext2_inode_read(ext2_root_handle_t* root_handle, ext2_ih_t* handle);

/// Inode schreiben
int ext2_inode_write(ext2_root_handle_t* root_handle, ext2_ih_t* handle);

/// Freien Inode suchen
uint32_t ext2_inode_allocate(ext2_root_handle_t* root_handle);

/// Inode freigeben
int ext2_inode_free(ext2_root_handle_t* root_handle, uint32_t inode_num);

/// Nutzdaten von aus einem Inode lesen
size_t ext2_inode_data_read(ext2_root_handle_t* root_handle, ext2_ih_t* ih,
    uint64_t start, size_t size, void* buffer);

/// Nutzdate in ein Inode schreiben
size_t ext2_inode_data_write(ext2_root_handle_t* root_handle, ext2_ih_t* ih,
    uint64_t start, size_t size, const void* buffer);

/// Groesse der Nutzdaten in einem Inode anpassen
int ext2_inode_data_truncate(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    uint64_t size);


/// Verzeichniseintraege auf 4 Bytes padden
#define EXT2_DIR_ENTRY_PAD 4

/// Verzeichniseintraege verabeiten
int ext2_dir_foreach_entry(ext2_root_handle_t* root_handle, ext2_ih_t* ih,
    int (*handler) (void*, ext2_dir_entry_t*), void* param);

/// Verzeichniseintrag suchen
uint32_t ext2_dir_entry_get(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name);

/// Verzeichniseintrag anlegen
int ext2_dir_link(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    ext2_ih_t* target, const char* name);

/// Verzeichniseintrag loeschen
int ext2_dir_unlink(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name);

/// Neues Verzeichnis erstellen
int ext2_dir_create(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name, uint32_t* inode_dest);

/// Verzeichnis loeschen
int ext2_dir_delete(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name);


/// Neue Datei erstellen
int ext2_file_create(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name, uint32_t* inode_dest);

/// Datei Loeschen
int ext2_file_delete(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name);


/// Neuen Symlink erstellen
int ext2_symlink_create(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name, uint32_t* inode_dest);

/// Symlink loeschen
int ext2_symlink_delete(ext2_root_handle_t* root_handle, ext2_ih_t* parent,
    const char* name);

/// Pfad aus einem Symlink auslesen
int ext2_symlink_read(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    size_t size, void* buffer);

/// Pfad eines Symlinks ueberschreiben
int ext2_symlink_write(ext2_root_handle_t* root_handle, ext2_ih_t* inode,
    size_t size, void* buffer);




/**
 * Inode-Handle initialisieren
 *
 * @return 1 bei Erfolg, sonst 0
 */
static inline int ext2_ih_init(ext2_root_handle_t* rh, ext2_ih_t* ih,
    uint32_t number)
{
    ih->modified = 0;
    ih->number = number;
    return ext2_inode_cache_get(rh, ih);
}

/**
 * IH deinitialisieren; Wenn modified != 0 ist, werden die Daten geschrieben.
 *
 * @return 1 bei Erfolg, sonst 0
 */
static inline int ext2_ih_destroy(ext2_root_handle_t* rh, ext2_ih_t* ih)
{
    return ext2_inode_cache_put(rh, ih);
}



#endif
