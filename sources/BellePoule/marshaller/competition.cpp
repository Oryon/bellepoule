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
#include "util/glade.hpp"
#include "util/attribute_desc.hpp"
#include "actors/player_factory.hpp"
#include "actors/fencer.hpp"
#include "actors/team.hpp"
#include "affinities.hpp"
#include "batch_panel.hpp"
#include "batch.hpp"

#include "competition.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Competition::Competition (guint            id,
                            Batch::Listener *listener)
    : Object ("Competition"),
    Module ("competition.glade")
  {
    _fencer_list   = nullptr;
    _batches       = nullptr;
    _weapon        = nullptr;
    _current_batch = nullptr;
    _id            = id;
    _gdk_color     = g_new (GdkColor, 1);
    _radio_group   = GTK_RADIO_BUTTON (_glade->GetWidget ("batch_group"));

    _batch_listener = listener;

    g_datalist_init (&_properties);

    {
      gchar              *gif       = g_build_filename (Global::_share_dir, "resources", "glade", "images", "apply.gif", NULL);
      GdkPixbufAnimation *animation = gdk_pixbuf_animation_new_from_file (gif, nullptr);

      gtk_image_set_from_animation (GTK_IMAGE (_glade->GetWidget ("apply_animation")),
                                    animation);

      g_free (gif);
      g_object_unref (animation);
    }

    _batch_image = _glade->GetWidget ("batch_image");
    gtk_widget_set_visible (_batch_image, FALSE);

    _batch_table = GTK_TABLE (_glade->GetWidget ("batch_table"));
  }

  // --------------------------------------------------------------------------------
  Competition::~Competition ()
  {
    gdk_color_free (_gdk_color);
    g_datalist_clear (&_properties);

    FreeFullGList (Player, _fencer_list);

    FreeFullGList (Batch, _batches);

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
  gboolean Competition::BatchIsModifiable (Batch *batch)
  {
    BatchPanel *panel = GetBatchPanel (batch);

    return panel->IsVisible ();
  }

  // --------------------------------------------------------------------------------
  void Competition::Freeze ()
  {
    gtk_widget_set_visible (_glade->GetWidget ("batch_table"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            FALSE);
  }

  // --------------------------------------------------------------------------------
  void Competition::UnFreeze ()
  {
    gtk_widget_set_visible (_glade->GetWidget ("batch_table"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            TRUE);
  }

  // --------------------------------------------------------------------------------
  void Competition::SetProperties (Net::Message *message)
  {
    UnFreeze ();

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

    if (batch == nullptr)
    {
      batch = new Batch (message,
                         this,
                         _batch_listener);

      {
        BatchPanel *panel = new BatchPanel (_batch_table,
                                            _radio_group,
                                            batch,
                                            this);

        batch->SetData (this,
                        "batch_panel",
                        panel);
      }

      _batches = g_list_append (_batches,
                                batch);

      Plug (batch,
            _glade->GetWidget ("batch_vbox"));

      gtk_widget_hide (batch->GetRootWidget ());
    }

    if (message->GetInteger ("done"))
    {
      MaskBatch (batch);
    }
    else
    {
      ExposeBatch (batch,
                   message->GetInteger ("expected_jobs"),
                   message->GetInteger ("ready_jobs"));
    }

    return batch;
  }

  // --------------------------------------------------------------------------------
  void Competition::ExposeBatch (Batch *batch,
                                 guint  expected_jobs,
                                 guint  ready_jobs)
  {
    BatchPanel *panel = GetBatchPanel (batch);

    panel->Expose (expected_jobs,
                   ready_jobs);

    if (_current_batch == nullptr)
    {
      panel->SetActive ();
    }

    UnFreeze ();
  }

  // --------------------------------------------------------------------------------
  void Competition::MaskBatch (Batch *batch)
  {
    {
      BatchPanel *panel = GetBatchPanel (batch);

      panel->Mask ();
    }

    if (_current_batch == batch)
    {
      _current_batch = nullptr;

      for (GList *current = g_list_last (_batches); current; current = g_list_previous (current))
      {
        Batch      *current_batch = (Batch *) current->data;
        BatchPanel *panel         = GetBatchPanel (current_batch);

        if (panel->IsVisible ())
        {
          panel->SetActive ();
          return;
        }
      }

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_radio_group),
                                    TRUE);
      Freeze ();
    }
  }

  // --------------------------------------------------------------------------------
  void Competition::DeleteBatch (Net::Message *message)
  {
    Batch      *batch = GetBatch (message->GetNetID ());
    BatchPanel *panel = GetBatchPanel (batch);

    MaskBatch (batch);
    panel->Release ();

    _batches = g_list_remove (_batches,
                              batch);

    batch->UnPlug ();
    batch->Release ();
  }

  // --------------------------------------------------------------------------------
  BatchPanel *Competition::GetBatchPanel (Batch *batch)
  {
    return (BatchPanel *) batch->GetPtrData (this,
                                             "batch_panel");
  }

  // --------------------------------------------------------------------------------
  Batch *Competition::GetBatch (guint id)
  {
    for (GList *current = _batches; current; current = g_list_next (current))
    {
      Batch *batch = (Batch *) current->data;

      if (batch->GetId () == id)
      {
        return batch;
      }
    }

    return nullptr;
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
    gtk_notebook_set_tab_reorderable (to,
                                      GetRootWidget (),
                                      TRUE);
  }

  // --------------------------------------------------------------------------------
  void Competition::ManageFencer (Net::Message *message)
  {
    gchar     *xml = message->GetString ("xml");
    xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

    if (doc)
    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);

      if (LoadFencer (xml_context,
                      message,
                      "/Tireur",
                      "Fencer") == FALSE)
      {
        LoadFencer (xml_context,
                    message,
                    "/Equipe",
                    "Team");
      }

      xmlXPathFreeContext (xml_context);
      xmlFreeDoc (doc);
    }

    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  gboolean Competition::LoadFencer (xmlXPathContext *xml_context,
                                    Net::Message    *message,
                                    const gchar     *path,
                                    const gchar     *code_name)
  {
    gboolean result = FALSE;

    xmlXPathInit ();

    {
      xmlXPathObject *xml_object = xmlXPathEval (BAD_CAST path, xml_context);
      xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

      if (xml_nodeset->nodeNr == 1)
      {
        xmlNode *xml_node = xml_nodeset->nodeTab[0];
        Player  *player   = LoadNode (xml_node,
                                      message,
                                      code_name);

        if (player && player->Is ("Team"))
        {
          Team       *team            = (Team *) player;
          Affinities *team_affinities = (Affinities *) team->GetPtrData (nullptr, "affinities");

          for (xmlNode *child = xml_node->children; child; child = child->next)
          {
            Fencer *fencer = (Fencer *) PlayerFactory::CreatePlayer ("Fencer");

            fencer->Load (child);

            fencer->SetTeam (team);

            {
              Affinities *affinities = new Affinities (fencer);

              fencer->SetData (nullptr,
                               "affinities",
                               affinities,
                               (GDestroyNotify) Affinities::Destroy);
              affinities->ShareWith (team_affinities);
            }
          }
        }

        result = TRUE;
      }

      xmlXPathFreeObject  (xml_object);
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  Player *Competition::LoadNode (xmlNode      *xml_node,
                                 Net::Message *message,
                                 const gchar  *code_name)
  {
    Player *player = PlayerFactory::CreatePlayer (code_name);

    player->Load (xml_node);

    player->SetData (this,
                     "netid",
                     GUINT_TO_POINTER (message->GetNetID ()));

    player->SetData (nullptr,
                     "affinities",
                     new Affinities (player),
                     (GDestroyNotify) Affinities::Destroy);

    DeleteFencer (message);
    _fencer_list = g_list_prepend (_fencer_list,
                                   player);

    return player;
  }

  // --------------------------------------------------------------------------------
  void Competition::DeleteFencer (Net::Message *message)
  {
    guint id = message->GetNetID ();

    for (GList *current = _fencer_list; current; current = g_list_next (current))
    {
      Player *fencer     = (Player *) current->data;
      guint   current_id = fencer->GetIntData (this, "netid");

      if (current_id == id)
      {
        fencer->Release ();
        _fencer_list = g_list_delete_link (_fencer_list,
                                           current);
        break;
      }
    }
  }

  // --------------------------------------------------------------------------------
  Player *Competition::GetFencer (guint ref)
  {
    for (GList *current = _fencer_list; current; current = g_list_next (current))
    {
      Player *fencer = (Player *) current->data;

      if (fencer->GetRef () == ref)
      {
        return fencer;
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Competition::OnBatchSelected (Batch *batch)
  {
    for (GList *current = _batches; current; current = g_list_next (current))
    {
      Batch *current_batch = (Batch *) current->data;

      if (batch == current_batch)
      {
        gtk_widget_show (current_batch->GetRootWidget ());
      }
      else
      {
        gtk_widget_hide (current_batch->GetRootWidget ());
      }
    }

    if (batch == nullptr)
    {
      gtk_widget_set_visible (_batch_image,
                              FALSE);
    }

    _current_batch = batch;
  }

  // --------------------------------------------------------------------------------
  void Competition::OnNewBatchStatus (Batch *batch)
  {
    {
      gtk_widget_set_visible (_batch_image,
                              FALSE);

      for (GList *current = _batches; current; current = g_list_next (current))
      {
        Batch *current_batch = (Batch *) current->data;

        if (BatchIsModifiable (current_batch))
        {
          if (current_batch->GetStatus () == Batch::Status::INCOMPLETE)
          {
            gtk_widget_set_visible (_batch_image,
                                    TRUE);
            break;
          }
        }
      }
    }

    {
      BatchPanel *panel = GetBatchPanel (batch);

      if (panel)
      {
        panel->RefreshStatus ();
      }
    }
  }
}
