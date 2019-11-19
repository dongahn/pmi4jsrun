/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pmi.h"

int main (int argc, char *argv[])
{
    int rc = 0, grc = 0, rank = 0, size = 0, spawned = 0;
    int name_len = 0, key_len = 0, val_len = 0;
    char *kvsname = NULL, *key1 = NULL, *key2 = NULL, *val1 = NULL, *val2 = NULL;

    if ( (rc = PMI_Init (&spawned)) != PMI_SUCCESS) {
        fprintf (stderr, "PMI_Init:\n"); grc++;
    }
    if ( (rc = PMI_Get_size (&size)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_Get_size: \n", rank); grc++;
    }
    if ( (rc = PMI_Get_rank (&rank)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_Get_rank:\n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Get_name_length_max (&name_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get_name_length_max: \n", rank); grc++;
    }
    if ( (kvsname = (char *)malloc (name_len)) == NULL) {
        fprintf (stderr, "%d: [error] OOM: \n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Get_my_name (kvsname, name_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get_my_name: \n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Get_key_length_max (&key_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get_key_length_max: \n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Get_value_length_max (&val_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get_value_length_max: \n", rank); grc++;
    }
    if ( (key1 = (char *)malloc (key_len)) == NULL) {
        fprintf (stderr, "%d: [error] OOM: \n", rank); grc++;
    }
    if ( (key2 = (char *)malloc (key_len)) == NULL) {
        fprintf (stderr, "%d: [error] OOM: \n", rank); grc++;
    }
    if ( (val1 = (char *)malloc (val_len)) == NULL) {
        fprintf (stderr, "%d: [error] OOM: \n", rank); grc++;
    }
    if ( (val2 = (char *)malloc (val_len)) == NULL) {
        fprintf (stderr, "%d: [error] OOM: \n", rank); grc++;
    }
    if ( (rc = snprintf (key1, key_len, "key-from-%d", rank)) < 0) {
        fprintf (stderr, "%d: [error] sprintf: %s", rank, strerror (errno)); grc++;
    }
    if ( (rc = snprintf (val1, val_len, "val-from-%d", rank)) < 0) {
        fprintf (stderr, "%d: [error] sprintf: %s", rank, strerror (errno)); grc++;
    }
    if ( (rc = snprintf (key2, key_len, "key-from-%d", (size + rank -1) % size)) < 0) {
        fprintf (stderr, "%d: [error] sprintf: %s", rank, strerror (errno)); grc++;
    }
    if ( (rc = PMI_KVS_Put (kvsname, key1, val1)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Put: \n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Commit (kvsname)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Commit: \n", rank); grc++;
    }
    if ( (rc = PMI_Barrier ()) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_Barrier: \n", rank); grc++;
    }
    if ( (rc = PMI_KVS_Get (kvsname, key1, val1, val_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get: key(%s)=val(%s) rc (%d)\n", rank, key2, val2, rc); grc++;
    }
    if ( (rc = PMI_KVS_Get (kvsname, key2, val2, val_len)) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_KVS_Get: key(%s)=val(%s) rc (%d)\n", rank, key2, val2, rc); grc++;
    }
    if ( (rc = PMI_Finalize ()) != PMI_SUCCESS) {
        fprintf (stderr, "%d: [error] PMI_Finalize: \n", rank);
        grc++;
    }

    free (kvsname);
    free (key1);
    free (key2);
    free (val1);
    free (val2);

    if (grc != 0) {
        fprintf (stdout, "%d: FAILED\n", rank);
    } else {
        fprintf (stdout, "%d: SUCCESS\n", rank);
    }

    return 0;
}
