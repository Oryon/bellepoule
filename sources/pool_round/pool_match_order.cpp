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

#include "player.hpp"
#include "pool.hpp"

#include "pool_match_order.hpp"

struct PlayerPair
{
  guint _a;
  guint _b;
};

static PlayerPair pool_2_fencing_pairs[1] =
{{1, 2}};

static PlayerPair pool_3_kendo_pairs[3] =
{ {1, 2},
  {1, 3},
  {2, 3}};

static PlayerPair pool_3_fencing_pairs[3] =
{ {2, 3},
  {1, 3},
  {1, 2}};

static PlayerPair pool_4_fencing_pairs[6] =
{ {1, 4},
  {2, 3},
  {1, 3},
  {2, 4},
  {3, 4},
  {1, 2}};

static PlayerPair pool_5_fencing_pairs[10] =
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

static PlayerPair pool_6_fencing_pairs[15] =
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

static PlayerPair pool_7_fencing_pairs[21] =
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

static PlayerPair pool_8_fencing_pairs[28] =
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

static PlayerPair pool_9_fencing_pairs[36] =
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
  {7, 2},
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

static PlayerPair pool_10_fencing_pairs[45] =
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

static PlayerPair pool_11_fencing_pairs[55] =
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

static PlayerPair pool_12_fencing_pairs[66] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {3, 1},
  {2, 4},
  {7, 5},
  {6, 8},
  {11, 9},
  {10, 12},
  {4, 1},
  {2, 3},
  {8, 5},
  {6, 7},
  {12, 9},
  {10, 11},
  {1, 5},
  {4, 8},
  {6, 2},
  {7, 3},
  {9, 1},
  {12, 5},
  {4, 10},
  {8, 11},
  {2, 7},
  {3, 6},
  {5, 9},
  {1, 12},
  {8, 10},
  {11, 4},
  {5, 2},
  {9, 7},
  {12, 3},
  {1, 6},
  {10, 2},
  {5, 11},
  {8, 9},
  {4, 7},
  {3, 10},
  {12, 6},
  {11, 1},
  {2, 8},
  {9, 4},
  {7, 10},
  {5, 3},
  {6, 11},
  {2, 12},
  {1, 8},
  {4, 5},
  {9, 3},
  {7, 11},
  {10, 6},
  {8, 12},
  {9, 2},
  {7, 1},
  {6, 4},
  {3, 11},
  {10, 5},
  {12, 7},
  {6, 9},
  {8, 3},
  {1, 10},
  {4, 12},
  {11, 2}};

static PlayerPair pool_13_fencing_pairs[78] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 1},
  {2, 3},
  {4, 5},
  {6, 7},
  {8, 9},
  {10, 11},
  {12, 1},
  {2, 13},
  {3, 5},
  {4, 6},
  {7, 9},
  {8, 10},
  {1, 11},
  {12, 2},
  {13, 3},
  {5, 7},
  {9, 4},
  {6, 8},
  {10, 1},
  {11, 2},
  {3, 12},
  {5, 13},
  {7, 4},
  {9, 6},
  {1, 8},
  {2, 10},
  {11, 3},
  {12, 5},
  {4, 13},
  {1, 7},
  {6, 2},
  {3, 9},
  {8, 11},
  {10, 5},
  {4, 12},
  {13, 7},
  {6, 1},
  {2, 9},
  {8, 3},
  {5, 11},
  {10, 4},
  {7, 12},
  {13, 6},
  {9, 1},
  {2, 8},
  {11, 4},
  {3, 10},
  {12, 6},
  {9, 13},
  {1, 5},
  {7, 2},
  {4, 8},
  {6, 11},
  {10, 12},
  {1, 3},
  {8, 13},
  {5, 9},
  {11, 7},
  {6, 10},
  {12, 8},
  {2, 4},
  {13, 11},
  {3, 7},
  {8, 5},
  {9, 12},
  {10, 13},
  {4, 1},
  {3, 6},
  {5, 2},
  {11, 9},
  {7, 10},
  {12, 13}};

