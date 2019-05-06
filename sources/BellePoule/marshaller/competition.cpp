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
#include "batch.hpp"

#include "competition.hpp"

namespace Marshaller
{
  enum class BatchStoreColumnId
  {
    BATCH_NAME_str,
    BATCH_VISIBILITY_bool,
    BATCH_void
  };

  // --------------------------------------------------------------------------------
  Competition::Competition (guint            id,
                            Batch::Listener *listener)
    : Object ("Competition"),
    Module ("competition.glade")
  {
    _fencer_list = nullptr;
    _batches     = nullptr;
    _weapon      = nullptr;
    _id          = id;
    _gdk_color   = g_new (GdkColor, 1);

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

    _batch_image   = _glade->GetWidget ("batch_image");
    _spread_button = _glade->GetWidget ("spread_button");

    gtk_widget_set_visible (_batch_image,   FALSE);
    gtk_widget_set_visible (_spread_button, FALSE);

    _combobox = GTK_COMBO_BOX (_glade->GetWidget ("batch_combobox"));

    {
      _batch_store        = GTK_LIST_STORE (_glade->GetGObject ("batch_store"));
      _batch_store_filter = GTK_TREE_MODEL_FILTER (_glade->GetGObject ("batch_store_filter"));

      gtk_tree_model_filter_set_visible_column (_batch_store_filter,
                                                (gint) BatchStoreColumnId::BATCH_VISIBILITY_bool);
    }
  }

  // --------------------------------------------------------------------------------
  Competition::~Competition ()
  {
    gdk_color_free (_gdk_color);
    g_datalist_clear (&_properties);

    FreeFullGList (Player, _fencer_list);

    gtk_list_store_clear (_batch_store);
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
    GtkTreeIter *iter = GetBatchIter (batch);

    if (iter)
    {
      gboolean visible;

      gtk_tree_model_get (GTK_TREE_MODEL (_batch_store), iter,
                          BatchStoreColumnId::BATCH_VISIBILITY_bool, &visible,
                          -1);
      gtk_tree_iter_free (iter);

      return visible;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Competition::Freeze ()
  {
    gtk_widget_set_visible (_glade->GetWidget ("batch_combobox"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            FALSE);
  }

  // --------------------------------------------------------------------------------
  void Competition::SetProperties (Net::Message *message)
  {
    gtk_widget_set_visible (_glade->GetWidget ("batch_combobox"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            TRUE);

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
        GtkTreeIter iter;

        gtk_list_store_append (_batch_store,
                               &iter);

        gtk_list_store_set (_batch_store, &iter,
                            BatchStoreColumnId::BATCH_NAME_str, batch->GetName (),
                            BatchStoreColumnId::BATCH_void,     batch,
                            -1);
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
      ExposeBatch (batch);
    }

    return batch;
  }

  // --------------------------------------------------------------------------------
  void Competition::ExposeBatch (Batch *batch)
  {
    GtkTreeIter *iter = GetBatchIter (batch);

    if (iter)
    {
      gtk_list_store_set (_batch_store, iter,
                          BatchStoreColumnId::BATCH_VISIBILITY_bool, 1,
                          -1);

      if (gtk_combo_box_get_active_iter (_combobox,
                                         iter) == FALSE)
      {
        gtk_combo_box_set_active (_combobox,
                                  0);
      }

      gtk_tree_iter_free (iter);
    }
  }

  // --------------------------------------------------------------------------------
  void Competition::MaskBatch (Batch *batch)
  {
    GtkTreeIter *iter = GetBatchIter (batch);

    if (iter)
    {
      GtkTreeIter  active_iter;
      Batch       *active_batch = nullptr;

      if (gtk_combo_box_get_active_iter (_combobox,
                                         &active_iter))
      {
        gtk_tree_model_get (GTK_TREE_MODEL (_batch_store_filter), &active_iter,
                            BatchStoreColumnId::BATCH_void, &active_batch,
                            -1);
      }

      gtk_list_store_set (_batch_store, iter,
                          BatchStoreColumnId::BATCH_VISIBILITY_bool, FALSE,
                          -1);

      if (active_batch == batch)
      {
        gtk_combo_box_set_active (_combobox,
                                  0);
      }

      gtk_tree_iter_free (iter);
    }
  }

  // --------------------------------------------------------------------------------
  void Competition::DeleteBatch (Net::Message *message)
  {
    Batch *batch = GetBatch (message->GetNetID ());

    _batches = g_list_remove (_batches,
                              batch);

    {
      GtkTreeIter *iter = GetBatchIter (batch);

      if (iter)
      {
        gtk_list_store_remove (_batch_store,
                               iter);
        gtk_tree_iter_free (iter);
      }
    }

    batch->UnPlug ();
    batch->Release ();
  }

  // --------------------------------------------------------------------------------
  GtkTreeIter *Competition::GetBatchIter (Batch *batch)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_batch_store),
                                                               &iter);

    while (iter_is_valid)
    {
      Batch *current_batch;

      gtk_tree_model_get (GTK_TREE_MODEL (_batch_store), &iter,
                          BatchStoreColumnId::BATCH_void, &current_batch,
                          -1);

      if (batch == current_batch)
      {
        return gtk_tree_iter_copy (&iter);
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_batch_store),
                                                &iter);
    }

    return nullptr;
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
          xmlNode    *child           = xml_node->children;

          while (child)
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

            child = child->next;
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
    guint  id      = message->GetNetID ();
    GList *current = _fencer_list;

    while (current)
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

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Competition::OnChangeBatch ()
  {
    Batch *batch   = GetCurrentBatch ();
    GList *current = _batches;

    while (current)
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
      current = g_list_next (current);
    }

    if (batch == nullptr)
    {
      gtk_widget_set_visible (_batch_image,
                              FALSE);
      gtk_widget_set_visible (_spread_button,
                              FALSE);
    }
  }

  // --------------------------------------------------------------------------------
  Batch *Competition::GetCurrentBatch ()
  {
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter (_combobox,
                                       &iter))
    {
      Batch *batch;

      gtk_tree_model_get (GTK_TREE_MODEL (_batch_store_filter), &iter,
                          BatchStoreColumnId::BATCH_void, &batch,
                          -1);
      return batch;
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Competition::OnSpread ()
  {
    Batch *batch = GetCurrentBatch ();

    if (batch)
    {
      batch->OnValidateAssign ();
    }
  }

  // --------------------------------------------------------------------------------
  void Competition::SetBatchStatus (Batch         *batch,
                                    Batch::Status  status)
  {
    if (batch == GetCurrentBatch ())
    {
      if (status == Batch::Status::UNCOMPLETED)
      {
        gtk_widget_set_visible (_batch_image,
                                TRUE);
        gtk_widget_set_visible (_spread_button,
                                FALSE);
      }
      else if (status == Batch::Status::CONCEALED)
      {
        gtk_widget_set_visible (_batch_image,
                                FALSE);
        gtk_widget_set_visible (_spread_button,
                                TRUE);
      }
      else if (status == Batch::Status::DISCLOSED)
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

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_batch_combobox_changed (GtkComboBox *widget,
                                                             Object      *owner)
  {
    Competition *c = dynamic_cast <Competition *> (owner);

    c->OnChangeBatch ();
  }
}
