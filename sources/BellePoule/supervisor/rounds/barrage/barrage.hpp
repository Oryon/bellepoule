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

#include "util/module.hpp"
#include "actors/players_list.hpp"
#include "../../stage.hpp"

namespace People
{
  class Barrage : public Stage, public PlayersList
  {
    public:
      static void Declare ();

      Barrage (StageClass *stage_class);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      void Display () override;

      void OnPlugged () override;

      void Garnish () override;

      GSList *GetCurrentClassification () override;

      void Load (xmlNode *xml_node) override;

    private:
      guint _ties_count;
      guint _short_list_length;
      guint _loaded_so_far;
      guint _promoted_count;

      static Stage *CreateInstance (StageClass *stage_class);

      void RefreshPromotedDisplay ();

      static void OnAttrPromotedChanged (Player    *player,
                                         Attribute *attr,
                                         Object    *object,
                                         guint      step);

      ~Barrage () override;
  };
}