static PlayerPair pool_14_fencing_pairs[91] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {3, 1},
  {2, 4},
  {7, 5},
  {6, 8},
  {11, 9},
  {10, 12},
  {1, 13},
  {14, 3},
  {2, 5},
  {4, 7},
  {6, 9},
  {8, 11},
  {10, 1},
  {12, 13},
  {3, 2},
  {5, 14},
  {4, 6},
  {9, 7},
  {8, 1},
  {11, 10},
  {12, 2},
  {13, 3},
  {4, 5},
  {14, 6},
  {1, 7},
  {8, 9},
  {2, 10},
  {11, 3},
  {12, 4},
  {5, 13},
  {6, 1},
  {7, 14},
  {2, 8},
  {9, 3},
  {10, 4},
  {12, 14},
  {5, 11},
  {13, 6},
  {7, 2},
  {1, 12},
  {14, 8},
  {3, 10},
  {4, 9},
  {1, 5},
  {6, 11},
  {12, 7},
  {2, 13},
  {8, 3},
  {10, 14},
  {4, 1},
  {9, 5},
  {6, 7},
  {11, 2},
  {3, 12},
  {13, 8},
  {14, 1},
  {5, 10},
  {11, 4},
  {2, 9},
  {3, 6},
  {7, 13},
  {8, 12},
  {1, 11},
  {4, 14},
  {5, 3},
  {10, 6},
  {9, 13},
  {14, 2},
  {7, 11},
  {8, 4},
  {12, 5},
  {1, 9},
  {13, 10},
  {6, 2},
  {3, 7},
  {9, 12},
  {14, 11},
  {13, 4},
  {5, 8},
  {10, 7},
  {12, 6},
  {11, 13},
  {9, 14},
  {8, 10}};

static PlayerPair pool_15_fencing_pairs[105] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 1},
  {2, 3},
  {4, 5},
  {6, 7},
  {8, 9},
  {10, 11},
  {12, 13},
  {14, 1},
  {2, 15},
  {3, 5},
  {4, 6},
  {7, 9},
  {8, 10},
  {11, 13},
  {1, 12},
  {14, 2},
  {15, 3},
  {5, 7},
  {9, 4},
  {6, 8},
  {10, 13},
  {1, 11},
  {12, 2},
  {3, 14},
  {5, 15},
  {4, 7},
  {6, 9},
  {13, 8},
  {10, 1},
  {2, 11},
  {12, 3},
  {14, 5},
  {15, 4},
  {7, 13},
  {1, 6},
  {9, 2},
  {8, 11},
  {3, 10},
  {5, 12},
  {4, 14},
  {7, 15},
  {13, 1},
  {2, 6},
  {11, 9},
  {8, 3},
  {10, 5},
  {12, 4},
  {14, 7},
  {13, 15},
  {9, 1},
  {6, 11},
  {2, 8},
  {3, 7},
  {4, 10},
  {13, 5},
  {14, 12},
  {15, 9},
  {1, 8},
  {6, 3},
  {7, 11},
  {4, 2},
  {10, 12},
  {9, 5},
  {15, 14},
  {13, 3},
  {1, 7},
  {8, 4},
  {10, 6},
  {11, 5},
  {12, 9},
  {2, 13},
  {3, 1},
  {8, 14},
  {6, 15},
  {7, 10},
  {11, 4},
  {5, 2},
  {9, 13},
  {12, 8},
  {14, 6},
  {1, 4},
  {11, 3},
  {15, 10},
  {2, 7},
  {5, 8},
  {13, 6},
  {9, 14},
  {12, 15},
  {5, 1},
  {10, 2},
  {11, 14},
  {7, 12},
  {4, 13},
  {8, 15},
  {3, 9},
  {6, 12},
  {14, 10},
  {15, 11}};

