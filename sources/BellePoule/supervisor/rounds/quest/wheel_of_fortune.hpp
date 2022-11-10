// Copyright (C) 2009 Yannick Le Roux.
// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <random>
#include <set>
#include <utility>
#include <vector>

#include <stdint.h>
namespace Quest
{
  class WheelOfFortune
  {
    public:
      WheelOfFortune(uint32_t n_fencers,
                     uint32_t n_matches_per_fencer,
                     uint32_t rand_seed) :
                     _n_fencers(n_fencers),
                     _n_matches_per_fencer(n_matches_per_fencer),
                     _rand_seed(rand_seed) {}


      // Generate a set of matchups where all n_fencers participants meet exactly
      // n_matches_per_fencer opponants.
      // Note: n_fencers * n_matches_per_fencer must be even, otherwise there is no
      //       valid solution. In such a case, and empty set is returned.
      std::vector<std::pair<uint32_t,uint32_t>> generate_matchups();

    private:

      // Populates the matchups
      void find_matchups();

      // Reorder the vector of matchups to separate fencers matches
      void spread_fencers_matches();

      // input
      uint32_t _n_fencers;
      uint32_t _n_matches_per_fencer;
      uint32_t _rand_seed;

      // internal state
      std::mt19937 _prng; // The pseudorandom generator we'll use
      std::set<std::pair<uint32_t,uint32_t>> _matchups_set; // Set used for fast lookup
      std::vector<std::pair<uint32_t,uint32_t>> _matchups; // Vector used for ordering
  };
}
