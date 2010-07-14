/*****************************************************************************
* Copyright (c) 2009-2010 Max Reitz                                          *
*                                                                            *
* THE COKE-WARE LICENSE                                                      *
*                                                                            *
* Redistribution  and use  in  source  and  binary  forms,  with or  without *
* modification,  are permitted  provided that the  following  conditions are *
* met:                                                                       *
*                                                                            *
*   1. Redistributions  of  source code  must  retain  the  above  copyright *
*      notice, this list of conditions and the following disclaimer.         *
*   2. Redistribution in  binary form  must reproduce  the  above  copyright *
*      notice  and either  the full  list  of  conditions and  the following *
*      disclaimer or a link to both.                                         *
*   3. If we meet one day and you think this stuff is worth it,  you may buy *
*      me a coke in return.                                                  *
*                                                                            *
* THIS SOFTWARE  IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND ANY *
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO, THE IMPLIED *
* WARRANTIES  OF MERCHANTABILITY  AND FITNESS  FOR A PARTICULAR PURPOSE  ARE *
* DISCLAIMED.  IN  NO  EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL *
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
* CAUSED  AND  ON  ANY  THEORY OF  LIABILITY,  WHETHER  IN CONTRACT,  STRICT *
* LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY *
* OUT OF THE USE  OF THIS  SOFTWARE,  EVEN IF ADVISED OF THE  POSSIBILITY OF *
* SUCH DAMAGE.                                                               *
*****************************************************************************/

#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>
#include <arpa/inet.h>

// Funktionen zum Konvertieren von/zu Big Endian
#define htobe16(val) htons(val)
#define htobe32(val) htonl(val)
#define betoh16(val) htobe16(val)
#define betoh32(val) htobe32(val)

// SCSI-Befehlscodes
#define SCSI_REQUEST_SENSE 0x03
#define SCSI_READ6         0x08
#define SCSI_INQUIRY       0x12
#define SCSI_READ_CAPACITY 0x25
#define SCSI_READ10        0x28
#define SCSI_WRITE10       0x2A
#define SCSI_READ12        0xA8

// Minimaler SCSI-Befehl
struct scsi_cmd {
    // Befehlscode
    uint8_t operation;
} __attribute__((packed));

// REQUEST SENSE
// Gibt Informationen im Fehlerfall zurück.
struct scsi_cmd_request_sense {
    struct scsi_cmd cmd;
    unsigned rsvd1 : 5;
    // LUN
    unsigned lun   : 3;
    uint16_t rsvd2;
    // Anzahl der Bytes, die der Host zu empfangen bereit ist
    uint8_t alloc_len;
    uint8_t control;
} __attribute__((packed));

// INQUIRY
// Gibt Informationen zum Gerät zurück.
struct scsi_cmd_inquiry {
    struct scsi_cmd cmd;
    unsigned evpd : 1;
    unsigned rsvd : 4;
    // LUN
    unsigned lun  : 3;
    uint8_t page;
    // Anzahl der Bytes, die der Host zu empfangen bereit ist
    uint16_t alloc_len;
    uint8_t control;
} __attribute__((packed));

// READ CAPACITY
// Gibt die LBA des letzten Blocks zurück sowie die Blockgröße
struct scsi_cmd_read_capacity {
    struct scsi_cmd cmd;
    unsigned reladdr : 1;
    unsigned rsvd1   : 4;
    // LUN
    unsigned lun     : 3;
    uint32_t lba;
    uint32_t rsvd2;
} __attribute__((packed));

// READ6
// Liest Daten
struct scsi_cmd_read6 {
    struct scsi_cmd cmd;
    // Höherwertiger Teil der LBA
    unsigned lba_hi : 5;
    // LUN
    unsigned lun : 3;
    // Niederwertiger Teil der LBA
    uint16_t lba_lo;
    // Anzahl zu lesende Blöcke
    uint8_t length;
    uint8_t control;
} __attribute__((packed));

// READ10
// Liest Daten
struct scsi_cmd_read10 {
    struct scsi_cmd cmd;
    unsigned reladdr : 1;
    unsigned rsvd1   : 2;
    unsigned fua     : 1;
    unsigned dpo     : 1;
    // LUN
    unsigned lun     : 3;
    // LBA
    uint32_t lba;
    uint8_t rsvd2;
    // Anzahl zu lesende Blöcke
    uint16_t length;
    uint8_t control;
} __attribute__((packed));

// READ12
// Liest Daten
struct scsi_cmd_read12 {
    struct scsi_cmd cmd;
    unsigned reladdr : 1;
    unsigned rsvd1   : 2;
    unsigned fua     : 1;
    unsigned dpo     : 1;
    // LUN
    unsigned lun     : 3;
    // LBA
    uint32_t lba;
    // Anzahl zu lesende Blöcke
    uint32_t length;
    uint16_t rsvd2;
} __attribute__((packed));

// WRITE10
// Schreibt Daten
struct scsi_cmd_write10 {
    struct scsi_cmd cmd;
    unsigned reladdr : 1;
    unsigned rsvd1   : 1;
    unsigned ebp     : 1;
    unsigned fua     : 1;
    unsigned dpo     : 1;
    // LUN
    unsigned lun     : 3;
    // LBA
    uint32_t lba;
    uint8_t rsvd2;
    // Anzahl zu lesende Blöcke
    uint16_t length;
    uint8_t control;
} __attribute__((packed));

// Rückgabestruktur von SCSI SENSE
struct scsi_sense {
    unsigned resp_code : 7;
    unsigned valid     : 1;
    uint8_t segment;
    unsigned sense_key : 4;
    unsigned rsvd1     : 1;
    unsigned ili       : 1;
    unsigned eom       : 1;
    unsigned filemark  : 1;
    uint32_t information;
    uint8_t add_sense_length;
    uint32_t cmd_spec_info;
    uint8_t add_sense_code;
    uint8_t add_sense_code_qual;
    uint8_t field_replacable_unit_code;
    unsigned sense_key_spec1 : 7;
    unsigned sksv            : 1;
    uint8_t sense_key_spec2;
    uint8_t sense_key_spec3;
} __attribute__((packed));

// Rückgabestruktur von SCSI INQUIRY
struct scsi_inq_answer {
    // Gerätetype
    unsigned dev_type  : 5;
    unsigned qualifier : 3;
    unsigned type_mod : 7;
    unsigned rmb      : 1;
    unsigned ansi_version : 3;
    unsigned ecma_version : 3;
    unsigned iso_version  : 2;
    unsigned resp_data_format : 4;
    unsigned rsvd1            : 2;
    unsigned trm_iop          : 1;
    unsigned aec              : 1;
    uint8_t add_length;
    uint16_t rsvd2;
    unsigned soft_rst  : 1;
    unsigned cmd_queue : 1;
    unsigned rsvd3     : 1;
    unsigned linked    : 1;
    unsigned sync      : 1;
    unsigned wbus16    : 1;
    unsigned wbus32    : 1;
    unsigned rel_adr   : 1;
    // Herstellername
    char vendor[8];
    // Produktname
    char product[16];
    uint32_t revision;
} __attribute__((packed));

#endif
