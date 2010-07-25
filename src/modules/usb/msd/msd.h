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

#ifndef MSD_H
#define MSD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "usb-ddrv.h"

// Beschreibt eine LUN eines MSC-Geräts (MSD) im Allgemeinen
struct msc_gen_dev {
    // LUN
    int lun;

    // Größe eines Blocks
    uint64_t block_size;
    // Anzahl Blöcke
    uint64_t blocks;

    /**
     * Führt einen SCSI-Befehl aus.
     *
     * @param dev Zielgerät
     * @param cmd_buffer Speicherbereich, der den Befehl enthält
     * @param cmd_length Länge dieses Befehls in Byte
     * @param receive true, wenn empfangen werden soll
     * @param req_length Länge der zu übertragenden Daten
     *
     * @return 0 bei Erfolg, sonst -errno
     */
    int (*issue_scsi_cmd)(struct msc_gen_dev* dev, const void* cmd_buffer,
        size_t cmd_length, bool receive, size_t req_length);

    /**
     * Sendet Daten zu einem Gerät.
     *
     * @param dev Zielgerät
     * @param buffer Speicherbereich, der die Daten enthält
     * @param size Anzahl der zu sendenden Bytes
     *
     * @return 0 bei Erfolg, sonst -errno
     */
    int (*send)(struct msc_gen_dev* dev, const void* buffer, size_t size);

    /**
     * Empfängt Daten von einem Gerät.
     *
     * @param dev Quellgerät
     * @param buffer Speicherbereich, in den die Daten gelesen werden sollen
     * @param size Anzahl der zu empfangenden Bytes
     *
     * @return 0 bei Erfolg, sonst -errno
     */
    int (*recv)(struct msc_gen_dev* dev, void* buffer, size_t size);
};

/**
 * Wird von einem RPC-Handler aufgerufen, um zu prüfen, ob das neue Gerät ein
 * MSD ist, mit dem der Treiber etwas anfangen kann.
 *
 * @param dev Neues Gerät
 *
 * @return 1 wenn das Gerät benutzt werden kann, sonst 0
 */
int is_valid_msd(struct usb_prov_dev* dev);

/**
 * Wird von einem RPC-Handler aufgerufen, sobald ein neues MSD gefunden wurde.
 *
 * @param dev Neues Gerät
 */
void got_msd(struct usb_prov_dev* dev);

/**
 * Initialisiert das LostIO-Interface.
 */
void init_lostio_interface(void);

/**
 * Initialisiert das RPC-Interface.
 */
void init_rpc_interface(void);

/**
 * Initialisiert ein MSD auf SCSI-Ebene.
 *
 * @param dev Das MSD
 * @param blocks Pointer zu einer Variable, in die die Anzahl der vorhandenen
 *               Blöcke geschrieben wird
 * @param bsize Pointer zu einer Variable, in die die Größe eines Blocks
 *              geschrieben wird
 *
 * @return 1 bei Erfolg, sonst 0
 */
int msd_scsi_init(struct msc_gen_dev* dev, uint64_t* blocks,
    uint64_t* bsize);

/**
 * Liest von einem MSD per SCSI.
 *
 * @param dev Das MSD
 * @param start_lba LBA des ersten zu lesenden Blocks
 * @param dest Zielspeicherbereich
 * @param blocks Anzahl der zu lesenden Blöcke
 *
 * @return Anzahl der gelesenen Bytes
 */
size_t msd_scsi_read(struct msc_gen_dev* dev, uint64_t start_lba, void* dest,
    uint64_t blocks);

/**
 * Wird von einer MSD-spezifischen Initialisierungsroutine aufgerufen, um ein
 * für die SCSI-Initialisierung vorbereitetes MSD bereitzustellen.
 *
 * @param dev Das neue Gerät
 */
void provide_msd(struct msc_gen_dev* dev);

/**
 * Registriert diesen Treiber beim USB-Bustreiber.
 */
void register_msddrv(void);

#endif