static PlayerPair pool_16_fencing_pairs[120] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 16},
  {3, 1},
  {2, 4},
  {7, 5},
  {6, 8},
  {11, 9},
  {10, 12},
  {15, 13},
  {14, 16},
  {4, 1},
  {2, 3},
  {8, 5},
  {6, 7},
  {12, 9},
  {10, 11},
  {16, 13},
  {14, 15},
  {1, 5},
  {4, 8},
  {6, 2},
  {7, 3},
  {9, 13},
  {12, 16},
  {14, 10},
  {15, 11},
  {8, 1},
  {5, 4},
  {2, 7},
  {3, 6},
  {16, 9},
  {13, 12},
  {10, 15},
  {11, 14},
  {1, 7},
  {8, 2},
  {5, 3},
  {4, 6},
  {9, 15},
  {16, 10},
  {12, 14},
  {13, 11},
  {1, 6},
  {2, 5},
  {3, 8},
  {4, 7},
  {9, 14},
  {10, 13},
  {11, 16},
  {12, 15},
  {9, 1},
  {6, 14},
  {10, 2},
  {5, 13},
  {11, 3},
  {8, 16},
  {12, 4},
  {7, 15},
  {14, 1},
  {6, 9},
  {13, 2},
  {5, 10},
  {16, 3},
  {8, 11},
  {15, 4},
  {7, 12},
  {1, 13},
  {2, 14},
  {9, 5},
  {10, 6},
  {3, 15},
  {4, 16},
  {12, 8},
  {11, 7},
  {1, 10},
  {2, 9},
  {14, 5},
  {3, 12},
  {13, 6},
  {4, 11},
  {16, 7},
  {15, 8},
  {12, 1},
  {10, 3},
  {11, 2},
  {9, 4},
  {5, 16},
  {7, 14},
  {6, 15},
  {8, 13},
  {1, 11},
  {2, 12},
  {3, 9},
  {4, 10},
  {15, 5},
  {16, 6},
  {13, 7},
  {14, 8},
  {15, 1},
  {5, 11},
  {16, 2},
  {6, 12},
  {13, 3},
  {7, 9},
  {14, 4},
  {8, 10},
  {1, 16},
  {2, 15},
  {5, 12},
  {6, 11},
  {3, 14},
  {4, 13},
  {7, 10},
  {8, 9}};

static PlayerPair pool_17_fencing_pairs[136] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 16},
  {17, 1},
  {2, 3},
  {4, 5},
  {6, 7},
  {8, 9},
  {10, 11},
  {12, 13},
  {14, 15},
  {16, 1},
  {2, 17},
  {3, 5},
  {4, 6},
  {7, 9},
  {8, 10},
  {11, 13},
  {12, 14},
  {1, 15},
  {16, 2},
  {17, 3},
  {5, 7},
  {9, 4},
  {6, 8},
  {10, 13},
  {14, 11},
  {1, 12},
  {15, 2},
  {3, 16},
  {5, 17},
  {4, 7},
  {6, 9},
  {13, 8},
  {10, 14},
  {11, 1},
  {2, 12},
  {15, 3},
  {16, 5},
  {17, 4},
  {7, 13},
  {10, 6},
  {9, 14},
  {8, 1},
  {2, 11},
  {12, 3},
  {5, 15},
  {4, 16},
  {7, 17},
  {13, 6},
  {1, 10},
  {14, 8},
  {9, 2},
  {3, 11},
  {12, 5},
  {15, 4},
  {16, 7},
  {6, 17},
  {13, 1},
  {2, 10},
  {8, 3},
  {14, 5},
  {11, 9},
  {4, 12},
  {7, 15},
  {6, 16},
  {17, 13},
  {1, 3},
  {8, 2},
  {5, 10},
  {14, 4},
  {16, 17},
  {9, 12},
  {11, 7},
  {17, 8},
  {15, 6},
  {13, 16},
  {1, 5},
  {3, 10},
  {2, 4},
  {7, 14},
  {12, 6},
  {15, 9},
  {16, 11},
  {5, 13},
  {10, 17},
  {4, 8},
  {7, 1},
  {3, 14},
  {6, 2},
  {12, 15},
  {9, 16},
  {5, 11},
  {13, 4},
  {10, 7},
  {14, 17},
  {1, 6},
  {8, 12},
  {3, 9},
  {2, 5},
  {11, 15},
  {16, 10},
  {4, 1},
  {13, 3},
  {6, 14},
  {12, 7},
  {17, 9},
  {5, 8},
  {10, 15},
  {13, 2},
  {4, 11},
  {3, 6},
  {14, 16},
  {9, 1},
  {17, 12},
  {2, 7},
  {6, 11},
  {8, 15},
  {10, 4},
  {9, 5},
  {16, 12},
  {1, 14},
  {15, 13},
  {11, 17},
  {7, 3},
  {8, 16},
  {14, 2},
  {12, 10},
  {15, 17},
  {13, 9},
  {11, 8}};

