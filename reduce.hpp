/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/*
 * reduction.h
 *
 * Template classes to perform different tree-based reduction algorithms.
 * The reductions are used primary when counting the number of times edges
 * are present in SMMs globally. This is done before compressing SSMs.
 *
 *      Author: Ignacio Laguna and Dong H. Ahn
 *     Contact: lagunaperalt1@llnl.gov and ahn1@llnl.gov
 *
 *
 */

#ifndef REDUCTION_H
#define REDUCTION_H

extern "C" {
#include <math.h>
}

#include <iostream>

/**
 * Abstraction to calculate 'senders' and 'receivers'
 * in a tree-based reduction.
 *
 * The class T needs to implement two member functions:
 * send(int destination)
 * receive(int source)
 *
 * The reduction operations are performed inside these functions.
 */
template <class T>
class Reducer {
public:
  virtual ~Reducer() {}

  /**
   * Virtual reduction operation (implemented in derived classes)
   */
  virtual int reduce(int root, int rank, int size, T &reduceObj) = 0;
};


/**
 * ----------------------------------
 * Binomial Tree Reduction from MPICH
 * ----------------------------------
 *
 * This algorithm is adapted from MPICH. It performs a binomial-tree reduction
 * assuming that the operations are commutative. The root process can be
 * specified in the reduce() function.
 */
template <class T>
class BinomialReducer : public Reducer<T> {
public:
  int reduce(int root, int rank, int size, T &reduceObj)
  {
    int rc = 0;
    int mask = 0x1, relrank, source, destination;
    relrank = (rank - root + size % size);
    while (mask < size) {
      // Receive
      if ((mask & relrank) == 0) {
        source = (relrank | mask);
        if (source < size) {
          source = (source + root) % size;
          //std::cout << "Proc " << rank << " recv from " << source << std::endl;
          if ( (rc = reduceObj.receive(source)) != 0) {
              return rc;
          }
        }
      } else {
        destination = ((relrank & (~ mask)) + root) % size;
        //std::cout << "Proc " << rank << " send to " << destination << std::endl;
        if ( ( rc = reduceObj.send(destination)) < 0) {
          return rc;
        } 
      }
      mask <<= 1;
    }
    return 0;
  }
};

#endif // REDUCTION_H

/*
 * vi:tabstop=2 shiftwidth=2 expandtab
 */
