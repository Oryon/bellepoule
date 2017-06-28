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

#include "util/global.hpp"
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
    _fencer_list   = NULL;
    _batches       = NULL;
    _weapon        = NULL;
    _id            = id;
    _gdk_color     = g_new (GdkColor, 1);
    _current_batch = NULL;

    _batch_listener = listener;

    g_datalist_init (&_properties);

    {
      gchar              *gif       = g_build_filename (Global::_share_dir, "resources", "glade", "images", "apply.gif", NULL);
      GdkPixbufAnimation *animation = gdk_pixbuf_animation_new_from_file (gif, NULL);

      gtk_image_set_from_animation (GTK_IMAGE (_glade->GetWidget ("apply_animation")),
                                    animation);

      g_free (gif);
      g_object_unref (animation);
    }

    _batch_image   = _glade->GetWidget ("batch_image");
    _spread_button = _glade->GetWidget ("spread_button");

    gtk_widget_set_visible (_batch_image,   FALSE);
    gtk_widget_set_visible (_spread_button, FALSE);
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
  gboolean Competition::BatchIsModifialble (Batch *batch)
  {
    return (batch == _current_batch);
  }

  // --------------------------------------------------------------------------------
  void Competition::SetProperties (Net::Message *message)
  {
    SetProperty (message, "gender");
    SetProperty (message, "weapon");
    SetProperty (message, "category");

    _weapon = message->GetString ("weapon");

    {
      GtkWidget *tab   = _glade->GetWidget ("color_box");
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
  Batch *Competition::ManageBatch (Net::Message *message)
  {
    Batch *batch = GetBatch (message->GetNetID ());

    if (batch == NULL)
    {
      gchar *round_name = message->GetString ("name");

      batch = new Batch (message->GetNetID (),
                         this,
                         round_name,
                         _batch_listener);

      g_free (round_name);

      _batches = g_list_append (_batches,
                                batch);
    }

    if (message->GetInteger ("done"))
    {
      if (_current_batch == batch)
      {
        SetCurrentBatch (NULL);
      }
    }
    else
    {
      SetCurrentBatch (batch);
    }

    return batch;
  }

  // --------------------------------------------------------------------------------
  void Competition::SetCurrentBatch (Batch *batch)
  {
    if (_current_batch)
    {
      _current_batch->UnPlug ();
    }

    _current_batch = batch;

    if (_current_batch)
    {
      Plug (_current_batch,
            _glade->GetWidget ("batch_viewport"));
    }
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

      if (_current_batch == batch)
      {
        SetCurrentBatch (NULL);
      }

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

  // --------------------------------------------------------------------------------
  void Competition::OnSpread ()
  {
    if (_current_batch)
    {
      _current_batch->OnValidateAssign ();
    }
  }

  // --------------------------------------------------------------------------------
  void Competition::SetBatchStatus (Batch         *batch,
                                    Batch::Status  status)
  {
    if (batch == _current_batch)
    {
      if (status == Batch::UNCOMPLETED)
      {
        gtk_widget_set_visible (_batch_image,
                                TRUE);
        gtk_widget_set_visible (_spread_button,
                                FALSE);
      }
      else if (status == Batch::CONCEALED)
      {
        gtk_widget_set_visible (_batch_image,
                                FALSE);
        gtk_widget_set_visible (_spread_button,
                                TRUE);
      }
      else if (status == Batch::DISCLOSED)
      {
        gtk_widget_set_visible (_batch_image,
                                FALSE);
        gtk_widget_set_visible (_spread_button,
                                FALSE);
      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_spread_button_clicked (GtkToolButton *widget,
                                                            Object        *owner)
  {
    Competition *c = dynamic_cast <Competition *> (owner);

    c->OnSpread ();
  }
}
