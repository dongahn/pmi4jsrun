/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#include <mpi.h>
#include <string.h>
#include <iostream>
#include "map_wrap.hpp"

size_t map_wrap_t::packed_size () const
{
  size_t size = 0;
  std::map<std::string, std::string>::const_iterator i;
  for (i = m_map.begin(); i != m_map.end(); i++) {
    size_t key_len = (i->first).size() + 1;
    size_t val_len = (i->second).size() + 1;
    size += (key_len + val_len);
  }
  return size;
}

size_t map_wrap_t::pack (char *buf, size_t len) const
{
  if (buf == NULL || len < packed_size()) {
    return 0;
  }

  char *p = buf;
  std::map<std::string, std::string>::const_iterator i;
  for (i = m_map.begin (); i != m_map.end(); i++) {
    /* copy in key string */
    strcpy(p, (i->first).c_str());
    p += (i->first).size() + 1;

    /* copy in value string */
    strcpy(p, (i->second).c_str());
    p += (i->second).size() + 1;
  }
  return static_cast<size_t>(p - buf);
}

size_t map_wrap_t::unpack (const char *buf, size_t len)
{
  const char *last = buf + len;
  const char *p = buf;
  size_t size = 0;

  while (p < last) {
    std::string key = p;
    p += key.size () + 1;
    std::string value = p;
    p += value.size () + 1; 
    m_map[key] = value;
    size++;
  }
  return static_cast<size_t>(p - buf);
}

bool map_wrap_t::insert (std::string key, std::string value)
{
  std::pair<std::map<std::string, std::string>::iterator, bool> ret;
  ret = m_map.insert (std::pair<std::string, std::string>(key, value));
  return ret.second;
}

int map_wrap_t::send (int receiver) const
{
  char *send_buf = NULL;
  int rc = -1;
  int buf_size = (int) packed_size();

  if ( (rc = MPI_Send((void *)&(buf_size), 1, MPI_INT, receiver,
                      MAP_WRAP_SEND_SIZE_TAG, MPI_COMM_WORLD)) != 0) {
    return rc;  
  }
  if (buf_size == 0) {
    return 0;
  }
  if ( !(send_buf = (char *) malloc(buf_size))) {
    return -1;
  }
  if ( (rc = pack(send_buf, buf_size)) == 0 ) {
    return -1;
  }
  if ( (rc = MPI_Send((void *)send_buf, buf_size, MPI_CHAR, receiver,
                       MAP_WRAP_SEND_DATA_TAG, MPI_COMM_WORLD) != 0)) {
    return rc;
  }
  free(send_buf);
  return 0;
}

int map_wrap_t::receive (int sender)
{
  int rc = -1;
  int buf_size;
  MPI_Status status;
  char *recv_buf = NULL;

  if ( (rc = MPI_Recv((void *)&buf_size, 1, MPI_INT, sender,
                      MAP_WRAP_SEND_SIZE_TAG, MPI_COMM_WORLD, &status))) {
    return rc;
  }
  if (buf_size == 0) {
    return 0;
  }
  if ( !(recv_buf = (char *) malloc(buf_size))) {
    return -1;
  }
  if ( (rc = MPI_Recv((void *) recv_buf, buf_size, MPI_CHAR, sender,
                      MAP_WRAP_SEND_DATA_TAG, MPI_COMM_WORLD, &status)) != 0) {
    free (recv_buf);
    return rc;
  }
  if (unpack (recv_buf, buf_size) < static_cast<size_t>(buf_size))  {
    free (recv_buf);
    return -1;
  }

  free(recv_buf);
  return 0;
}

/*
 * vi:tabstop=2 shiftwidth=2 expandtab
 */
