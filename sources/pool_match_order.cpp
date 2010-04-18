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

#include "pool_match_order.hpp"

static PoolMatchOrder::PlayerPair pool_3_pairs[3] =
{ {2, 3},
  {1, 3},
  {1, 2}};

static PoolMatchOrder::PlayerPair pool_4_pairs[6] =
{ {1, 4},
  {2, 3},
  {1, 3},
  {2, 4},
  {3, 4},
  {1, 2}};

static PoolMatchOrder::PlayerPair pool_5_pairs[10] =
{ {1, 2},
  {3, 4},
  {5, 1},
  {2, 3},
  {5, 4},
  {1, 3},
  {2, 5},
  {4, 1},
  {3, 5},
  {4, 2}};

static PoolMatchOrder::PlayerPair pool_6_pairs[15] =
{ {1, 2},
  {4, 5},
  {2, 3},
  {5, 6},
  {3, 1},
  {6, 4},
  {2, 5},
  {1, 4},
  {5, 3},
  {1, 6},
  {4, 2},
  {3, 6},
  {5, 1},
  {3, 4},
  {6, 2}};

static PoolMatchOrder::PlayerPair pool_7_pairs[21] =
{ {1, 4},
  {2, 5},
  {3, 6},
  {7, 1},
  {5, 4},
  {2, 3},
  {6, 7},
  {5, 1},
  {4, 3},
  {6, 2},
  {5, 7},
  {3, 1},
  {4, 6},
  {7, 2},
  {3, 5},
  {1, 6},
  {2, 4},
  {7, 3},
  {6, 5},
  {1, 2},
  {4, 7}};

static PoolMatchOrder::PlayerPair pool_8_pairs[28] =
{ {2, 3},
  {1, 5},
  {7, 4},
  {6, 8},
  {1, 2},
  {3, 4},
  {5, 6},
  {8, 7},
  {4, 1},
  {5, 2},
  {8, 3},
  {6, 7},
  {4, 2},
  {8, 1},
  {7, 5},
  {3, 6},
  {2, 8},
  {5, 4},
  {6, 1},
  {3, 7},
  {4, 8},
  {2, 6},
  {3, 5},
  {1, 7},
  {4, 6},
  {8, 5},
  {7, 2},
  {1, 3}};

static PoolMatchOrder::PlayerPair pool_9_pairs[36] =
{ {1, 9},
  {2, 8},
  {3, 7},
  {4, 6},
  {1, 5},
  {2, 9},
  {8, 3},
  {7, 4},
  {6, 5},
  {1, 2},
  {9, 3},
  {8, 4},
  {7, 5},
  {6, 1},
  {3, 2},
  {9, 4},
  {5, 8},
  {7, 6},
  {3, 1},
  {2, 4},
  {5, 9},
  {8, 6},
  {7, 1},
  {4, 3},
  {5, 2},
  {6, 9},
  {8, 7},
  {4, 1},
  {5, 3},
  {6, 2},
  {9, 7},
  {1, 8},
  {4, 5},
  {3, 6},
  {5, 7},
  {9, 8}};

static PoolMatchOrder::PlayerPair pool_10_pairs[45] =
{ {1, 4},
  {6, 9},
  {2, 5},
  {7, 10},
  {3, 1},
  {8, 6},
  {4, 5},
  {9, 10},
  {2, 3},
  {7, 8},
  {5, 1},
  {10, 6},
  {4, 2},
  {9, 7},
  {5, 3},
  {10, 8},
  {1, 2},
  {6, 7},
  {3, 4},
  {8, 9},
  {5, 10},
  {1, 6},
  {2, 7},
  {3, 8},
  {4, 9},
  {6, 5},
  {10, 2},
  {8, 1},
  {7, 4},
  {9, 3},
  {2, 6},
  {5, 8},
  {4, 10},
  {1, 9},
  {3, 7},
  {8, 2},
  {6, 4},
  {9, 5},
  {10, 3},
  {7, 1},
  {4, 8},
  {2, 9},
  {3, 6},
  {5, 7},
  {1, 10}};

static PoolMatchOrder::PlayerPair pool_11_pairs[55] =
{ {1, 2},
  {7, 8},
  {4, 5},
  {10, 11},
  {2, 3},
  {8, 9},
  {5, 6},
  {3, 1},
  {9, 7},
  {6, 4},
  {2, 5},
  {8, 11},
  {1, 4},
  {7, 10},
  {5, 3},
  {11, 9},
  {1, 6},
  {4, 2},
  {10, 8},
  {3, 6},
  {5, 1},
  {11, 7},
  {3, 4},
  {9, 10},
  {6, 2},
  {1, 7},
  {3, 9},
  {10, 4},
  {8, 2},
  {5, 11},
  {1, 8},
  {9, 2},
  {3, 10},
  {4, 11},
  {6, 7},
  {9, 1},
  {2, 10},
  {11, 3},
  {7, 5},
  {6, 8},
  {10, 1},
  {11, 2},
  {4, 7},
  {8, 5},
  {6, 9},
  {11, 1},
  {7, 3},
  {4, 8},
  {9, 5},
  {6, 10},
  {2, 7},
  {8, 3},
  {4, 9},
  {10, 5},
  {6, 11}};

static PoolMatchOrder::PlayerPair *all_pool_pairs[12] =
{
  NULL,
  NULL,
  NULL,
  pool_3_pairs,
  pool_4_pairs,
  pool_5_pairs,
  pool_6_pairs,
  pool_7_pairs,
  pool_8_pairs,
  pool_9_pairs,
  pool_10_pairs,
  pool_11_pairs
};

// --------------------------------------------------------------------------------
PoolMatchOrder::PlayerPair *PoolMatchOrder::GetPlayerPair (guint pool_size)
{
  if (pool_size <= 11)
  {
    return all_pool_pairs[pool_size];
  }
  else
  {
    return NULL;
  }
}