#if 0
static PlayerPair pool_18_fencing_pairs[153] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 16},
  {17, 18},
  {3, 1},
  {2, 4},
  {7, 5},
  {6, 8},
  {11, 9},
  {10, 12},
  {15, 13},
  {14, 16},
  {1, 17},
  {18, 3},
  {2, 5},
  {4, 7},
  {6, 9},
  {8, 11},
  {10, 13},
  {12, 15},
  {14, 1},
  {16, 17},
  {3, 2},
  {5, 18},
  {4, 6},
  {9, 7},
  {8, 10},
  {13, 11},
  {12, 1},
  {15, 14},
  {16, 2},
  {17, 3},
  {4, 5},
  {18, 6},
  {7, 10},
  {8, 9},
  {1, 11},
  {12, 13},
  {2, 14},
  {15, 3},
  {16, 4},
  {5, 17},
  {6, 7},
  {10, 18},
  {1, 8},
  {9, 12},
  {11, 2},
  {13, 3},
  {14, 4},
  {5, 15},
  {6, 16},
  {17, 7},
  {10, 1},
  {18, 8},
  {2, 9},
  {3, 12},
  {4, 11},
  {13, 5},
  {14, 6},
  {7, 15},
  {1, 16},
  {17, 10},
  {8, 2},
  {9, 18},
  {11, 3},
  {12, 4},
  {5, 14},
  {6, 13},
  {7, 1},
  {10, 15},
  {16, 8},
  {2, 17},
  {3, 9},
  {18, 11},
  {12, 5},
  {4, 13},
  {14, 7},
  {1, 6},
  {8, 15},
  {16, 10},
  {9, 17},
  {2, 18},
  {3, 5},
  {11, 7},
  {14, 12},
  {4, 1},
  {13, 8},
  {15, 6},
  {9, 16},
  {10, 2},
  {17, 11},
  {7, 18},
  {14, 3},
  {5, 1},
  {8, 12},
  {15, 4},
  {13, 9},
  {6, 2},
  {11, 16},
  {3, 10},
  {17, 14},
  {1, 18},
  {12, 7},
  {5, 8},
  {4, 9},
  {2, 15},
  {16, 13},
  {6, 11},
  {10, 14},
  {7, 3},
  {12, 17},
  {9, 1},
  {18, 4},
  {16, 5},
  {2, 13},
  {8, 14},
  {11, 15},
  {10, 6},
  {3, 16},
  {4, 17},
  {18, 12},
  {7, 2},
  {5, 9},
  {1, 13},
  {11, 14},
  {8, 3},
  {15, 17},
  {10, 4},
  {6, 12},
  {18, 16},
  {13, 7},
  {11, 5},
  {9, 14},
  {1, 15},
  {12, 2},
  {17, 8},
  {3, 6},
  {5, 10},
  {16, 7},
  {13, 18},
  {4, 8},
  {17, 6},
  {15, 9},
  {14, 18},
  {11, 10},
  {12, 16},
  {13, 17},
  {18, 15}};

static PlayerPair pool_19_fencing_pairs[171] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 16},
  {17, 18},
  {19, 1},
  {2, 3},
  {4, 5},
  {6, 7},
  {8, 9},
  {10, 11},
  {12, 13},
  {14, 15},
  {16, 17},
  {18, 1},
  {2, 19},
  {3, 5},
  {4, 6},
  {7, 9},
  {8, 10},
  {11, 13},
  {12, 14},
  {15, 17},
  {1, 16},
  {18, 2},
  {19, 3},
  {5, 7},
  {9, 4},
  {6, 8},
  {10, 13},
  {14, 11},
  {15, 12},
  {17, 1},
  {16, 2},
  {3, 18},
  {5, 19},
  {4, 7},
  {6, 9},
  {13, 8},
  {10, 14},
  {11, 15},
  {1, 12},
  {2, 17},
  {16, 3},
  {18, 5},
  {19, 4},
  {7, 13},
  {10, 6},
  {9, 14},
  {8, 11},
  {1, 15},
  {12, 2},
  {17, 3},
  {5, 16},
  {4, 18},
  {7, 19},
  {13, 6},
  {10, 1},
  {14, 8},
  {9, 11},
  {2, 15},
  {3, 12},
  {17, 5},
  {16, 4},
  {18, 7},
  {6, 19},
  {13, 1},
  {2, 10},
  {8, 15},
  {14, 3},
  {12, 9},
  {11, 5},
  {4, 17},
  {7, 16},
  {6, 18},
  {19, 13},
  {1, 8},
  {15, 10},
  {14, 2},
  {3, 9},
  {5, 12},
  {11, 4},
  {17, 7},
  {16, 6},
  {13, 18},
  {8, 19},
  {1, 14},
  {3, 10},
  {9, 15},
  {2, 5},
  {12, 4},
  {7, 11},
  {6, 17},
  {13, 16},
  {18, 8},
  {19, 14},
  {3, 1},
  {5, 10},
  {9, 2},
  {4, 15},
  {12, 7},
  {11, 6},
  {17, 13},
  {8, 16},
  {14, 18},
  {10, 19},
  {1, 5},
  {15, 3},
  {2, 4},
  {16, 19},
  {13, 9},
  {6, 12},
  {17, 11},
  {7, 14},
  {16, 18},
  {5, 8},
  {15, 19},
  {4, 10},
  {9, 1},
  {13, 3},
  {6, 2},
  {12, 17},
  {11, 16},
  {14, 5},
  {15, 7},
  {18, 19},
  {8, 4},
  {1, 6},
  {10, 12},
  {2, 13},
  {17, 9},
  {3, 11},
  {16, 14},
  {5, 15},
  {7, 1},
  {18, 10},
  {19, 12},
  {8, 2},
  {4, 13},
  {6, 3},
  {9, 16},
  {14, 17},
  {1, 11},
  {15, 18},
  {10, 7},
  {5, 13},
  {12, 8},
  {19, 9},
  {11, 2},
  {4, 14},
  {15, 6},
  {3, 7},
  {16, 10},
  {8, 17},
  {18, 12},
  {4, 1},
  {9, 5},
  {19, 11},
  {13, 15},
  {2, 7},
  {10, 17},
  {18, 9},
  {14, 6},
  {8, 3},
  {12, 16},
  {17, 19},
  {11, 18}};

