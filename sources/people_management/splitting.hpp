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

#ifndef splitting_hpp
#define splitting_hpp

#include <gtk/gtk.h>

#include "stage.hpp"
#include "players_list.hpp"

class Tournament;
class Contest;

namespace People
{
  class Splitting : public virtual Stage, public PlayersList
  {
    public:
      static void Declare ();

      static void SetHostTournament (Tournament *tournament);

      Splitting (StageClass *stage_class);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      void OnLocked ();

      void OnUnLocked ();

      void Display ();

      void Wipe ();

      GSList *GetCurrentClassification ();

      void Save (xmlTextWriter *xml_writer);

      void Load (xmlNode *xml_node);

    private:
      static Tournament *_tournament;

      static Stage *CreateInstance (StageClass *stage_class);

      static gboolean PresentPlayerFilter (Player *player);

      virtual ~Splitting ();

      void Garnish ();

      GSList *GetOutputShortlist ();
  };
}

#endif
