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

#include "attribute.hpp"
#include "schedule.hpp"
#include "player.hpp"
#include "filter.hpp"

#include "checkin.hpp"

const gchar *Checkin::_class_name     = N_("Check-in");
const gchar *Checkin::_xml_class_name = "checkin_stage";

// --------------------------------------------------------------------------------
Checkin::Checkin (StageClass *stage_class)
: Stage (stage_class),
  PlayersList ("checkin.glade")
{
  _use_initial_rank = FALSE;

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
    AddSensitiveWidget (_glade->GetWidget ("all_present_button"));
    AddSensitiveWidget (_glade->GetWidget ("all_absent_button"));
  }

  // Player attributes to display
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
                               "start_rank",
#endif
                               "status",
                               "global_status",
                               "previous_stage_rank",
                               "exported",
                               "final_rank",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
                               "HS",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("attending");
    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("birth_date");
    filter->ShowAttribute ("gender");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("league");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("ranking");
    filter->ShowAttribute ("licence");

    SetFilter (filter);
    filter->Release ();
  }

  _attendings = 0;

  {
    GtkWidget *content_area;

    _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        gettext ("<b><big>Which fencers do you want to print?</big></b>"));

    gtk_window_set_title (GTK_WINDOW (_print_dialog),
                          gettext ("Fencer list printing"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_dialog-vbox"),
                         content_area);
  }

  {
    GSList      *filter_list = _filter->GetAttrList ();
    GtkComboBox *selector_w  = NULL;

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
                                                        AttributeDesc::DISCRETE_USER_IMAGE);
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

  RefreshAttendingDisplay ();
}

// --------------------------------------------------------------------------------
Checkin::~Checkin ()
{
  gtk_widget_destroy (_print_dialog);
}

