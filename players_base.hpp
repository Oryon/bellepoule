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

#ifndef playersbase_hpp
#define playersbase_hpp

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>

#include "object.hpp"
#include "glade.hpp"
#include "player.hpp"

class PlayersBase_c : public Object_c
{
  public:
    typedef gboolean (*CustomFilter) (Player_c *player);

  public:
     PlayersBase_c ();
    ~PlayersBase_c ();

    GtkTreeModel *GetTreeModel      ();
    void          EnterPlayers      ();
    void          RemoveSelection   (GtkTreeSelection *selection);
    guint         GetNbPlayers      ();
    Player_c     *GetPlayerFromRef  (guint ref);
    Player_c     *GetPlayer         (guint index);
    Player_c     *GetPlayer         (const gchar *path_string);
    void          Update            (Player_c *player);
    void          Save              (xmlTextWriter *xml_writer);
    void          Load              (xmlDoc        *doc);
    void          Lock              ();
    void          Add               (Player_c *player);
    GSList       *CreateCustomList  (CustomFilter filter);

  public:
    void on_close_button_clicked ();
    void on_add_button_clicked   ();

  private:
    Glade_c      *_glade;
    GtkListStore *_store;
    GSList       *_player_list;

    GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter *iter);
    Player_c            *GetPlayer       (GtkTreePath *path);

};

#endif
