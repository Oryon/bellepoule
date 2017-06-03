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

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "util/player.hpp"
#include "actors/player_factory.hpp"

#include "competition.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Competition::Competition (guint            id,
                            Batch::Listener *listener)
    : Object ("Competition"),
    Module ("competition.glade", "batch_viewport")
  {
    _fencer_list = NULL;
    _batches     = NULL;
    _weapon      = NULL;
    _id          = id;
    _gdk_color   = g_new (GdkColor, 1);

    _batch_listener = listener;

    g_datalist_init (&_properties);
  }

  // --------------------------------------------------------------------------------
  Competition::~Competition ()
  {
    gdk_color_free (_gdk_color);
    g_datalist_clear (&_properties);

    FreeFullGList (Player, _fencer_list);
    FreeFullGList (Batch,  _batches);

    g_free (_weapon);
  }

  // --------------------------------------------------------------------------------
  guint Competition::GetId ()
  {
    return _id;
  }

  // --------------------------------------------------------------------------------
  GData *Competition::GetProperties ()
  {
    return _properties;
  }

  // --------------------------------------------------------------------------------
  GdkColor *Competition::GetColor ()
  {
    return _gdk_color;
  }

  // --------------------------------------------------------------------------------
  void Competition::SetProperty (Net::Message *message,
                                 const gchar  *property)
  {
    gchar         *property_widget = g_strdup_printf ("competition_%s_label", property);
    GtkLabel      *label           = GTK_LABEL (_glade->GetGObject (property_widget));
    AttributeDesc *desc            = AttributeDesc::GetDescFromCodeName (property);
    gchar         *xml             = message->GetString (property);
    gchar         *image           = desc->GetUserImage (xml, AttributeDesc::LONG_TEXT);

    g_datalist_set_data_full (&_properties,
                              property,
                              image,
                              g_free);
    gtk_label_set_text (label,
                        gettext (image));

    g_free (property_widget);
    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  void Competition::SetProperties (Net::Message *message)
  {
    SetProperty (message, "gender");
    SetProperty (message, "weapon");
    SetProperty (message, "category");

    _weapon = message->GetString ("weapon");

    {
      GtkWidget *tab   = _glade->GetWidget ("notebook_title");
      gchar     *color = message->GetString ("color");

      gdk_color_parse (color,
                       _gdk_color);

      gtk_widget_modify_bg (tab,
                            GTK_STATE_NORMAL,
                            _gdk_color);
      gtk_widget_modify_bg (tab,
                            GTK_STATE_ACTIVE,
                            _gdk_color);

      g_free (color);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Competition::GetWeaponCode ()
  {
    return _weapon;
  }

  // --------------------------------------------------------------------------------
  void Competition::ManageBatch (Net::Message *message)
  {
    Batch *batch = new Batch (message->GetNetID (),
                              this,
                              _batch_listener);

    _batches = g_list_append (_batches,
                              batch);

    Plug (batch,
          _glade->GetWidget ("batch_viewport"));
  }

  // --------------------------------------------------------------------------------
  void Competition::DeleteBatch (Net::Message *message)
  {
    Batch *batch = GetBatch (message->GetNetID ());

    if (batch)
    {
      GList *node = g_list_find (_batches,
                                 batch);

      if (node)
      {
        _batches = g_list_delete_link (_batches,
                                       node);
      }
      batch->UnPlug ();
      batch->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  Batch *Competition::GetBatch (guint id)
  {
    GList *current = _batches;

    while (current)
    {
      Batch *batch = (Batch *) current->data;

      if (batch->GetId () == id)
      {
        return batch;
      }

      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  GList *Competition::GetBatches ()
  {
    return _batches;
  }

  // --------------------------------------------------------------------------------
  void Competition::AttachTo (GtkNotebook *to)
  {
    GtkWidget *root = GetRootWidget ();

    g_object_set_data (G_OBJECT (root),
                       "competition",
                       this);
    gtk_notebook_append_page (to,
                              root,
                              _glade->GetWidget ("notebook_title"));
  }

  // --------------------------------------------------------------------------------
  void Competition::ManageFencer (Net::Message *message)
  {
    gchar     *xml = message->GetString ("xml");
    xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

    if (doc)
    {
      xmlXPathInit ();

      {
        xmlXPathContext *xml_context = xmlXPathNewContext (doc);
        xmlXPathObject  *xml_object;
        xmlNodeSet      *xml_nodeset;

        xml_object = xmlXPathEval (BAD_CAST "/Tireur", xml_context);
        xml_nodeset = xml_object->nodesetval;

        if (xml_nodeset->nodeNr == 1)
        {
          Player *fencer = PlayerFactory::CreatePlayer ("Fencer");

          fencer->Load (xml_nodeset->nodeTab[0]);

          fencer->SetData (this,
                           "netid",
                           GUINT_TO_POINTER (message->GetNetID ()));

          _fencer_list = g_list_prepend (_fencer_list,
                                         fencer);
        }

        xmlXPathFreeObject  (xml_object);
        xmlXPathFreeContext (xml_context);
      }
      xmlFreeDoc (doc);
    }

    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  void Competition::DeleteFencer (Net::Message *message)
  {
    guint  id      = message->GetNetID ();
    GList *current = _fencer_list;

    while (current)
    {
      Player *fencer     = (Player *) current->data;
      guint   current_id = fencer->GetIntData (this, "netid");

      if (current_id == id)
      {
        break;
      }

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  Player *Competition::GetFencer (guint ref)
  {
    GList *current = _fencer_list;

    while (current)
    {
      Player *fencer = (Player *) current->data;

      if (fencer->GetRef () == ref)
      {
        return fencer;
      }

      current = g_list_next (current);
    }

    return NULL;
  }
}