// --------------------------------------------------------------------------------
void Checkin::Init ()
{
  RegisterStageClass (gettext (_class_name),
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
void Checkin::Add (Player *player)
{
  Monitor (player);
  PlayersList::Add (player);
}

// --------------------------------------------------------------------------------
void Checkin::Load (xmlXPathContext *xml_context,
                    const gchar     *from_node)
{
  gchar          *path        = g_strdup_printf ("%s/Tireurs", from_node);
  xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST path, xml_context);
  xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

  if (xml_object->nodesetval->nodeNr)
  {
    Load (xml_nodeset->nodeTab[0]);
  }

  _attendees = new Attendees ();

  g_free (path);
  xmlXPathFreeObject (xml_object);
}

// --------------------------------------------------------------------------------
void Checkin::Load (xmlNode *xml_node)
{
  Player::AttributeId start_rank_id    ("start_rank");
  Player::AttributeId previous_rank_id ("previous_stage_rank", this);

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        Attribute *start_rank;
        Player    *player = new Player;

        player->Load (n);

        Add (player);
        start_rank = player->GetAttribute (&start_rank_id);
        if (start_rank)
        {
          UseInitialRank ();
          player->SetAttributeValue (&previous_rank_id,
                                     start_rank->GetUIntValue ());
        }
        player->Release ();
      }
      else if (strcmp ((char *) n->name, "Tireurs") != 0)
      {
        OnListChanged ();
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void Checkin::Save (xmlTextWriter *xml_writer)
{
  // Players
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Tireurs");

    for (guint i = 0; i < g_slist_length (_player_list); i++)
    {
      Player *p = (Player *) g_slist_nth_data (_player_list, i);

      if (p)
      {
        p->Save (xml_writer);
      }
    }

    xmlTextWriterEndElement (xml_writer);
  }

  // Referees
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Arbitres");
    xmlTextWriterEndElement (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Checkin::Display ()
{
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Checkin::OnListChanged ()
{
  RefreshAttendingDisplay ();
  UpdateRanking ();
  MakeDirty ();
}

// --------------------------------------------------------------------------------
void Checkin::UseInitialRank ()
{
  _use_initial_rank = TRUE;
}

// --------------------------------------------------------------------------------
void Checkin::ConvertFromBaseToResult ()
{
  // This method aims to deal with the strange FIE xml specification.
  // Two different file formats are used. One for preparation one for result.
  // Both are almost similar except that they use a same keyword ("classement")
  // for a different meanning.

  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player *p = (Player *) g_slist_nth_data (_player_list, i);

    if (p)
    {
      Player::AttributeId  place_attr_id ("final_rank");
      Attribute           *final_rank  = p->GetAttribute (&place_attr_id);

      if (final_rank)
      {
        Player::AttributeId ranking_attr_id ("ranking");

        p->SetAttributeValue (&ranking_attr_id,
                              final_rank->GetUIntValue ());
        p->SetAttributeValue (&place_attr_id,
                              1);
      }
    }
  }
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::UpdateChecksum ()
{
  {
    Player::AttributeId attr_id ("ref");

    attr_id.MakeRandomReady (1);
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           &attr_id);
  }

  {
    Player::AttributeId  attr_id ("attending");
    GChecksum           *checksum       = g_checksum_new (G_CHECKSUM_MD5);
    GSList              *current_player = _player_list;

    while (current_player)
    {
      Player *p;

      p = (Player *) current_player->data;
      if (p)
      {
        Attribute *attending;

        attending = p->GetAttribute (&attr_id);
        if (attending && attending->GetUIntValue ())
        {
          gchar *name = p->GetName ();

          name[0] = toupper (name[0]);
          g_checksum_update (checksum,
                             (guchar *) name,
                             1);
          g_free (name);
        }
      }
      current_player = g_slist_next (current_player);
    }

    {
      gchar short_checksum[9];

      strncpy (short_checksum,
               g_checksum_get_string (checksum),
               sizeof (short_checksum));
      short_checksum[8] = 0;

      _rand_seed = strtoul (short_checksum, NULL, 16);
    }

    g_checksum_free (checksum);
  }
}

// --------------------------------------------------------------------------------
void Checkin::UpdateRanking ()
{
  guint                nb_player = g_slist_length (_player_list);
  Player::AttributeId *rank_criteria_id;

  UpdateChecksum ();

  {
    if (_use_initial_rank)
    {
      rank_criteria_id = new Player::AttributeId ("previous_stage_rank",
                                                  this);
    }
    else
    {
      rank_criteria_id = new Player::AttributeId ("ranking");
    }

    rank_criteria_id->MakeRandomReady (_rand_seed);
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           rank_criteria_id);
  }

  {
    Player *previous_player = NULL;
    GSList *current_player  = _player_list;
    gint    previous_rank   = 0;
    guint   nb_present      = 1;

    while (current_player)
    {
      Player    *p;
      Attribute *attending;

      p = (Player *) current_player->data;

      {
        Player::AttributeId attr_id ("attending");

        attending = p->GetAttribute (&attr_id);
      }

      {
        Player::AttributeId previous_rank_id ("previous_stage_rank", this);
        Player::AttributeId rank_id          ("rank", this);

        if (attending && attending->GetUIntValue ())
        {
          if (   previous_player
              && (Player::Compare (previous_player, p, rank_criteria_id) == 0))
          {
            p->SetAttributeValue (&previous_rank_id,
                                  previous_rank);
            p->SetAttributeValue (&rank_id,
                                  previous_rank);
          }
          else
          {
            p->SetAttributeValue (&previous_rank_id,
                                  nb_present);
            p->SetAttributeValue (&rank_id,
                                  nb_present);
            previous_rank = nb_present;
          }
          previous_player = p;
          nb_present++;
        }
        else
        {
          p->SetAttributeValue (&previous_rank_id,
                                nb_player);
          p->SetAttributeValue (&rank_id,
                                nb_player);
        }
      }
      Update (p);

      current_player  = g_slist_next (current_player);
    }
  }
  rank_criteria_id->Release ();
}

// --------------------------------------------------------------------------------
void Checkin::OnLocked (Reason reason)
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
    Player::AttributeId attr_id ("rank");

    attr_id.MakeRandomReady (_rand_seed);
    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) Player::Compare,
                                     &attr_id);

    result = g_slist_reverse (result);

    _attendees->SetGlobalList (result);
  }
  return result;
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

  return ((attr == NULL) || (attr->GetUIntValue () == TRUE));
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
void Checkin::OnImport ()
{
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext ("Choose a fencer file to import..."),
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
                              gettext ("All FFF files (.FFF)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.FFF");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

#if 0
  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("All Excel files (.CSV)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.CSV");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }
#endif

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("All files"));
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
        gchar *example_dirname = g_strdup_printf ("%s/Exemples", install_dirname);

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
    if (last_dirname && g_file_test (last_dirname,
                                     G_FILE_TEST_IS_DIR))
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

#if 0
      if (strstr (filename, ".FFF"))
      {
        ImportFFF (file);
      }
      else
      {
        ImportCSV (file);
      }
#else
      ImportFFF (file);
#endif

      g_free (file);
    }
  }

  gtk_widget_destroy (chooser);

  OnListChanged ();
}

// --------------------------------------------------------------------------------
gchar *Checkin::ConvertToUtf8 (gchar *what)
{
  gsize   bytes_written;
  GError *error = NULL;
  gchar  *result;

  result = g_convert (what,
                      -1,
                      "UTF-8",
                      "ISO-8859-1",
                      NULL,
                      &bytes_written,
                      &error);

  if (error)
  {
    g_print ("<<ConvertToUtf8>> %s\n", error->message);
    g_clear_error (&error);
  }

  return result;
}