static PlayerPair pool_20_fencing_pairs[190] =
{ {1, 2},
  {3, 4},
  {5, 6},
  {7, 8},
  {9, 10},
  {11, 12},
  {13, 14},
  {15, 16},
  {17, 18},
  {19, 20},
  {3, 1},
  {2, 4},
  {7, 5},
  {6, 8},
  {11, 9},
  {10, 12},
  {15, 13},
  {14, 16},
  {19, 17},
  {18, 20},
  {4, 1},
  {2, 3},
  {8, 5},
  {6, 7},
  {12, 9},
  {10, 11},
  {16, 13},
  {14, 15},
  {20, 17},
  {18, 19},
  {1, 5},
  {4, 8},
  {6, 2},
  {7, 3},
  {9, 13},
  {12, 16},
  {14, 10},
  {15, 11},
  {17, 1},
  {5, 20},
  {4, 18},
  {8, 19},
  {2, 7},
  {3, 6},
  {16, 9},
  {13, 12},
  {10, 15},
  {11, 14},
  {1, 20},
  {5, 17},
  {19, 4},
  {8, 18},
  {9, 2},
  {16, 7},
  {12, 3},
  {13, 6},
  {1, 10},
  {20, 15},
  {5, 11},
  {17, 14},
  {2, 19},
  {4, 9},
  {18, 7},
  {8, 16},
  {3, 13},
  {6, 12},
  {15, 1},
  {20, 10},
  {14, 5},
  {11, 17},
  {9, 19},
  {18, 2},
  {7, 4},
  {8, 3},
  {16, 6},
  {13, 1},
  {12, 15},
  {10, 5},
  {20, 14},
  {17, 9},
  {19, 11},
  {2, 8},
  {3, 18},
  {4, 6},
  {1, 7},
  {5, 16},
  {10, 13},
  {12, 14},
  {9, 15},
  {11, 20},
  {17, 2},
  {19, 3},
  {1, 8},
  {6, 18},
  {5, 4},
  {7, 10},
  {16, 11},
  {14, 9},
  {13, 20},
  {2, 12},
  {15, 17},
  {19, 1},
  {3, 5},
  {8, 10},
  {6, 11},
  {18, 16},
  {4, 14},
  {9, 7},
  {13, 2},
  {20, 12},
  {15, 19},
  {17, 3},
  {1, 6},
  {5, 18},
  {11, 8},
  {10, 16},
  {7, 14},
  {13, 4},
  {20, 9},
  {2, 15},
  {12, 19},
  {6, 17},
  {18, 1},
  {3, 11},
  {13, 5},
  {14, 8},
  {4, 10},
  {16, 20},
  {7, 15},
  {9, 6},
  {12, 17},
  {11, 2},
  {5, 19},
  {18, 13},
  {14, 1},
  {10, 3},
  {8, 20},
  {16, 4},
  {15, 6},
  {12, 7},
  {9, 5},
  {17, 13},
  {11, 18},
  {2, 14},
  {19, 10},
  {1, 16},
  {20, 3},
  {8, 15},
  {4, 12},
  {7, 13},
  {6, 14},
  {18, 9},
  {5, 2},
  {10, 17},
  {1, 11},
  {16, 19},
  {3, 15},
  {20, 4},
  {12, 8},
  {14, 18},
  {17, 7},
  {6, 10},
  {11, 13},
  {9, 1},
  {2, 16},
  {15, 5},
  {14, 19},
  {4, 17},
  {7, 20},
  {18, 12},
  {3, 9},
  {10, 2},
  {13, 8},
  {19, 6},
  {11, 4},
  {1, 12},
  {16, 17},
  {15, 18},
  {14, 3},
  {20, 2},
  {5, 12},
  {19, 7},
  {8, 9},
  {10, 18},
  {4, 15},
  {6, 20},
  {13, 19},
  {3, 16},
  {7, 11},
  {17, 8}};
