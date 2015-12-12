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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "kernel.h"
#include "kprintf.h"

#include "lostio/core.h"
#include "lostio/client.h"
#include "lostio_int.h"

static list_t* lio_trees;
static list_t* lio_services;

/**
 * Initialisiert die statischen Variablen dieser Datei
 */
int lio_init_tree(void)
{
    lio_trees = list_create();
    lio_services = list_create();

    return 0;
}

/**
 * Erzeugt einen neuen Verzeichnisbaum. Das root-Feld im der Baum-Struktur muss
 * vom Aufrufer eingefuellt werden.
 *
 * @param serv LIO-Service, der den Baum verwaltet
 * @param source LIO-Stream der Datenquelle
 *
 * @return Den neu angelegten Baum oder NULL im Fehlerfall
 */
static struct lio_tree* lio_add_tree(struct lio_service* serv,
    struct lio_stream* source)
{
    struct lio_tree* tree;

    // TODO Der Service sollte auch wirklich fuer den Pfad zustaendig sein

    // Neuen Baum anlegen und in die Liste pushen
    tree = calloc(1, sizeof(*tree));
    tree->service = serv;
    tree->source = source;

    lio_usp_add_tree(tree);

    return tree;
}

/**
 * Zerlegt einen Pfad. Die zwei uebergebenen Zeiger zeigen anschliessend auf
 * den Namen des zustaendigen Services bzw. den Anfang des servicerelativen
 * Pfads im Originalpfad.
 *
 * Im Fehlerfall (der Pfad enthaelt kein ":/") wird rel_path auf NULL gesetzt.
 */
static void split_path(const char* path, const char** service,
    const char** rel_path)
{
    const char* service_name;
    const char* tmp_rel_path;

    service_name = strrchr(path, '|');
    if (service_name == NULL) {
        service_name = path;
    } else {
        service_name++;
    }

    tmp_rel_path = strstr(service_name, ":/");
    if (tmp_rel_path != NULL) {
        tmp_rel_path += 2;
    }

    *service = service_name;
    *rel_path = tmp_rel_path;
}

/**
 * Sucht den richtigen Service zur Verarbeitung eines Pfads und erzeugt einen
 * neuen Verzeichnisbaum fuer diesen.
 *
 * @param path Pfad der zu oeffnenden Datei
 * @param rel_path Wenn rel_path != NULL ist, wird darin ein Pointer auf den
 * servicerelativen Teil des Pfads zurueckgegeben.
 * @param depth Symlink-Verschachtelungstiefe
 *
 * @return Den neu angelegten Verzeichnisbaum oder NULL im Fehlerfall
 */
static struct lio_tree* mount(const char* path, const char** rel_path,
                              int depth)
{
    struct lio_stream* source_stream;
    struct lio_resource* source_res;
    struct lio_service* service;
    struct lio_resource* root_res;
    struct lio_node* root_node;
    struct lio_tree* tree;
    const char* service_name;
    const char* tmp_rel_path;
    int i;

    // Pfad aufsplitten
    split_path(path, &service_name, &tmp_rel_path);

    // Passenden Service suchen
    for (i = 0; (service = list_get_element_at(lio_services, i)); i++) {

        size_t len = strlen(service->name);

        if (len != tmp_rel_path - 2 - service_name) {
            continue;
        }

        if (!strncmp(service->name, service_name, len)) {
            goto found;
        }
    }
    return NULL;

found:

    // Quelle oeffnen
    source_stream = NULL;
    if (path == service_name) {
        source_stream = NULL;
    } else {
        char* source;
        source = malloc(service_name - path);
        strncpy(source, path, service_name - path - 1);
        source[service_name - path - 1] = '\0';

        source_res = lio_do_get_resource(source, 1, depth);
        free(source);

        if (source_res == NULL) {
            return NULL;
        }

        // TODO: Was machen wir hier mit den Flags fuer fopen genau?
        source_stream = lio_open(source_res, LIO_READ | LIO_WRITE);
        if (source_stream == NULL) {
            source_stream = lio_open(source_res, LIO_READ);
        }
        if (source_stream == NULL) {
            return NULL;
        }
    }

    tree = lio_add_tree(service, source_stream);

    // Wurzelressource laden
    root_res = service->lio_ops.load_root(tree);
    if (root_res == NULL) {
        goto out_err;
    }

    // Wurzelknoten fuer neuen Baum anlegen
    root_node = calloc(1, sizeof(*root_node));
    root_node->res = root_res;
    root_node->name = malloc(tmp_rel_path - path + 1);
    strncpy(root_node->name, path, tmp_rel_path - path);
    root_node->name[tmp_rel_path - path] = '\0';

    tree->root = root_node;
    root_node->res->tree = tree;

    // Neuen Baum registrieren
    list_push(lio_trees, tree);
    if (rel_path) {
        *rel_path = tmp_rel_path;
    }

    return tree;

out_err:
    if (source_stream) {
        lio_close(source_stream);
    }
    lio_usp_remove_tree(tree);
    free(tree);
    return NULL;
}