// --------------------------------------------------------------------------------
void Checkin::ImportFFF (gchar *file)
{
  gchar *iso_8859_file = ConvertToUtf8 (file);

  if (iso_8859_file)
  {
    gchar **lines = g_strsplit_set (iso_8859_file,
                                    "\n",
                                    0);

    if (lines && lines[0] && lines[1])
    {
      // Header
      // FFF;WIN;CLASSEMENT;FFE
      // 08/11/2009;;;;

      // Fencers
      for (guint l = 2; lines[l] != NULL; l++)
      {
        gchar               **line;
        Player               *player;
        Player::AttributeId   attr_id ("");

        player = NULL;
        line = g_strsplit_set (lines[l],
                               ";",
                               0);

        // General
        if (line && line[0])
        {
          gchar **tokens = g_strsplit_set (line[0],
                                           ",",
                                           0);

          player = new Player;

          attr_id._name = (gchar *) "attending";
          player->SetAttributeValue (&attr_id, (guint) FALSE);

          if (tokens)
          {
            attr_id._name = (gchar *) "name";
            player->SetAttributeValue (&attr_id, tokens[0]);

            attr_id._name = (gchar *) "first_name";
            player->SetAttributeValue (&attr_id, tokens[1]);

            attr_id._name = (gchar *) "birth_date";
            player->SetAttributeValue (&attr_id, tokens[2]);

            attr_id._name = (gchar *) "gender";
            player->SetAttributeValue (&attr_id, tokens[3]);

            attr_id._name = (gchar *) "country";
            player->SetAttributeValue (&attr_id, tokens[4]);

            g_strfreev (tokens);
          }
        }

        // FIE
        if (player && line[1])
        {
        }

        // National federation
        if (player && line[2])
        {
          gchar **tokens = g_strsplit_set (line[2],
                                           ",",
                                           0);
          if (tokens)
          {
            attr_id._name = (gchar *) "licence";
            player->SetAttributeValue (&attr_id, tokens[0]);

            if (strlen (tokens[0]) > 2)
            {
              AttributeDesc *league_desc = AttributeDesc::GetDesc ("league");

              tokens[0][2] = 0;

              if (league_desc)
              {
                gchar *league = league_desc->GetDiscreteUserImage (atoi (tokens[0]));

                if (league)
                {
                  attr_id._name = (gchar *) "league";
                  player->SetAttributeValue (&attr_id, league);
                  g_free (league);
                }
              }
            }

            attr_id._name = (gchar *) "club";
            player->SetAttributeValue (&attr_id, tokens[2]);

            attr_id._name = (gchar *) "exported";
            player->SetAttributeValue (&attr_id, (guint) FALSE);

            if (tokens[3] == NULL)
            {
              attr_id._name = (gchar *) "ranking";
              player->SetAttributeValue (&attr_id, (guint) 0);
            }
            else
            {
              attr_id._name = (gchar *) "ranking";
              player->SetAttributeValue (&attr_id, tokens[3]);
            }

            g_strfreev (tokens);
          }
        }

        if (line)
        {
          if (player)
          {
            Add (player);
            player->Release ();
          }

          g_strfreev (line);
        }
      }

      g_strfreev (lines);
    }
    g_free (iso_8859_file);
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

          attr_id._name = (gchar *) "attending";
          player->SetAttributeValue (&attr_id, (guint) FALSE);

          attr_id._name = (gchar *) "name";
          player->SetAttributeValue (&attr_id, tokens[i]);

          attr_id._name = (gchar *) "first_name";
          player->SetAttributeValue (&attr_id, tokens[i+1]);

          attr_id._name = (gchar *) "exported";
          player->SetAttributeValue (&attr_id, (guint) FALSE);


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
  GList  *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));

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
            const gchar *xml_image = attr_desc->GetDiscreteXmlImage (gtk_entry_get_text (GTK_ENTRY (entry)));

            player->SetAttributeValue (attr_id,
                                       xml_image);

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
            gchar *value = attr_desc->GetXmlImage (&iter);

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
  Add (player);

  player->Release ();
  OnListChanged ();
}

// --------------------------------------------------------------------------------
void Checkin::Monitor (Player *player)
{
  Player::AttributeId attr_id ("attending");

  Attribute *attr = player->GetAttribute (&attr_id);

  if (attr && (attr->GetUIntValue () == TRUE))
  {
    _attendings++;
  }

  player->SetChangeCbk ("attending",
                        (Player::OnChange) OnAttendingChanged,
                        this);
}

