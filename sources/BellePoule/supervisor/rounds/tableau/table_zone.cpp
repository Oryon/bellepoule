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

#include "util/global.hpp"
#include "util/canvas.hpp"
#include "util/canvas_module.hpp"
#include "util/player.hpp"
#include "util/fie_time.hpp"
#include "../../match.hpp"

#include "table_zone.hpp"

namespace Table
{
  // --------------------------------------------------------------------------------
  TableZone::TableZone ()
    : Object ("TableZone")
  {
    _node = NULL;
  }

  // --------------------------------------------------------------------------------
  TableZone::~TableZone ()
  {
  }

  // --------------------------------------------------------------------------------
  void TableZone::AddNode (GNode *node)
  {
    _node = node;
  }

  // --------------------------------------------------------------------------------
  void TableZone::PutInTable (CanvasModule  *canvas_module,
                              GooCanvasItem *table,
                              guint          row,
                              guint          column)
  {
    if (_node)
    {
      NodeData *data = (NodeData *) _node->data;

      if (data->_match)
      {
        GSList  *current    = data->_match->GetRefereeList ();
        guint    piste      = data->_match->GetPiste ();
        FieTime *start_time = data->_match->GetStartTime ();

        if (current && piste && start_time)
        {
          {
            gchar *roadmap = g_strdup_printf ("%s %d @ %s",
                                              gettext ("Piste"),
                                              piste,
                                              start_time->GetImage ());

            GooCanvasItem *item = Canvas::PutTextInTable (table,
                                                          roadmap,
                                                          row,
                                                          column);
            Canvas::SetTableItemAttribute (item, "x-align", 1.0);
            Canvas::SetTableItemAttribute (item, "y-align", 0.5);
            g_object_set (item,
                          "font", BP_FONT "Bold Italic 12px",
                          NULL);

            g_free (roadmap);
          }

          for (guint i = 1; current != NULL; i++)
          {
            Player *referee = (Player *) current->data;

            {
              static gchar *referee_icon = NULL;

              if (referee_icon == NULL)
              {
                referee_icon = g_build_filename (Global::_share_dir, "resources/glade/images/referee.png", NULL);
              }

              Canvas::PutIconInTable (table,
                                      referee_icon,
                                      row + i,
                                      column);
            }

            {
              gchar *name = referee->GetName ();

              GooCanvasItem *item = Canvas::PutTextInTable (table,
                                                            name,
                                                            row + i,
                                                            column+1);
              Canvas::SetTableItemAttribute (item, "x-align", 1.0);
              Canvas::SetTableItemAttribute (item, "y-align", 0.5);
              g_object_set (item,
                            "font", BP_FONT "Bold Italic 12px",
                            NULL);

              g_free (name);
            }

            current = g_slist_next (current);
          }

          return;
        }
      }
    }

    // Goocanvas display issue workaround
    {
      Canvas::PutTextInTable (table,
                              "",
                              row,
                              column+1);
    }
  }
}
