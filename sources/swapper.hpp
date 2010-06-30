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

#ifndef swapper_hpp
#define swapper_hpp

#include <gtk/gtk.h>

#include "object.hpp"
#include "attribute.hpp"

class Swapper : public Object
{
  public:
    struct ValueUsage : Object
    {
      struct PoolUsage
      {
        guint _nb_max;
        guint _nb;
      };

      ValueUsage (gchar *image,
                 guint  nb_pool);

      ~ValueUsage ();

      gchar     *_image;
      guint      _nb_similar;
      guint      _max1_by_pool;
      guint      _max2_by_pool;
      guint      _nb_pool_with_max1;
      guint      _nb_pool_with_max2;
      PoolUsage *_pool_usage;
    };

    Swapper (Object *owner);

    void SetCriteria (Player::AttributeId *criteria_id);

    void SetPlayerList (GSList *player_list);

    void Swap (GSList *pools);

  private:
    Player::AttributeId *_criteria_id;
    GSList              *_player_list;
    GArray              *_array;
    Object              *_owner;
    GSList              *_pools;

    ~Swapper ();

    void Update ();

    void FreeArray ();

    ValueUsage *GetValueUsage (gchar *value_image);

    gboolean Swap (Player *player,
                   Pool   *to_pool);
};

#endif