#endif

static PlayerPair *fencing_pairs[PoolMatchOrder::_MAX_POOL_SIZE+1] =
{
  NULL,
  NULL,
  pool_2_fencing_pairs,
  pool_3_fencing_pairs,
  pool_4_fencing_pairs,
  pool_5_fencing_pairs,
  pool_6_fencing_pairs,
  pool_7_fencing_pairs,
  pool_8_fencing_pairs,
  pool_9_fencing_pairs,
  pool_10_fencing_pairs,
  pool_11_fencing_pairs,
  pool_12_fencing_pairs,
  pool_13_fencing_pairs,
  pool_14_fencing_pairs,
  pool_15_fencing_pairs,
  pool_16_fencing_pairs,
  pool_17_fencing_pairs
  //pool_18_fencing_pairs,
  //pool_19_fencing_pairs,
  //pool_20_fencing_pairs
};

static PlayerPair *kendo_pairs[PoolMatchOrder::_MAX_POOL_SIZE+1] =
{
  NULL,
  NULL,
  pool_2_fencing_pairs,
  pool_3_kendo_pairs,
  pool_4_fencing_pairs,
  pool_5_fencing_pairs,
  pool_6_fencing_pairs,
  pool_7_fencing_pairs,
  pool_8_fencing_pairs,
  pool_9_fencing_pairs,
  pool_10_fencing_pairs,
  pool_11_fencing_pairs,
  pool_12_fencing_pairs,
  pool_13_fencing_pairs,
  pool_14_fencing_pairs,
  pool_15_fencing_pairs,
  pool_16_fencing_pairs,
  pool_17_fencing_pairs
  //pool_18_fencing_pairs,
  //pool_19_fencing_pairs,
  //pool_20_fencing_pairs
};

// --------------------------------------------------------------------------------
PoolMatchOrder::PoolMatchOrder (gchar weapon_code)
  : Object ("PoolMatchOrder")
{
  _weapon_code  = weapon_code;
  _player_pairs = NULL;
}

// --------------------------------------------------------------------------------
PoolMatchOrder::~PoolMatchOrder ()
{
  Reset ();
}

// --------------------------------------------------------------------------------
void PoolMatchOrder::Reset ()
{
  if (_player_pairs)
  {
    g_slist_foreach (_player_pairs,
                     (GFunc) g_free,
                     NULL);
    g_slist_free (_player_pairs);
    _player_pairs = NULL;
  }
}

