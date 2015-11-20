/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <tms.h>
#include "resstr.h"

static int get_number(int n)
{
    return (n == 1 ? 0 : 1);
}

static const struct tms_strings dict[] = {

    // tyndur-Konfiguration
    &RESSTR_P$SETUP_RSTITLE,
    "tyndur Configuration",


    // Bitte wähle aus dem Menü das gewünschte...
    &RESSTR_MENU_RSCHOOSEMODULE,
    "Please choose a configuration program from the menu.",

    // Tastaturbelegung
    &RESSTR_MENU_RSMODKEYBOARD,
    "Keyboard layout",

    // lpt-Quellen
    &RESSTR_MENU_RSMODLPT,
    "lpt repositories",

    // Netzwerk
    &RESSTR_MENU_RSMODNETWORK,
    "Network",

    // Beenden
    &RESSTR_MENU_RSQUIT,
    "Quit",


    // tyndur-Konfiguration: Tastaturbelegung
    &RESSTR_SETUP_KEYBOARD_RSTITLE,
    "tyndur Configuration: Keyboard layout",

    // Deutsch
    &RESSTR_SETUP_KEYBOARD_RSGERMAN,
    "German (de)",

    // Schweizerdeutsch
    &RESSTR_SETUP_KEYBOARD_RSSWISSGERMAN,
    "Swiss German (de_ch)",

    // US-Amerikanisch
    &RESSTR_SETUP_KEYBOARD_RSUSAMERICAN,
    "US American (us)",

    // Zurück
    &RESSTR_SETUP_KEYBOARD_RSBACK,
    "Back",

    // Bitte wähle aus dem Menü...
    &RESSTR_SETUP_KEYBOARD_RSCHOOSELAYOUT,
    "Please choose your keyboard layout from the list below:",

    // Konnte Tastaturbelegung nicht einlesen
    &RESSTR_SETUP_KEYBOARD_RSFAILREADKEYMAP,
    "Could not read keyboard layout file",

    // Konnte Tastaturbelegung nicht anwenden
    &RESSTR_SETUP_KEYBOARD_RSFAILAPPLYKEYMAP,
    "Could not apply keyboard layout",


    // tyndur-Konfiguration: Quelle für lpt
    &RESSTR_SETUP_LPT_RSTITLE,
    "tyndur Configuration: lpt repositories",

    // Bitte eine beliebige Taste drücken
    &RESSTR_SETUP_LPT_RSANYKEY,
    "Press any key to continue",

    // Bitte wähle aus dem Menü die Quelle
    &RESSTR_SETUP_LPT_RSCHOOSE,
    "Please choose your software repository from the list below:",

    // Stabile Version (gothmog)
    &RESSTR_SETUP_LPT_RSSTABLE,
    "Stable version (gothmog)",

    // Entwicklerversion (current)
    &RESSTR_SETUP_LPT_RSCURRENT,
    "Developer versions (current)",

    // Zurück
    &RESSTR_SETUP_LPT_RSBACK,
    "Back",


    //tyndur-Konfiguration: Netzwerk
    &RESSTR_SETUP_NETWORK_RSTITLE,
    "tyndur Configuration: Network",

    // Bitte in folgende Felder die Netzwerkeinstellungen eintragen:
    &RESSTR_SETUP_NETWORK_RSCONFIGURENETWORK,
    "Please enter your network configuration below:",

    // Treiber
    &RESSTR_SETUP_NETWORK_RSDRIVER,
    "Driver",

    // Über DHCP konfigurieren
    &RESSTR_SETUP_NETWORK_RSDHCP,
    "Configure with DHCP",

    // Statische Konfiguration
    &RESSTR_SETUP_NETWORK_RSSTATIC,
    "Static configuration",

    // IP-Adresse
    &RESSTR_SETUP_NETWORK_RSIPADDRESS,
    "IP address",

    // Gateway
    &RESSTR_SETUP_NETWORK_RSGATEWAY,
    "Gateway",

    // OK
    &RESSTR_SETUP_NETWORK_RSOK,
    "OK",

    // Abbrechen
    &RESSTR_SETUP_NETWORK_RSCANCEL,
    "Cancel",

    0,
    0,
};

static const struct tms_lang lang = {
    .lang = "en",
    .numbers = 2,
    .get_number = get_number,
    .strings = dict,
};

LANGUAGE(&lang)
