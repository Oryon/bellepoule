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

#ifndef green_swapper_hpp
#define green_swapper_hpp

#include <gtk/gtk.h>

#include "object.hpp"
#include "swapper.hpp"
#include "player.hpp"
#include "attribute.hpp"

class Pool;

class GreenSwapper : public Object, public Swapper
{
  public:
    static Swapper *Create (GSList *pools,
                            gchar  *criteria,
                            GSList *player_list);

    GreenSwapper (GSList *pools,
                  gchar  *criteria,
                  GSList *player_list);

    void Delete ();

    gboolean Swap (Player *player,
                   Pool   *to_pool);

    virtual Player *GetNextPlayer (Pool *for_pool);

  private:
    struct ValueUsage : Object
  {
    ValueUsage (gchar *image);

    ~ValueUsage ();

    gchar *_image;
    guint  _nb_similar;
    guint  _max_by_pool;
    guint  _nb_asymetric_pools;
  };

    Player::AttributeId *_criteria_id;
    GSList              *_player_list;
    GArray              *_array;
    GSList              *_pools;
    GSList              *_players_list;

    ~GreenSwapper ();

    void Update ();

    void FreeArray ();

    ValueUsage *GetValueUsage (gchar *value_image);

    gboolean CanSwap (ValueUsage *value,
                      guint       from_pool,
                      guint       to_pool);
};

#endif
