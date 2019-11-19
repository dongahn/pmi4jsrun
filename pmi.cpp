/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* Implement subset of PMI functionality on top of MPI calls */

#include <mpi.h>

#include <map>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmi.h"
#include "map_wrap.hpp"
#include "reduce.hpp"

using namespace std;

static int initialized = 0;
static int ranks = -1;
static int my_rank = -1;
static int id = -1;
static bool debug = false;

#define MAX_KVS_LEN (256)
#define MAX_KEY_LEN (256)
#define MAX_VAL_LEN (256)

#ifndef DPRINTF
 #define DPRINTF(fmt,...) do { \
     if (debug) fprintf (stdout, fmt, ##__VA_ARGS__); \
 } while (0)
#endif

static char kvs_name[MAX_KVS_LEN];

/*
put:    avl str-->str
commit: avl str-->str
global: avl str-->str
mem:    avl void*-->null
*/

typedef map<string,string> str2str;
static str2str put;
static map_wrap_t commit;

extern "C" int PMI_Init( int *spawned )
{
  /* debug support */
  if (getenv ("PMI_MPI_DEBUG") != NULL) {
    debug = true;
  }

  /* check that we got a variable to write our flag value to */
  if (spawned == NULL) {
    return PMI_ERR_INVALID_ARG;
  }

  /* we don't support spawned procs */
  *spawned = PMI_FALSE;

  if (MPI_Init (NULL, NULL) != 0)
    goto error;
  if (MPI_Comm_size (MPI_COMM_WORLD, &ranks) != 0)
    goto error;
  if (MPI_Comm_rank (MPI_COMM_WORLD, &my_rank) != 0)
    goto error;

  id = 0; /* TODO: This may not work */
  if (snprintf(kvs_name, MAX_KVS_LEN, "%d", id) < MAX_KVS_LEN) {
    initialized = 1;
    DPRINTF ("%d: PMI_Init succeeded\n", my_rank);  
    return PMI_SUCCESS;
  } else {
    DPRINTF ("%d: PMI_Init (OOM)\n", my_rank);  
    return PMI_ERR_NOMEM;
  }

error:
  return PMI_FAIL;
}

extern "C" int PMI_Initialized( PMI_BOOL *out_initialized )
{
  /* check that we got a variable to write our flag value to */
  if (out_initialized == NULL) {
    DPRINTF ("%d: PMI_Initialized (not initialized).\n", my_rank);  
    return PMI_ERR_INVALID_ARG;
  }

  /* set whether we've initialized or not */
  *out_initialized = PMI_FALSE;
  if (initialized) {
    *out_initialized = PMI_TRUE;
  }
  DPRINTF ("%d: PMI_Initialized succeeded.\n", my_rank);  
  return PMI_SUCCESS;
}

extern "C" int PMI_Finalize( void )
{
  int rc = PMI_SUCCESS;

  if (MPI_Finalize() != 0) {
    DPRINTF ("%d: PMI_Finalize failed.\n", my_rank);
    rc = PMI_FAIL;
  }

  put.clear ();
  commit.m_map.clear ();

  DPRINTF ("%d: PMI_Finalize succeeded.\n", my_rank);
  return rc;
}

