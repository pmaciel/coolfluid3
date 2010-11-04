// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef PECommPattern2_hpp
#define PECommPattern2_hpp

////////////////////////////////////////////////////////////////////////////////

/**
  @file PECommPattern2.hpp
  @author Tamas Banyai
  Parallel Communication Pattern.
  This class provides functionality to collect communication
**/

// TODO:
// 1.: make possible to use compattern on non-registered data
// 2.: when adding, how to give values to the newly createable elements?

////////////////////////////////////////////////////////////////////////////////

#include "Common/BoostArray.hpp"

#include "Common/CF.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace CF {
  namespace Common  {

////////////////////////////////////////////////////////////////////////////////

class Common_API PECommPattern2 {

public:

  /// constructor
  PECommPattern2();

  /// constructor with settting up communication pattern
  /// @param gid vector of global ids
  /// @param rank vector of ranks where given global ids are updatable
  /// @see setup for committing changes
  PECommPattern2(std::vector<Uint> gid, std::vector<Uint> rank);

  /// destructor
  ~PECommPattern2();

  /// build and/or modify communication pattern
  /// this function sets actually up the communication pattern
  /// beware: interprocess communication heavy
  /// @param gid vector of global ids
  /// @param rank vector of ranks where given global ids are updatable
  void setup(std::vector<Uint> gid, std::vector<Uint> rank);

  /// build and/or modify communication pattern
  /// this function sets actually up the communication pattern
  /// beware: interprocess communication heavy
  void setup();

  /// add element to the commpattern
  /// when all changes done, all needs to be committed by calling setup
  /// @param gid global id
  /// @param rank rank where given global id is updatable
  /// @see setup for committing changes
  void add(Uint gid, Uint rank);

  /// delete element from the commpattern
  /// when all changes done, all needs to be committed by calling setup
  /// @param gid global id
  /// @param rank rank where given global id is updatable
  /// @see setup for committing changes
  void remove(Uint gid, Uint rank);

  /// move element along partitions
  /// when all changes done, all needs to be committed by calling setup
  /// @param gid global id
  /// @param rank rank where given global id is updatable
  /// @see setup for committing changes
  void move(Uint gid, Uint rank);

  /// registers data
  template <typename T> insert(T*);

  /// releases data
  template <typename T> release(T*);

  /// synchronize items
  void sync();

  /// clears the communication pattern and releases every data associated to commpattern
  void clear();

private:

  /// storing updatable information
  std::vector<bool> m_updatable;

  /// this is the global id
  std::vector< Uint > gid;

  /// this is the process-wise counter of sending communication pattern
  std::vector< int > m_sendCount;

  /// this is the map of sending communication pattern
  std::vector< int > m_sendMap;

  /// this is the process-wise counter of receiveing communication pattern
  std::vector< int > m_receiveCount;

  /// this is the map of receiveing communication pattern
  std::vector< int > m_receiveMap;

  /// flag telling if communication pattern is up-to-date
  bool m_isCommPatternPrepared;

};


////////////////////////////////////////////////////////////////////////////////

  }  // namespace Common
} // namespace CF

////////////////////////////////////////////////////////////////////////////////

#endif // PECommPattern2_hpp