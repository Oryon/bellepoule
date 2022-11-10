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

#include "wheel_of_fortune.hpp"

#include <algorithm>
#include <vector>

namespace Quest
{
  std::vector<std::pair<uint32_t,uint32_t>> WheelOfFortune::generate_matchups() {
    // This algorithm is generating a random regular graph, which means a graph where
    // each node has exactly N edges to other nodes.
    uint32_t n_matchups = _n_fencers * _n_matches_per_fencer;

    if ((n_matchups & 0x1) || (_n_fencers < (_n_matches_per_fencer - 1))) {
      return _matchups;
    }

    _prng = std::mt19937(_rand_seed);

    // Generate matches
    find_matchups();

    // Get them in a good order to avoid fencers to have consecutive matches
    spread_fencers_matches();

    return _matchups;
  }

  void WheelOfFortune::find_matchups() {
    uint32_t n_matchups = (_n_fencers * _n_matches_per_fencer) / 2;

    std::vector<uint32_t> edges;
    do { // Algorithm might fail. If it does, we try again.
      // Remove from last run
      edges.clear();
      _matchups_set.clear();
      _matchups.clear();

      // Init desired edges with n_matches_per_fencer items per fencer
      for (uint32_t fi = 0; fi < _n_fencers; fi++) {
        for (uint32_t mi = 0; mi < _n_matches_per_fencer; mi++) {
          edges.push_back(fi);
        }
      }

      std::vector<uint32_t> remaining_edges;
      uint32_t found_matchups_this_turn = 0;
      do {
        // Pick random matchups by shuffling the edges
        std::shuffle(edges.begin(), edges.end(), _prng);

        // Check all picked matchups
        found_matchups_this_turn = 0;
        for (uint32_t ei = 0; ei < edges.size(); ei += 2) {
          std::pair<int, int> matchup(std::minmax(edges[ei], edges[ei+1]));
          if ((edges[ei] == edges[ei+1]) || _matchups_set.count(matchup)) {
            // The picked matchup is invalid
            // We need to try again
            remaining_edges.push_back(edges[ei]);
            remaining_edges.push_back(edges[ei+1]);
          } else {
            // the matchup is valid
            _matchups_set.insert(matchup);
            _matchups.push_back(matchup);
            found_matchups_this_turn++;
          }
        }

        // Remaining edges become new edges
        edges.clear();
        edges.swap(remaining_edges);

      } while ((_matchups_set.size() != n_matchups) && (found_matchups_this_turn != 0));
    } while (_matchups_set.size() != n_matchups);

  }

  void WheelOfFortune::spread_fencers_matches() {
    std::vector<int32_t> last_match_by_fencer;
    uint32_t worst_case_match_proximity = 1000;

    for (uint32_t fi = 0; fi < _n_fencers; fi++) {
      // Last match was an eternity ago
      last_match_by_fencer.push_back(-1000);
    }

    uint32_t n_matchups = _matchups.size();
    _matchups.clear(); // We will fill this with matchups
    while (_matchups.size() != n_matchups) {
      // Current match index, used to calculate age
      uint32_t match_index = _matchups.size();

      // Let's find best match to add next so that fencers do not have matches
      // too close.
      uint32_t highest_match_proximity = 0;
      std::pair<uint32_t,uint32_t> best_match;
      for (auto& m : _matchups_set) {
        uint32_t f1_proximity = match_index - last_match_by_fencer[m.first];
        uint32_t f2_proximity = match_index - last_match_by_fencer[m.second];

        // Only retaining the worst case, which means the lowest proximity
        uint32_t worst = (f1_proximity < f2_proximity) ? f1_proximity : f2_proximity;

        if (worst > highest_match_proximity) {
          highest_match_proximity = worst;
          best_match = m;
        }
      }

      _matchups.push_back(best_match); // Adding match to list
      _matchups_set.erase(best_match); // Removing match from set

      // Recording time of last match
      last_match_by_fencer[best_match.first] = match_index;
      last_match_by_fencer[best_match.second] = match_index;

      // Recording worst case, for stat
      if (highest_match_proximity < worst_case_match_proximity) {
        worst_case_match_proximity = highest_match_proximity;
      }
    }
  }

}