extern "C" int PMI_Get_size( int *size )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_Get_size (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (size == NULL) {
    DPRINTF ("%d: PMI_Get_size (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *size = ranks;
  DPRINTF ("%d: PMI_Get_size succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Get_rank( int *out_rank )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_Get_rank (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (out_rank == NULL) {
    DPRINTF ("%d: PMI_Get_rank (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *out_rank = my_rank;
  DPRINTF ("%d: PMI_Get_rank succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Get_universe_size( int *size )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_Get_universe_size (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (size == NULL) {
    DPRINTF ("%d: PMI_Get_universe_size (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *size = ranks;
  DPRINTF ("%d: PMI_Get_universe_size succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Get_appnum( int *appnum )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_Get_appnum (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (appnum == NULL) {
    DPRINTF ("%d: PMI_Get_appnum (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *appnum = id;
  DPRINTF ("%d: PMI_Get_appnum succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Abort(int exit_code, const char error_msg[])
{
  PMI_Finalize ();
  exit(exit_code);

  /* function prototype requires us to return something */
  DPRINTF ("%d: PMI_Abort succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Get_my_name( char kvsname[], int length )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Get_my_name (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (kvsname == NULL) {
    fprintf (stdout, "%d: PMI_KVS_Get_my_name (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  /* check that length is large enough */
  if (length < MAX_KVS_LEN) {
    fprintf (stdout, "%d: PMI_KVS_Get_my_name (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_LENGTH;
  }

  /* just use the id as the kvs space */
  strcpy(kvsname, kvs_name);
  DPRINTF ("%d: PMI_KVS_Get_my_name succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Get_name_length_max( int *length )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Get_name_length_max (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (length == NULL) {
    fprintf (stdout, "%d: PMI_KVS_Get_name_length_max (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *length = MAX_KVS_LEN;
  DPRINTF ("%d: PMI_KVS_Get_name_length_max succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Get_key_length_max( int *length )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Get_key_length_max (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (length == NULL) {
    DPRINTF ("%d: PMI_KVS_Get_key_length_max (PMI not initialized).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *length = MAX_KEY_LEN;
  DPRINTF ("%d: PMI_KVS_Get_key_length_max succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Get_value_length_max( int *length )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Get_value_length_max (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check that we got a variable to write our flag value to */
  if (length == NULL) {
    DPRINTF ("%d: PMI_KVS_Get_value_length_max (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_ARG;
  }

  *length = MAX_VAL_LEN;
  DPRINTF ("%d: PMI_KVS_Get_value_length_max succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Create( char kvsname[], int length )
{
  /* since we don't support spawning, we just have a static key value space */
  int rc = PMI_KVS_Get_my_name(kvsname, length);
  DPRINTF ("%d: PMI_KVS_Create (rc=%d).\n", my_rank, rc);
  return rc;
}

extern "C" int PMI_KVS_Put( const char kvsname[], const char key[], const char value[])
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Put(PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check length of name */
  if (kvsname == NULL || strlen(kvsname) > MAX_KVS_LEN) {
    DPRINTF ("%d: PMI_KVS_Put (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }

  /* check length of key */
  if (key == NULL || strlen(key) > MAX_KEY_LEN) {
    DPRINTF ("%d: PMI_KVS_Put (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KEY;
  }

  /* check length of value */
  if (value == NULL || strlen(value) > MAX_VAL_LEN) {
    DPRINTF ("%d: PMI_KVS_Put (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_VAL;
  }

  /* check that kvsname is the correct one */
  if (strcmp(kvsname, kvs_name) != 0) {
    DPRINTF ("%d: PMI_KVS_Put (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }
      
  /* add string to put */
  pair<string,string> kv(key, value);
  put.insert(kv);

  DPRINTF ("%d: PMI_KVS_Put succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Commit( const char kvsname[] )
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Commit (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check length of name */
  if (kvsname == NULL || strlen(kvsname) > MAX_KVS_LEN) {
    DPRINTF ("%d: PMI_KVS_Commit (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }

  /* check that kvsname is the correct one */
  if (strcmp(kvsname, kvs_name) != 0) {
    DPRINTF ("%d: PMI_KVS_Commit (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }
      
  /* copy all entries in put to commit, in order to overwrite existing
   * entires in commit, we may have to erase before the insert */
  str2str::iterator i;
  str2str::iterator target;
  for (i = put.begin(); i != put.end(); i++) {
    /* search for this string in commit */
    string key = i->first;
    target = commit.m_map.find(key);

    /* if we found an existing entry in commit, we need to erase it */
    if (target != commit.m_map.end()) {
      commit.m_map.erase(target);
    }

    /* now insert value from put */
    commit.m_map.insert(*i);
  }

  /* clear put */
  put.clear();

  DPRINTF ("%d: PMI_KVS_Commit succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Barrier( void )
{
  int rc = -1;
  int total_size = 0;
  char *buf = NULL;

  /* check that we're initialized */
  if (!initialized) {
    /* would like to return PMI_ERR_INIT here, but definition says
     * it must return either SUCCESS or FAIL, and since user knows
     * that PMI_FAIL == -1, he could be testing for this */
    DPRINTF ("%d: PMI_Barrier (PMI not initialized).\n", my_rank);
    return PMI_FAIL;
  }

  BinomialReducer<map_wrap_t> reducer;
  if ( (rc = reducer.reduce (0, my_rank, ranks, commit)) != 0) {
    DPRINTF ("%d: PMI_Barrier (reducer failed).\n", my_rank);
    return PMI_FAIL;
  }

  if (my_rank == 0) {
    total_size = commit.packed_size ();
  }
  if ( (rc = MPI_Bcast(&total_size, 1, MPI_INT, 0, MPI_COMM_WORLD)) != 0) {
    DPRINTF ("%d: PMI_Barrier (Bcast failed for total_size).\n", my_rank);
    return PMI_FAIL;
  }
  if ( (buf = (char *)malloc (total_size)) == NULL) {
    DPRINTF ("%d: PMI_Barrier (OOM).\n", my_rank);
    return PMI_FAIL;
  }
  if (my_rank == 0) {
    if ( (rc = commit.pack (buf, total_size)) == 0) {
      DPRINTF ("%d: PMI_Barrier (pack failed).\n", my_rank);
      return PMI_FAIL;
    }
  } 
  if ( (rc = MPI_Bcast(buf, total_size, MPI_CHAR, 0, MPI_COMM_WORLD)) != 0) {
    return PMI_FAIL;
  }
  if (my_rank != 0) {
    size_t size;
    if ( (size = commit.unpack (buf, total_size)) < static_cast<size_t>(total_size)) {
      DPRINTF ("%d: PMI_Barrier (unpack failed: ret size (%d) vs. size (%d)).\n", my_rank, (int)size, (int)total_size);
      return PMI_FAIL;
    } 
  }

  DPRINTF ("%d: PMI_Barrier succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_KVS_Get( const char kvsname[], const char key[], char value[], int length)
{
  /* check that we're initialized */
  if (!initialized) {
    DPRINTF ("%d: PMI_KVS_Get (PMI not initialized).\n", my_rank);
    return PMI_ERR_INIT;
  }

  /* check length of name */
  if (kvsname == NULL || strlen(kvsname) > MAX_KVS_LEN) {
    DPRINTF ("%d: PMI_KVS_Get (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }

  /* check that kvsname is the correct one */
  if (strcmp(kvsname, kvs_name) != 0) {
    DPRINTF ("%d: PMI_KVS_Get (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KVS;
  }
      
  /* check length of key */
  if (key == NULL || strlen(key) > MAX_KEY_LEN) {
    DPRINTF ("%d: PMI_KVS_Get (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_KEY;
  }

  /* check that we have a buffer to write something to */
  if (value == NULL) {
    DPRINTF ("%d: PMI_KVS_Get (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_VAL;
  }

  /* lookup entry from commit */
  string keystr = key;
  str2str::iterator target = commit.m_map.find(keystr);
  if (target == commit.m_map.end()) {
    /* failed to find the key */
    DPRINTF ("%d: PMI_KVS_Get (ENOENT).\n", my_rank);
    return PMI_FAIL;
  }

  /* check that the user's buffer is large enough */
  int len = (target->second).size() + 1;
  if (length < len) {
    DPRINTF ("%d: PMI_KVS_Get (invalid argument).\n", my_rank);
    return PMI_ERR_INVALID_LENGTH;
  }

  /* copy the value into user's buffer */
  strcpy(value, (target->second).c_str());

  DPRINTF ("%d: PMI_KVS_Get succeeded.\n", my_rank);
  return PMI_SUCCESS;
}

extern "C" int PMI_Spawn_multiple(
  int count, const char * cmds[], const char ** argvs[], const int maxprocs[],
  const int info_keyval_sizesp[], const PMI_keyval_t * info_keyval_vectors[],
  int preput_keyval_size, const PMI_keyval_t preput_keyval_vector[], int errors[])
{
  /* we don't implement this yet, but mvapich2 needs a reference */
  return PMI_FAIL;
}

/*
 * vi:tabstop=2 shiftwidth=2 expandtab
 */
