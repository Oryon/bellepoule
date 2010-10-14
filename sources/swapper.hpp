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
    Swapper (Object *owner);

    void SetCriteria (Player::AttributeId *criteria_id);

    void SetPlayerList (GSList *player_list);

    void Swap (GSList  *pools,
               guint32  rand_seed);

  private:
    struct ValueUsage : Object
    {
      ValueUsage (gchar *image,
                 guint  nb_pool);

      ~ValueUsage ();

      gchar *_image;
      guint  _nb_similar;
      guint  _max_by_pool;
      guint  _nb_asymetric_pool;
      guint *_in_pool;
    };

    Player::AttributeId *_criteria_id;
    GSList              *_player_list;
    GArray              *_array;
    Object              *_owner;
    GSList              *_pools;

    ~Swapper ();

    void Update ();

    void FreeArray ();

    ValueUsage *GetValueUsage (gchar *value_image);

    gboolean CanSwap (ValueUsage *value,
                      guint       from_pool,
                      guint       to_pool);

    gboolean Swap (Player *player,
                   Pool   *to_pool);
};

#endif
