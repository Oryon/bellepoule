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

#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>

#include "util/attribute.hpp"

#include "people_management/form.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  Form::Form (Filter             *filter,
              Module             *client,
              Player::PlayerType  player_type,
              PlayerCbk           player_cbk)
    : Module ("form.glade",
              "FillInForm")
  {
    _client = client;
    _cbk    = player_cbk;

    _player_type = player_type;

    _filter = filter;
    _filter->Retain ();

    _locked = FALSE;

    if (player_type == Player::FENCER)
    {
      gtk_window_set_title (GTK_WINDOW (_glade->GetWidget ("FillInForm")),
                            gettext ("Fencer"));
    }
    else
    {
      gtk_window_set_title (GTK_WINDOW (_glade->GetWidget ("FillInForm")),
                            gettext ("Referee"));
    }

    {
      GSList      *current     = _filter->GetAttrList ();
      GtkComboBox *selector_w  = NULL;

      while (current)
      {
        AttributeDesc *attr_desc = (AttributeDesc *) current->data;

        if (attr_desc->_rights == AttributeDesc::PUBLIC)
        {
          {
            GtkWidget *w   = gtk_label_new (attr_desc->_user_name);
            GtkWidget *box = _glade->GetWidget ("title_vbox");

            gtk_box_pack_start (GTK_BOX (box),
                                w,
                                TRUE,
                                TRUE,
                                0);
            g_object_set (G_OBJECT (w),
                          "xalign", 0.0,
                          NULL);
          }

          {
            GtkWidget *value_w;

            {
              GtkWidget *box = _glade->GetWidget ("value_vbox");

              if (attr_desc->_type == G_TYPE_BOOLEAN)
              {
                value_w = gtk_check_button_new ();
              }
              else
              {
                if (attr_desc->HasDiscreteValue ())
                {
                  GtkCellRenderer    *cell;
                  GtkEntryCompletion *completion;

                  if (attr_desc->_free_value_allowed)
                  {
                    value_w = gtk_combo_box_entry_new ();
                    cell = NULL;
                    completion = gtk_entry_completion_new ();
                  }
                  else
                  {
                    value_w = gtk_combo_box_new ();
                    cell = gtk_cell_renderer_text_new ();
                    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (value_w), cell, TRUE);
                    completion = NULL;
                  }

                  if (attr_desc->_is_selector)
                  {
                    selector_w = GTK_COMBO_BOX (value_w);
                    g_signal_connect (value_w, "changed",
                                      G_CALLBACK (AttributeDesc::Refilter), value_w);
                  }

                  attr_desc->BindDiscreteValues (G_OBJECT (value_w),
                                                 cell,
                                                 selector_w);

                  if (completion)
                  {
                    GtkEntry     *entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (value_w)));
                    GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (value_w));

                    gtk_entry_completion_set_model (completion,
                                                    model);
                    gtk_entry_completion_set_text_column (completion,
                                                          AttributeDesc::DISCRETE_LONG_TEXT_str);
                    gtk_entry_completion_set_inline_completion (completion,
                                                                TRUE);
                    g_object_set (G_OBJECT (completion),
                                  "popup-set-width", FALSE,
                                  NULL);
                    gtk_entry_set_completion (entry,
                                              completion);
                    g_object_unref (completion);

                    if (attr_desc->_is_selector)
                    {
                      g_signal_connect (G_OBJECT (completion), "match-selected",
                                        G_CALLBACK (OnSelectorChanged), value_w);

                      g_signal_connect (G_OBJECT (entry), "activate",
                                        G_CALLBACK (OnSelectorEntryActivate), value_w);
                    }
                  }
                  else
                  {
                    gtk_combo_box_set_active (GTK_COMBO_BOX (value_w),
                                              0);
                  }
                }
                else
                {
                  value_w = gtk_entry_new ();
                  g_object_set (G_OBJECT (value_w),
                                "xalign", 0.0,
                                NULL);
                }
              }

              gtk_box_pack_start (GTK_BOX (box),
                                  value_w,
                                  TRUE,
                                  TRUE,
                                  0);
              g_object_set_data (G_OBJECT (value_w), "attribute_name", attr_desc->_code_name);
            }

            {
              GtkWidget *w   = gtk_check_button_new ();
              GtkWidget *box = _glade->GetWidget ("check_vbox");

              gtk_widget_set_tooltip_text (w,
                                           gettext ("Uncheck this box to quicken the data input. The corresponding field will be "
                                                    "ignored on TAB key pressed."));
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                            TRUE);
              g_signal_connect (w, "toggled",
                                G_CALLBACK (OnSensitiveStateToggled), value_w);

              gtk_box_pack_start (GTK_BOX (box),
                                  w,
                                  TRUE,
                                  TRUE,
                                  0);
              g_object_set (G_OBJECT (w),
                            "xalign", 0.0,
                            NULL);
            }
          }
        }
        current = g_slist_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Form::~Form ()
  {
    _filter->Release ();
  }

  // --------------------------------------------------------------------------------
  void Form::SetSelectorValue (GtkComboBox *combo_box,
                               const gchar *value)
  {
    GtkTreeModel *model = gtk_combo_box_get_model (combo_box);

    if (value && model)
    {
      GtkTreeIter  iter;
      gboolean     iter_is_valid;
      gboolean     target_found = FALSE;
      gchar       *case_value = g_utf8_casefold (value, -1);

      iter_is_valid = gtk_tree_model_get_iter_first (model,
                                                     &iter);

      while (iter_is_valid && (target_found == FALSE))
      {
        gchar *current_value;

        gtk_tree_model_get (model, &iter,
                            AttributeDesc::DISCRETE_LONG_TEXT_str, &current_value,
                            -1);

        if (current_value)
        {
          gchar *case_current_value = g_utf8_casefold (current_value, -1);

          if (g_ascii_strcasecmp (case_current_value, case_value) == 0)
          {
            gtk_combo_box_set_active_iter (combo_box,
                                           &iter);
            target_found = TRUE;
          }
          g_free (case_current_value);
          g_free (current_value);
        }

        iter_is_valid = gtk_tree_model_iter_next (model,
                                                  &iter);
      }

      g_free (case_value);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Form::OnSelectorChanged (GtkEntryCompletion *widget,
                                    GtkTreeModel       *model,
                                    GtkTreeIter        *iter,
                                    GtkComboBox        *combobox)
  {
    gchar *value;

    gtk_tree_model_get (model, iter,
                        AttributeDesc::DISCRETE_LONG_TEXT_str, &value,
                        -1);
    SetSelectorValue (combobox,
                      value);
    g_free (value);

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Form::OnSelectorEntryActivate (GtkEntry    *widget,
                                      GtkComboBox *combobox)
  {
    SetSelectorValue (combobox,
                      gtk_entry_get_text (widget));
  }

  // --------------------------------------------------------------------------------
  void Form::OnAddButtonClicked ()
  {
    GList  *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));
    Player *player;

    if (_player_to_update)
    {
      player = _player_to_update;
    }
    else
    {
      player = new Player (_player_type);
    }

    ReadAndWipe (player);

    gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                          0));

    if (_player_to_update)
    {
      (_client->*_cbk) (player,
                        UPDATE_PLAYER);
      OnCloseButtonClicked ();
    }
    else
    {
      (_client->*_cbk) (player,
                        NEW_PLAYER);
    }

    player->Release ();
  }

  // --------------------------------------------------------------------------------
  void Form::ReadAndWipe (Player *player)
  {
    GList *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));

    for (guint i = 0; i < g_list_length (children); i ++)
    {
      GtkWidget           *w;
      gchar               *attr_name;
      AttributeDesc       *attr_desc;
      Player::AttributeId *attr_id = NULL;

      w = (GtkWidget *) g_list_nth_data (children,
                                         i);
      attr_name = (gchar *) g_object_get_data (G_OBJECT (w), "attribute_name");
      attr_desc = AttributeDesc::GetDescFromCodeName (attr_name);

      if (player)
      {
        attr_id = Player::AttributeId::Create (attr_desc,
                                               _client);
      }

      if (attr_desc->_type == G_TYPE_BOOLEAN)
      {
        if (attr_id)
        {
          player->SetAttributeValue (attr_id,
                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
        }

        if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                        FALSE);
        }
      }
      else
      {
        if (attr_desc->HasDiscreteValue ())
        {
          if (attr_desc->_free_value_allowed)
          {
            GtkEntry *entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (w)));

            if (entry)
            {
              if (attr_id)
              {
                gchar *xml_image = attr_desc->GetDiscreteXmlImage (gtk_entry_get_text (GTK_ENTRY (entry)));

                player->SetAttributeValue (attr_id,
                                           xml_image);
                g_free (xml_image);
              }

              if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
              {
                gtk_entry_set_text (GTK_ENTRY (w), "");
              }
            }
          }
          else if (attr_id)
          {
            GtkTreeIter iter;

            if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (w),
                                               &iter))
            {
              gchar *value = attr_desc->GetXmlImage (&iter);

              player->SetAttributeValue (attr_id,
                                         value);
              g_free (value);
            }
          }
        }
        else
        {
          if (attr_id)
          {
            gchar *value = (gchar *) gtk_entry_get_text (GTK_ENTRY (w));

            player->SetAttributeValue (attr_id,
                                       value);
          }

          if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
          {
            gtk_entry_set_text (GTK_ENTRY (w), "");
          }
        }
      }
      Object::TryToRelease (attr_id);
    }
  }

  // --------------------------------------------------------------------------------
  void Form::OnCloseButtonClicked ()
  {
    GtkWidget *w = _glade->GetWidget ("FillInForm");

    gtk_widget_hide (w);

    ReadAndWipe (NULL);
  }

  // --------------------------------------------------------------------------------
  void Form::Lock ()
  {
    _locked = TRUE;
  }

  // --------------------------------------------------------------------------------
  void Form::UnLock ()
  {
    _locked = FALSE;
  }

  // --------------------------------------------------------------------------------
  void Form::Show (Player *player)
  {
    _player_to_update = player;
    if (_player_to_update)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));

      _player_to_update->Retain ();
      for (guint i = 0; i < g_list_length (children); i ++)
      {
        GtkWidget           *w;
        gchar               *attr_name;
        AttributeDesc       *attr_desc;
        Player::AttributeId *attr_id;
        Attribute           *attr;

        w = (GtkWidget *) g_list_nth_data (children,
                                           i);
        attr_name = (gchar *) g_object_get_data (G_OBJECT (w), "attribute_name");
        attr_desc = AttributeDesc::GetDescFromCodeName (attr_name);
        attr_id   = Player::AttributeId::Create (attr_desc, _client);
        attr      = player->GetAttribute (attr_id);

        if (attr)
        {
          if (attr_desc->_type == G_TYPE_BOOLEAN)
          {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                          attr->GetUIntValue ());
          }
          else if (attr_desc->HasDiscreteValue ())
          {
            gchar *image = attr->GetUserImage (AttributeDesc::LONG_TEXT);

            if (attr_desc->_free_value_allowed)
            {
              GtkEntry *entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (w)));

              gtk_entry_set_text (entry,
                                  image);
            }
            else
            {
              GtkTreeIter *iter = attr_desc->GetDiscreteIter (image);

              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (w),
                                             iter);
              gtk_tree_iter_free (iter);
            }

            if (attr_desc->_is_selector)
            {
              SetSelectorValue (GTK_COMBO_BOX (w),
                                image);
            }
            g_free (image);
          }
          else
          {
            gchar *image = attr->GetUserImage (AttributeDesc::LONG_TEXT);

            gtk_entry_set_text (GTK_ENTRY (w),
                                image);
            g_free (image);
          }
        }
        attr_id->Release ();
      }

      gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                            0));
    }

    gtk_widget_show_all (_glade->GetWidget ("FillInForm"));

    if (_player_to_update)
    {
      gtk_widget_hide (_glade->GetWidget ("add_button"));
      if (_locked)
      {
        gtk_widget_hide (_glade->GetWidget ("apply_button"));
      }
      else
      {
        gtk_widget_show (_glade->GetWidget ("apply_button"));
      }
    }
    else
    {
      gtk_widget_show (_glade->GetWidget ("add_button"));
      gtk_widget_hide (_glade->GetWidget ("apply_button"));
    }

    gtk_widget_set_sensitive (_glade->GetWidget ("check_vbox"),
                              !_locked);
    gtk_widget_set_sensitive (_glade->GetWidget ("value_vbox"),
                              !_locked);

    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));

      gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                            0));
    }
  }

  // --------------------------------------------------------------------------------
  void Form::OnSensitiveStateToggled (GtkToggleButton *togglebutton,
                                      GtkWidget       *w)
  {
    if (gtk_toggle_button_get_active (togglebutton))
    {
      gtk_widget_set_sensitive (w,
                                TRUE);
    }
    else
    {
      gtk_widget_set_sensitive (w,
                                FALSE);
    }
  }

  // --------------------------------------------------------------------------------
  void Form::Hide ()
  {
    GtkWidget *w = _glade->GetWidget ("FillInForm");

    gtk_widget_hide (w);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_add_button_clicked (GtkWidget *widget,
                                                         Object    *owner)
  {
    Form *f = dynamic_cast <Form *> (owner);

    f->OnAddButtonClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_close_button_clicked (GtkWidget *widget,
                                                           Object    *owner)
  {
    Form *f = dynamic_cast <Form *> (owner);

    f->OnCloseButtonClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT gboolean on_FillInForm_key_press_event (GtkWidget   *widget,
                                                                     GdkEventKey *event,
                                                                     Object      *owner)
  {
    Form *f = dynamic_cast <Form *> (owner);

    if (event->keyval == GDK_Return)
    {
      f->OnAddButtonClicked ();
      return TRUE;
    }

    return FALSE;
  }
}
