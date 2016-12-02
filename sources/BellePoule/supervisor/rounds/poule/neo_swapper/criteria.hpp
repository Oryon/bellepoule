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

#include "util/object.hpp"
#include "util/attribute.hpp"
#include "util/player.hpp"

#include "fencer_proxy.hpp"

namespace NeoSwapper
{
  class Criteria : public Object
  {
    public:
      Criteria (GQuark  quark,
                guint   pool_count,
                guint   depth,
                Object *owner);

      void Use (FencerProxy *fencer);

      GList *GetErrors (GList *errors);

      GQuark GetQuark ();

      gboolean HasQuark (GQuark quark);

      void ChangeScore (gint       delta,
                        PoolProxy *for_pool);

      gboolean FreePlaceIn (PoolProxy *pool);

      static void Dump (GQuark         quark,
                        Criteria *criteria_value);

    private:
      struct Profile
      {
        guint _score;
        guint _pool_count;
      };

      Object      *_owner;
      GQuark       _quark;
      const gchar *_image;
      guint        _depth;
      guint        _pool_count;
      GSList      *_fencer_list;
      guint        _fencer_count;
      Profile      _big_profile;
      Profile      _small_profile;

    private:
      guint      *_score_table;
      PoolProxy **_pool_table;
      GList      *_errors;

      ~Criteria ();

      void UpdateProfile ();
  };
}
