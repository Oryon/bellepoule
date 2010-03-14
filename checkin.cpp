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

#include "attribute.hpp"
#include "schedule.hpp"
#include "player.hpp"
#include "filter.hpp"

#include "checkin.hpp"

const gchar *Checkin::_class_name     = "checkin";
const gchar *Checkin::_xml_class_name = "checkin_stage";

// --------------------------------------------------------------------------------
Checkin::Checkin (StageClass *stage_class)
: Stage (stage_class),
  PlayersList ("checkin.glade")
{
  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
  }

  // Player attributes to display
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "HS",
                               "previous_stage_rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("attending");
    filter->ShowAttribute ("rating");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("birth_year");
    filter->ShowAttribute ("gender");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("ligue");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("licence");

    SetFilter (filter);
    filter->Release ();
  }

  _attendings = 0;

  {
    GSList *filter_list = _filter->GetAttrList ();

    for (guint i = 0; i < g_slist_length (filter_list); i++)
    {
      AttributeDesc *attr_desc;

      attr_desc = (AttributeDesc *) g_slist_nth_data (filter_list,
                                                      i);

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
            GtkWidget *box = GetWidget ("value_vbox");

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

                attr_desc->BindDiscreteValues (G_OBJECT (value_w),
                                               cell);

                if (completion)
                {
                  GtkEntry     *entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (value_w)));
                  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (value_w));

                  gtk_entry_completion_set_model (completion,
                                                  model);
                  gtk_entry_completion_set_text_column (completion,
                                                        1);
                  gtk_entry_completion_set_inline_completion (completion,
                                                              TRUE);
                  gtk_entry_set_completion (entry,
                                            completion);
                  g_object_unref (completion);
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
            g_object_set_data (G_OBJECT (value_w), "attribute_name", attr_desc->_xml_name);
          }

          {
            GtkWidget *w   = gtk_check_button_new ();
            GtkWidget *box = _glade->GetWidget ("check_vbox");

            gtk_widget_set_tooltip_text (w,
                                         "Décochez cette case pour accélerer la navigation. "
                                         "Le champs correspondant sera ignoré lors des appuis sur la touche TAB.");
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                          TRUE);
            g_signal_connect (w, "toggled",
                              G_CALLBACK (on_sensitive_state_toggled), value_w);

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
    }
  }
}

// --------------------------------------------------------------------------------
Checkin::~Checkin ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      0);
}

// --------------------------------------------------------------------------------
Stage *Checkin::CreateInstance (StageClass *stage_class)
{
  return new Checkin (stage_class);
}

