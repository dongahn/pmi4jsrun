/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef MAP_WRAP_HPP
#define MAP_WRAP_HPP

#include <map>
#include <string>

struct map_wrap_t {
  const int MAP_WRAP_SEND_SIZE_TAG = 14568;
  const int MAP_WRAP_SEND_DATA_TAG = 14569;

  size_t pack (char *buf, size_t len) const;
  size_t packed_size () const;
  size_t unpack (const char *buf, size_t len);
  bool insert (std::string key, std::string value);
  int send (int receiver) const;
  int receive (int sender);
  std::map<std::string, std::string> m_map;
};

#endif // MAP_WRAP_HPP

/*
 * vi:tabstop=2 shiftwidth=2 expandtab
 */
