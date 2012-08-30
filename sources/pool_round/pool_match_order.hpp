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

#ifndef pool_match_order_hpp
#define pool_match_order_hpp

#include <gtk/gtk.h>
#include "object.hpp"
#include "attribute.hpp"

class PoolMatchOrder : public Object
{
  public:
    static const guint _MAX_POOL_SIZE = 17;

  public:
    PoolMatchOrder (gchar weapon_code);

    gboolean GetPlayerPair (guint  match_index,
                            guint *a_id,
                            guint *b_id);

    void SetAffinityCriteria (AttributeDesc *affinity_criteria,
                              GSList        *fencer_list);

  private:
    gchar   _weapon_code;
    GSList *_player_pairs;

    virtual ~PoolMatchOrder ();

    void Reset ();

    GSList *GetPlayerPairModel (guint pool_size);

    void MovePairsOnHead (GSList *affinity_list);

    static gint CompareAffinityOccurence (GSList *a,
                                          GSList *b);
};

#endif