// --------------------------------------------------------------------------------
void Checkin::OnPlayerRemoved (Player *player)
{
  Player::AttributeId  attr_id ("attending");
  Attribute           *attr = player->GetAttribute (&attr_id);

  if (attr && (attr->GetUIntValue () == TRUE))
  {
    _attendings--;
  }
}

// --------------------------------------------------------------------------------
void Checkin::OnAttendingChanged (Player    *player,
                                  Attribute *attr,
                                  Checkin   *checkin)
{
  guint               value = attr->GetUIntValue ();
  Player::AttributeId global_status_attr_id ("global_status");

  if (value == 1)
  {
    checkin->_attendings++;
    player->SetAttributeValue (&global_status_attr_id,
                               "Q");
  }
  else if (value == 0)
  {
    checkin->_attendings--;
    player->SetAttributeValue (&global_status_attr_id,
                               "F");
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
    GList *children = gtk_container_get_children (GTK_CONTAINER (_glade->GetWidget ("value_vbox")));

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
void Checkin::OnPrint ()
{
  if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
  {
    _print_attending = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("attending_checkbutton")));
    _print_missing   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("missing_checkbutton")));

    if (_print_missing && _print_attending)
    {
      Print (gettext ("List of registered"));
    }
    else if ((_print_missing == FALSE) && _print_attending)
    {
      Print (gettext ("List of presents"));
    }
    else if ((_print_attending == FALSE) && _print_missing)
    {
      Print (gettext ("List of absents"));
    }
    else
    {
      Print (gettext ("Fencer list"));
    }
  }
  gtk_widget_hide (_print_dialog);
}

// --------------------------------------------------------------------------------
gboolean Checkin::PlayerIsPrintable (Player *player)
{
  Player::AttributeId  attr_id ("attending");
  Attribute           *attr = player->GetAttribute (&attr_id);
  gboolean             attending = attr->GetUIntValue ();

  if ((attending == TRUE) && (_print_attending == TRUE))
  {
    return TRUE;
  }
  else if ((attending == FALSE) && (_print_missing == TRUE))
  {
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_player_button_clicked (GtkWidget *widget,
                                                                 Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->on_remove_player_button_clicked ();
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
void Checkin::OnToggleAllPlayers (gboolean present)
{
  Player::AttributeId  attr_id ("attending");
  GSList              *current_player = _player_list;

  while (current_player)
  {
    Player *p;

    p = (Player *) current_player->data;

    if (p)
    {
      p->SetAttributeValue (&attr_id,
                            (guint) present);

      Update (p);
    }
    current_player = g_slist_next (current_player);
  }
  OnListChanged ();
}

// --------------------------------------------------------------------------------
void Checkin::SetSelectorValue (GtkComboBox *combo_box,
                                const gchar *value)
{
  GtkTreeModel *model = gtk_combo_box_get_model (combo_box);

  if (value && model)
  {
    GtkTreeIter  iter;
    gboolean     iter_is_valid;
    gchar       *case_value = g_utf8_casefold (value, -1);

    iter_is_valid = gtk_tree_model_get_iter_first (model,
                                                   &iter);

    while (iter_is_valid)
    {
      gchar *current_value;
      gchar *case_current_value;

      gtk_tree_model_get (model, &iter,
                          AttributeDesc::DISCRETE_USER_IMAGE, &current_value,
                          -1);
      case_current_value = g_utf8_casefold (current_value, -1);
      if (current_value && (g_ascii_strcasecmp (case_current_value, case_value) == 0))
      {
        g_free (case_current_value);
        gtk_combo_box_set_active_iter (combo_box,
                                       &iter);
        break;
      }

      g_free (case_current_value);
      iter_is_valid = gtk_tree_model_iter_next (model,
                                                &iter);
    }

    g_free (case_value);
  }
}

// --------------------------------------------------------------------------------
gboolean Checkin::OnSelectorChanged (GtkEntryCompletion *widget,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter,
                                     GtkComboBox        *combobox)
{
  gchar *value;

  gtk_tree_model_get (model, iter,
                      AttributeDesc::DISCRETE_USER_IMAGE, &value,
                      -1);
  SetSelectorValue (combobox,
                    value);
  return FALSE;
}

// --------------------------------------------------------------------------------
void Checkin::OnSelectorEntryActivate (GtkEntry    *widget,
                                       GtkComboBox *combobox)
{
  SetSelectorValue (combobox,
                    gtk_entry_get_text (widget));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_players_list_filter_button_clicked (GtkWidget *widget,
                                                                       Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_import_toolbutton_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->OnImport ();
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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_checkin_print_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->OnPrint ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_all_present_button_clicked (GtkWidget *widget,
                                                               Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->OnToggleAllPlayers (TRUE);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_all_absent_button_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->OnToggleAllPlayers (FALSE);
}