/**
 * Loest einen gegebenen Pfad in einen Verzeichnisbaum und einen
 * servicerelativen Pfad auf.
 *
 * @param path Aufzulosender Pfad
 * @param rel_path Wenn rel_path != NULL ist, wird darin ein Pointer auf den
 * servicerelativen Teil des Pfads zurueckgegeben
 * @param depth Symlink-Verschachtelungstiefe
 *
 * @return Den zum Pfad gehoerenden Verzeichnisbaum oder NULL im Fehlerfall
 */
struct lio_tree* lio_get_tree(const char* path, const char** rel_path,
                              int depth)
{
    int i;
    struct lio_tree* tree;
    const char* service_name;
    const char* tmp_rel_path;

    // Pfad fuer den Baum heraussucen
    split_path(path, &service_name, &tmp_rel_path);
    if (rel_path) {
        *rel_path = tmp_rel_path;
    }

    // Suchen, ob es schon einen Baum fuer den Pfad gibt
    for (i = 0; (tree = list_get_element_at(lio_trees, i)); i++) {
        if (!tree->root) {
            panic("BUG: lio_tree ohne Wurzel");
        }

        if (strlen(tree->root->name) != (tmp_rel_path - path)) {
            continue;
        }

        if (!strncmp(tree->root->name, path, tmp_rel_path - path)) {
            return tree;
        }
    }

    // Neuen Baum anlegen
    tree = mount(path, rel_path, depth);

    return tree;
}

/**
 * Registriert einen neuen LIO-Service
 */
void lio_add_service(struct lio_service* service)
{
    list_push(lio_services, service);
}

/**
 * Service finden, der die Ressource als Pipequelle akzeptiert
 */
int lio_probe_service(struct lio_resource* res,
                      struct lio_probe_service_result* probe_data)
{
    struct lio_service* service;
    struct lio_stream *stream;
    int ret;
    int i;

    stream = lio_open(res, LIO_READ);
    if (stream == NULL) {
        return -EIO;
    }

    for (i = 0; (service = list_get_element_at(lio_services, i)); i++) {
        if (!service->lio_ops.probe) {
            continue;
        }

        ret = service->lio_ops.probe(service, stream, probe_data);
        if (ret == 0) {
            goto out;
        }
    }

    ret = -ENOENT;
out:
    lio_close(stream);
    return ret;
}

/**
 * Neue Ressource initialisieren.
 * Vom Aufrufer initialisiert werden muessen (werden alle auf 0 initialisiert):
 *   - blocksize
 *   - tree
 *   - Flags
 *
 * @return Zeiger auf die neue Ressource
 */
struct lio_resource* lio_create_resource(void)
{
    struct lio_resource* res = calloc(sizeof(*res), 1);

    lio_init_resource(res);

    return res;
}

/**
 * Interne Daten einer Ressource initialisieren. Dabei werden nur die internen
 * Felder veraendert, der Rest muss vom Aufrufer gesetzt werden. Diese Funktion
 * kann verwendet werden, wenn der Speicher fuer eine Resource bereits
 * verfuegbar ist (beispielsweise bei statischem Speicher).
 *
 * @param res Zu initialisierende Ressource
 */
void lio_init_resource(struct lio_resource* res)
{
    lio_usp_add_resource(res);
}

/**
 * Ressource zerstoeren und Speicher freigeben. Der Aufrufer ist dafuer
 * allenfalls Caches zu syncen.
 *
 * @param res Zu zerstoerende Ressource
 */
void lio_destroy_resource(struct lio_resource* res)
{
    lio_usp_remove_resource(res);
    free(res);
}

/** Kindknoten einer Ressource suchen */
struct lio_node* lio_resource_get_child(struct lio_resource* parent,
    const char* name)
{
    struct lio_node* n;
    size_t i;

    for (i = 0; (n = list_get_element_at(parent->children, i)); i++) {
        if (!strcmp(name, n->name)) {
            return n;
        }
    }

    return NULL;
}

/** Einer Resource einen Kindknoten anfuegen */
void lio_resource_add_child(struct lio_resource* parent,
    struct lio_node* child)
{
    if (!parent->children) {
        parent->children = list_create();
    }

    list_push(parent->children, child);
}

/** Kindknoten einer Ressource entfernen */
void lio_resource_remove_child(struct lio_resource* parent,
    struct lio_node* child)
{
    struct lio_node* n;
    size_t i;

    // FIXME: Hier brauchen wir irgendwann noch gescheite Locks
    for (i = 0; (n = list_get_element_at(parent->children, i)); i++) {
        if (n == child) {
            list_remove(parent->children, i);
            break;
        }
    }
}

/** Neuen Knoten anlegen und bei Elternressource als Kind eintragen */
struct lio_node* lio_create_node(struct lio_resource* parent,
    struct lio_resource* res, const char* name)
{
    struct lio_node* n = calloc(sizeof(*n), 1);

    n->res = res;
    n->name = strdup(name);
    lio_resource_add_child(parent, n);

    return n;
}

/** Größe aktualisieren und on_size-Notifier ausführen */
void lio_resource_update_size(struct lio_resource* res, uint64_t size)
{
    res->size = size;
    notify(res->on_resize);
}