// --------------------------------------------------------------------------------
gboolean PoolMatchOrder::GetPlayerPair (guint  match_index,
                                        guint *a_id,
                                        guint *b_id)
{
  PlayerPair *pair = (PlayerPair *) g_slist_nth_data (_player_pairs,
                                                      match_index);

  if (pair)
  {
    *a_id = pair->_a;
    *b_id = pair->_b;
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void PoolMatchOrder::SetAffinityCriteria (AttributeDesc *affinity_criteria,
                                          GSList        *fencer_list)
{
  GSList *affinities = NULL;

  Reset ();

#if 0
  if (affinity_criteria)
  {
    GHashTable *affinity_distribution = g_hash_table_new (NULL,
                                                          NULL);

    // Count affinities
    {
      Player::AttributeId *affinity = Player::AttributeId::Create (affinity_criteria, NULL);
      GSList              *current  = fencer_list;

      for (guint i = 0; current != NULL; i++)
      {
        Player    *fencer = (Player *) current->data;
        Attribute *criteria_attr;

        criteria_attr = fencer->GetAttribute (affinity);
        if (criteria_attr)
        {
          gchar  *user_image = criteria_attr->GetUserImage (AttributeDesc::LONG_TEXT);
          GQuark  quark      = g_quark_from_string (user_image);
          GSList *affinity_list;

          g_free (user_image);
          affinity_list = (GSList *) g_hash_table_lookup (affinity_distribution,
                                                          (const void *) quark);
          affinity_list = g_slist_append (affinity_list,
                                          (void *) i);
          g_hash_table_insert (affinity_distribution,
                               (void *) quark,
                               affinity_list);
        }

        current = g_slist_next (current);
      }
      affinity->Release ();
    }

    // Sort affinities according to their occurence
    {
      GHashTableIter  iter;
      GSList         *affinity_list;

      g_hash_table_iter_init (&iter,
                              affinity_distribution);
      while (g_hash_table_iter_next (&iter,
                                     NULL,
                                     (gpointer *) &affinity_list))
      {
        affinities = g_slist_prepend (affinities,
                                      affinity_list);
      }

      affinities = g_slist_sort (affinities,
                                 (GCompareFunc) CompareAffinityOccurence);
    }

    g_hash_table_destroy (affinity_distribution);
  }
#endif

  // Set the match order according to the affinities
  {
    GSList *current   = affinities;
    guint   nb_fencer = g_slist_length (fencer_list);

    _insertion_step     = 1;
    _insertion_position = 0;
    _nb_pairs_inserted  = 0;

    _player_pairs = GetPlayerPairModel (nb_fencer);
    while (current)
    {
      GSList *affinity_list   = (GSList *) current->data;
      guint   affinity_length = g_slist_length (affinity_list);

      if (   (affinity_length > 1)
          && (affinity_length * 2 <= nb_fencer))
      {
        MovePairsOnHead (affinity_list);
      }
      g_slist_free (affinity_list);

      current = g_slist_next (current);
    }
  }

  // ReorderAdjacents ();

  g_slist_free (affinities);
}

// --------------------------------------------------------------------------------
GSList *PoolMatchOrder::GetPlayerPairModel (guint pool_size)
{
  PlayerPair *pair_table = NULL;
  GSList     *model      = NULL;

  if ((pool_size >= 2) &&  (pool_size <= _MAX_POOL_SIZE))
  {
    if (_weapon_code == 'K')
    {
      pair_table = kendo_pairs[pool_size];
    }
    else
    {
      pair_table = fencing_pairs[pool_size];
    }
  }

  if (pair_table)
  {
    for (guint i = (pool_size * (pool_size - 1)) / 2; i > 0; i--)
    {
      PlayerPair *pair = g_new (PlayerPair, 1);

      *pair = pair_table[i-1];
      model = g_slist_prepend (model,
                               pair);
    }
  }

  return model;
}

// --------------------------------------------------------------------------------
gint PoolMatchOrder::CompareAffinityOccurence (GSList *a,
                                               GSList *b)
{
  return (g_slist_length (b) - g_slist_length (a));
}

// --------------------------------------------------------------------------------
void PoolMatchOrder::MovePairsOnHead (GSList *affinity_list)
{
  guint       nb_fencer        = g_slist_length (affinity_list);
  PlayerPair *translation_grid[nb_fencer][nb_fencer];
  GSList     *a                = affinity_list;

  // Extract pairs having affinities
  for (guint a_list_index = 0; a != NULL; a_list_index++)
  {
    guint   fencer_a_index = GPOINTER_TO_UINT (a->data);
    GSList *b              = g_slist_next (a);

    for (guint b_list_index = a_list_index+1; b != NULL; b_list_index++)
    {
      guint   fencer_b_index = GPOINTER_TO_UINT (b->data);
      GSList *current_pair   = _player_pairs;

      while (current_pair)
      {
        PlayerPair *pair = (PlayerPair *) current_pair->data;

        if (   ((pair->_a == fencer_a_index+1) && (pair->_b == fencer_b_index+1))
            || ((pair->_a == fencer_b_index+1) && (pair->_b == fencer_a_index+1)))
        {
          _player_pairs = g_slist_remove_link (_player_pairs,
                                               current_pair);

          // Temporarily store the pair in a fake pool grid.
          translation_grid[a_list_index][b_list_index] = pair;
          translation_grid[b_list_index][a_list_index] = pair;
          break;
        }
        current_pair = g_slist_next (current_pair);
      }
      b = g_slist_next (b);
    }
    a = g_slist_next (a);
  }

  // Inject extracted pairs on the head of the list
  // with the relevant order given by the new pair sub model
  {
    GSList *sub_model = GetPlayerPairModel (nb_fencer);
    GSList *current   = sub_model;

    while (current)
    {
      PlayerPair *sub_model_pair = (PlayerPair *) current->data;
      PlayerPair *extracted_pair = translation_grid[sub_model_pair->_a-1][sub_model_pair->_b-1];

      if (_insertion_position > _nb_pairs_inserted)
      {
        _insertion_step++;
        _insertion_position = 1;
      }
      _player_pairs = g_slist_insert (_player_pairs,
                                      extracted_pair,
                                      _insertion_position);
      _nb_pairs_inserted++;
      _insertion_position += _insertion_step;

      current = g_slist_next (current);
    }

    if (_insertion_position == _nb_pairs_inserted)
    {
      _insertion_position += _insertion_step;
    }

    g_slist_foreach (sub_model,
                     (GFunc) g_free,
                     NULL);
    g_slist_free (sub_model);
  }
}

// --------------------------------------------------------------------------------
void PoolMatchOrder::ReorderAdjacents ()
{
  if (_nb_pairs_inserted)
  {
    GSList *previous   = g_slist_nth (_player_pairs,
                                      _nb_pairs_inserted-1);
    GSList *current    = g_slist_next (previous);
    GSList *remainging = NULL;

    while (current)
    {
      PlayerPair *previous_pair = (PlayerPair *) previous->data;
      PlayerPair *current_pair  = (PlayerPair *) current->data;

      if (   (current_pair->_a == previous_pair->_a)
          || (current_pair->_a == previous_pair->_b)
          || (current_pair->_b == previous_pair->_a)
          || (current_pair->_b == previous_pair->_b))
      {
        GSList *substitute = g_slist_next (current);

        if (substitute == NULL)
        {
          remainging = current;
        }

        while (substitute)
        {
          PlayerPair *substitute_pair = (PlayerPair *) substitute->data;

          if (   (substitute_pair->_a != previous_pair->_a)
              && (substitute_pair->_a != previous_pair->_b)
              && (substitute_pair->_b != previous_pair->_a)
              && (substitute_pair->_b != previous_pair->_b))
          {
            // Following lines are safe because
            // _player_pairs can't be modified in our case
            _player_pairs = g_slist_remove_link (_player_pairs,
                                                 substitute);
            _player_pairs = g_slist_insert_before (_player_pairs,
                                                   current,
                                                   substitute_pair);
            break;
          }

          substitute = g_slist_next (substitute);
        }
      }

      previous = current;
      current  = g_slist_next (previous);
    }

    if (remainging)
    {
      PlayerPair *remainging_pair = (PlayerPair *) remainging->data;
      GSList     *previous        = g_slist_nth (_player_pairs,
                                                 _nb_pairs_inserted-1);
      GSList     *next            = g_slist_next (previous);

      printf ("*** %d - %d\n", remainging_pair->_a, remainging_pair->_b);
      while (previous && (previous != remainging) && (next != remainging))
      {
        PlayerPair *previous_pair = (PlayerPair *) previous->data;
        PlayerPair *next_pair     = (PlayerPair *) next->data;

        if (   (remainging_pair->_a != previous_pair->_a)
            && (remainging_pair->_a != previous_pair->_b)
            && (remainging_pair->_b != previous_pair->_a)
            && (remainging_pair->_b != previous_pair->_b)
            && (remainging_pair->_a != next_pair->_a)
            && (remainging_pair->_a != next_pair->_b)
            && (remainging_pair->_b != next_pair->_a)
            && (remainging_pair->_b != next_pair->_b))
        {
          printf ("    %d - %d\n", previous_pair->_a, previous_pair->_b);

          _player_pairs = g_slist_remove_link (_player_pairs,
                                               remainging);
          _player_pairs = g_slist_insert_before (_player_pairs,
                                                 next,
                                                 remainging_pair);

          break;
        }

        previous = g_slist_next (previous);
        next     = g_slist_next (previous);
      }
    }
  }
}