// --------------------------------------------------------------------------------
void Checkin::OnPlugged ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Load (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "player") == 0)
      {
        Player *player = new Player;

        player->Load (n);

        Monitor (player);
        Add (player);

        player->Release ();
      }
      else if (strcmp ((char *) n->name, _xml_class_name) != 0)
      {
        RefreshAttendingDisplay ();
        UpdateRanking ();
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void Checkin::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  Stage::SaveConfiguration (xml_writer);

  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player *p;

    p = (Player *) g_slist_nth_data (_player_list, i);

    if (p)
    {
      p->Save (xml_writer);
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Checkin::Display ()
{
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Checkin::OnListChanged ()
{
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::UpdateRanking ()
{
  guint               nb_player  = g_slist_length (_player_list);
  guint               nb_missing = 0;
  Player::AttributeId attr_id ("rating");

  _player_list = g_slist_sort_with_data (_player_list,
                                         (GCompareDataFunc) Player::Compare,
                                         &attr_id);

  for (guint i = 0; i < nb_player; i++)
  {
    Player    *p;
    Attribute *attending;

    p = (Player *) g_slist_nth_data (_player_list, i);

    {
      Player::AttributeId attr_id ("attending");

      attending = p->GetAttribute (&attr_id);
    }

    if ((gboolean) attending->GetValue ())
    {
      Player::AttributeId attr_id ("rank", this);

      p->SetAttributeValue (&attr_id,
                            i - nb_missing + 1);
    }
    else
    {
      Player::AttributeId attr_id ("rank", this);

      p->SetAttributeValue (&attr_id,
                            nb_player - nb_missing);
      nb_missing++;
    }
    Update (p);
  }
}

// --------------------------------------------------------------------------------
void Checkin::OnLocked ()
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);
}

// --------------------------------------------------------------------------------
GSList *Checkin::GetCurrentClassification ()
{
  GSList *result = CreateCustomList (PresentPlayerFilter);

  if (result)
  {
    Player::AttributeId attr_id ("rank",
                                 this);

    return g_slist_sort_with_data (result,
                                   (GCompareDataFunc) Player::Compare,
                                   &attr_id);
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
gboolean Checkin::IsOver ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Checkin::PresentPlayerFilter (Player *player)
{
  Player::AttributeId  attr_id ("attending");
  Attribute           *attr = player->GetAttribute (&attr_id);

  return ((gboolean) attr->GetValue () == TRUE);
}

// --------------------------------------------------------------------------------
void Checkin::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
void Checkin::Wipe ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Import ()
{
  GtkWidget *chooser = gtk_file_chooser_dialog_new ("Choisissez un fichier de tireurs ? importer ...",
                                                    NULL,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All FFE files (.FFF)");
    gtk_file_filter_add_pattern (filter,
                                 "*.FFF");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All Excell files (.CSV)");
    gtk_file_filter_add_pattern (filter,
                                 "*.CSV");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All files");
    gtk_file_filter_add_pattern (filter,
                                 "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    gchar *prg_name = g_get_prgname ();

    if (prg_name)
    {
      gchar *install_dirname = g_path_get_dirname (prg_name);

      if (install_dirname)
      {
        gchar *example_dirname = g_strdup_printf ("%s/Exemples_Fichiers_FFE", install_dirname);

        gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (chooser),
                                              example_dirname,
                                              NULL);
        g_free (example_dirname);
        g_free (install_dirname);
      }
    }
  }

  {
    gchar *last_dirname = g_key_file_get_string (_config_file,
                                                 "Checkin",
                                                 "default_import_dir_name",
                                                 NULL);
    if (last_dirname)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                           last_dirname);

      g_free (last_dirname);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (filename)
    {
      gchar *file;

      {
        gchar *dirname = g_path_get_dirname (filename);

        g_key_file_set_string (_config_file,
                               "Checkin",
                               "default_import_dir_name",
                               dirname);
        g_free (dirname);
      }

      {
        GtkRecentManager *manager = gtk_recent_manager_get_default ();

        gtk_recent_manager_add_item (manager, filename);
      }

      {
        gchar *raw_file;
        gsize  length;
        guint  j;

        g_file_get_contents ((const gchar *) filename,
                             &raw_file,
                             &length,
                             NULL);
        length++;

        file = (gchar *) g_malloc (length);

        j = 0;
        for (guint i = 0; i < length; i++)
        {
          if (raw_file[i] != '\r')
          {
            file[j] = raw_file[i];
            j++;
          }
        }
      }

      if (strstr (filename, ".FFF"))
      {
        ImportFFF (file);
      }
      else
      {
        ImportCSV (file);
      }

      g_free (file);
    }
  }

  gtk_widget_destroy (chooser);

  RefreshAttendingDisplay ();
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::ImportFFF (gchar *file)
{
  gchar **lines = g_strsplit_set (file,
                                  "\n",
                                  0);

  if (lines == NULL)
  {
    return;
  }
  else
  {
    // Header
    // FFF;WIN;CLASSEMENT;FFE
    // 08/11/2009;;;;

    // Fencers
    for (guint l = 2; lines[l] != NULL; l++)
    {
      gchar **tokens = g_strsplit_set (lines[l],
                                       ",;",
                                       0);

      if (tokens && tokens[0])
      {
        Player              *player = new Player;
        Player::AttributeId  attr_id ("");

        attr_id._name = "attending";
        player->SetAttributeValue (&attr_id, (guint) FALSE);

        attr_id._name = "name";
        player->SetAttributeValue (&attr_id, tokens[0]);

        attr_id._name = "first_name";
        player->SetAttributeValue (&attr_id, tokens[1]);

        attr_id._name = "birth_year";
        player->SetAttributeValue (&attr_id, tokens[2]);

        attr_id._name = "gender";
        player->SetAttributeValue (&attr_id, tokens[3]);

        attr_id._name = "country";
        player->SetAttributeValue (&attr_id, tokens[4]);

        attr_id._name = "licence";
        player->SetAttributeValue (&attr_id, tokens[8]);

        attr_id._name = "club";
        player->SetAttributeValue (&attr_id, tokens[10]);

        attr_id._name = "exported";
        player->SetAttributeValue (&attr_id, (guint) FALSE);

        if (*tokens[11])
        {
          attr_id._name = "rating";
          player->SetAttributeValue (&attr_id, tokens[11]);

        }
        else
        {
          attr_id._name = "rating";
          player->SetAttributeValue (&attr_id, (guint) 0);

        }

        Monitor (player);
        Add (player);
        player->Release ();

        g_strfreev (tokens);
      }
    }

    g_strfreev (lines);
  }
}

// --------------------------------------------------------------------------------
void Checkin::ImportCSV (gchar *file)
{
  guint nb_attr = 0;

  gchar **header_line = g_strsplit_set (file,
                                        "\n",
                                        0);

  if (header_line == NULL)
  {
    return;
  }
  else
  {
    // Header
    {
      gchar **header_attr = g_strsplit_set (header_line[0],
                                            ";",
                                            0);

      if (header_attr)
      {
        for (guint i = 0; header_attr[i] != NULL; i++)
        {
          nb_attr++;
        }
        g_strfreev (header_attr);
      }

      g_strfreev (header_line);
    }

    // Fencers
    {
      gchar **tokens = g_strsplit_set (file,
                                       ";\n",
                                       0);

      if (tokens)
      {
        for (guint i = nb_attr; tokens[i] != NULL; i += nb_attr)
        {
          Player              *player = new Player;
          Player::AttributeId  attr_id ("");

          attr_id._name = "attending";
          player->SetAttributeValue (&attr_id, (guint) FALSE);

          attr_id._name = "name";
          player->SetAttributeValue (&attr_id, tokens[i]);

          attr_id._name = "first_name";
          player->SetAttributeValue (&attr_id, tokens[i+1]);

          attr_id._name = "exported";
          player->SetAttributeValue (&attr_id, (guint) FALSE);


          Monitor (player);
          Add (player);
          player->Release ();
        }
        g_strfreev (tokens);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Checkin::on_add_button_clicked ()
{
  Player *player   = new Player;
  GList  *children = gtk_container_get_children (GTK_CONTAINER (GetWidget ("value_vbox")));

  {
    Player::AttributeId  attr_id ("ref");
    gchar               *str = g_strdup_printf ("%d\n", player->GetRef ());

    player->SetAttributeValue (&attr_id, str);
    g_free (str);
  }

  {
    Player::AttributeId attr_id ("exported");

    player->SetAttributeValue (&attr_id, (guint) FALSE);
  }

  for (guint i = 0; i < g_list_length (children); i ++)
  {
    GtkWidget           *w;
    gchar               *attr_name;
    AttributeDesc       *attr_desc;
    Player::AttributeId *attr_id;

    w = (GtkWidget *) g_list_nth_data (children,
                                       i);
    attr_name = (gchar *) g_object_get_data (G_OBJECT (w), "attribute_name");
    attr_desc = AttributeDesc::GetDesc (attr_name);
    attr_id   = Player::AttributeId::CreateAttributeId (attr_desc, this);

    if (attr_desc->_type == G_TYPE_BOOLEAN)
    {
      player->SetAttributeValue (attr_id,
                                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));

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
            player->SetAttributeValue (attr_id,
                                       (gchar *) gtk_entry_get_text (GTK_ENTRY (entry)));

            if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
            {
              gtk_entry_set_text (GTK_ENTRY (w), "");
            }
          }
        }
        else
        {
          GtkTreeIter iter;

          if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (w),
                                             &iter))
          {
            gchar *value = attr_desc->GetUserImage (&iter);

            player->SetAttributeValue (attr_id,
                                       value);
            g_free (value);
          }
        }
      }
      else
      {
        player->SetAttributeValue (attr_id,
                                   (gchar *) gtk_entry_get_text (GTK_ENTRY (w)));

        if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
        {
          gtk_entry_set_text (GTK_ENTRY (w), "");
        }
      }
    }
    attr_id->Release ();
  }

  gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                        0));

  Monitor (player);
  Add (player);
  player->Release ();
  RefreshAttendingDisplay ();
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::Monitor (Player *player)
{
  {
    Player::AttributeId attr_id ("attending");

    Attribute *attr = player->GetAttribute (&attr_id);

    if (attr && ((gboolean) attr->GetValue () == TRUE))
    {
      _attendings++;
    }

    player->SetChangeCbk ("attending",
                          (Player::OnChange) OnAttendingChanged,
                          this);
  }
}

// --------------------------------------------------------------------------------
void Checkin::OnPlayerRemoved (Player *player)
{
  Player::AttributeId  attr_id ("attending");
  Attribute           *attr = player->GetAttribute (&attr_id);

  if (attr && ((gboolean) attr->GetValue () == TRUE))
  {
    _attendings--;
  }
  RefreshAttendingDisplay ();
}

// --------------------------------------------------------------------------------
void Checkin::OnAttendingChanged (Player    *player,
                                  Attribute *attr,
                                  Checkin   *checkin)
{
  guint value = (guint) attr->GetValue ();

  if (value == 1)
  {
    checkin->_attendings++;
  }
  else if (value == 0)
  {
    checkin->_attendings--;
  }

  checkin->RefreshAttendingDisplay ();
}

// --------------------------------------------------------------------------------
void Checkin::RefreshAttendingDisplay ()
{
  gchar *text;

  text = g_strdup_printf ("%d", _attendings);
  gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("attending_label")),
                      text);
  g_free (text);

  text = g_strdup_printf ("%d", g_slist_length (_player_list) - _attendings);
  gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("absent_label")),
                      text);
  g_free (text);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_player_button_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->on_add_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void Checkin::on_add_player_button_clicked ()
{
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  gtk_widget_show_all (w);

  {
    GList *children = gtk_container_get_children (GTK_CONTAINER (GetWidget ("value_vbox")));

    gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                          0));
  }
}

// --------------------------------------------------------------------------------
void Checkin::on_sensitive_state_toggled (GtkToggleButton *togglebutton,
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
extern "C" G_MODULE_EXPORT void on_remove_player_button_clicked (GtkWidget *widget,
                                                                 Object    *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->on_remove_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void Checkin::on_remove_player_button_clicked ()
{
  RemoveSelection ();
}

// --------------------------------------------------------------------------------
void Checkin::on_close_button_clicked ()
{
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  gtk_widget_hide (w);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_players_list_filter_button_clicked (GtkWidget *widget,
                                                                       Object    *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_import_toolbutton_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->Import ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_button_clicked (GtkWidget *widget,
                                                       Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->on_add_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_close_button_clicked (GtkWidget *widget,
                                                         Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->on_close_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_FillInForm_key_press_event (GtkWidget   *widget,
                                                                   GdkEventKey *event,
                                                                   Object      *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  if (event->keyval == GDK_Return)
  {
    c->on_add_button_clicked ();
    return TRUE;
  }

  return FALSE;
}
