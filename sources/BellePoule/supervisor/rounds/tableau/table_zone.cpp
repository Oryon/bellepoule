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
#include "util/attribute.hpp"
#include "../../match.hpp"

#include "table_zone.hpp"

namespace Table
{
  // --------------------------------------------------------------------------------
  TableZone::TableZone ()
    : Object ("TableZone")
  {
    _node = nullptr;
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
  void TableZone::PutRoadmapInTable (CanvasModule  *canvas_module,
                                     GooCanvasItem *table,
                                     guint          row,
                                     guint          column)
  {
    if (_node)
    {
      NodeData *data = (NodeData *) _node->data;

      if (data->_match)
      {
        guint    piste      = data->_match->GetPiste ();
        FieTime *start_time = data->_match->GetStartTime ();

        if (piste && start_time)
        {
          {
            gchar *roadmap = g_strdup_printf ("%s %d%c%c@%c%c%s",
                                              gettext ("Piste"),
                                              piste,
                                              0xC2, 0xA0, // non breaking space
                                              0xC2, 0xA0, // non breaking space
                                              start_time->GetImage ());
            gchar *undivadable = GetUndivadableText (roadmap);

            GooCanvasItem *item = Canvas::PutTextInTable (table,
                                                          undivadable,
                                                          row,
                                                          column);
            Canvas::SetTableItemAttribute (item, "top-padding",   5.0);
            Canvas::SetTableItemAttribute (item, "right-padding", 2.0);
            Canvas::SetTableItemAttribute (item, "x-align", 1.0);
            Canvas::SetTableItemAttribute (item, "y-align", 0.5);
            g_object_set (item,
                          "font",       BP_FONT "Bold Italic 12px",
                          "fill-color", "DarkGreen",
                          NULL);

            g_free (undivadable);
            g_free (roadmap);
          }

          {
            GSList        *current_referee = data->_match->GetRefereeList ();
            GooCanvasItem *referee_table   = goo_canvas_table_new (table,
                                                                   NULL);
            Canvas::PutInTable (table,
                                referee_table,
                                row + 1,
                                column);
            Canvas::SetTableItemAttribute (referee_table, "right-padding", 2.0);

            for (guint i = 0; current_referee != nullptr; i++)
            {
              Player *referee = (Player *) current_referee->data;

              {
                static gchar *referee_icon = nullptr;

                if (referee_icon == nullptr)
                {
                  referee_icon = g_build_filename (Global::_share_dir, "resources/glade/images/referee.png", NULL);
                }

                Canvas::PutIconInTable (referee_table,
                                        referee_icon,
                                        i,
                                        0);
              }

              {
                GooCanvasItem *item;
                gchar         *name      = referee->GetName ();
                GString       *full_name = g_string_new (name);
                Player::AttributeId  first_name_attr_id ("first_name");
                Attribute *first_name  = referee->GetAttribute (&first_name_attr_id);

                if (first_name)
                {
                  gchar *image = first_name->GetUserImage (AttributeDesc::LONG_TEXT);

                  g_string_append_c (full_name, ' ');
                  g_string_append   (full_name, image);
                  g_free (image);
                }

                {
                  gchar *undivadable = GetUndivadableText (full_name->str);

                  item = Canvas::PutTextInTable (referee_table,
                                                 undivadable,
                                                 i,
                                                 1);
                  g_free (undivadable);
                }

                Canvas::SetTableItemAttribute (item, "x-align", 1.0);
                Canvas::SetTableItemAttribute (item, "y-align", 0.5);
                g_object_set (item,
                              "font",       BP_FONT "12px",
                              "fill-color", "DarkGreen",
                              NULL);

                g_string_free (full_name,
                               TRUE);
                g_free (name);
              }

              current_referee = g_slist_next (current_referee);
            }
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
