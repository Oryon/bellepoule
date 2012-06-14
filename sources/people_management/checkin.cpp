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

// --------------------------------------------------------------------------------
Checkin::Checkin (const gchar *glade,
                  const gchar *player_tag)
: PlayersList (glade)
{
  _form        = NULL;
  _attendings  = 0;
  _player_tag  = player_tag;
  _players_tag = g_strdup_printf ("%ss", _player_tag);

  {
    GtkWidget *content_area;

    _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        gettext ("<b><big>What do you want to print?</big></b>"));

    gtk_window_set_title (GTK_WINDOW (_print_dialog),
                          gettext ("Fencer list printing"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_dialog-vbox"),
                         content_area);
  }

  RefreshAttendingDisplay ();
}

// --------------------------------------------------------------------------------
Checkin::~Checkin ()
{
  gtk_widget_destroy (_print_dialog);

  _form->Release ();

  g_free (_players_tag);
}

// --------------------------------------------------------------------------------
void Checkin::CreateForm (Filter *filter)
{
  _form = new Form (filter,
                    this,
                    GetPlayerType (),
                    (Form::AddPlayerCbk) &Checkin::OnAddPlayerFromForm);
}

// --------------------------------------------------------------------------------
void Checkin::Add (Player *player)
{
  PlayersList::Add (player);
  Monitor (player);
}

// --------------------------------------------------------------------------------
void Checkin::LoadList (xmlXPathContext *xml_context,
                        const gchar     *from_node)
{
  gchar          *path        = g_strdup_printf ("%s/%ss", from_node, _player_tag);
  xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST path, xml_context);
  xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

  if (xml_object->nodesetval->nodeNr)
  {
    LoadList (xml_nodeset->nodeTab[0]);
  }

  g_free (path);
  xmlXPathFreeObject (xml_object);

  OnLoaded ();
}

// --------------------------------------------------------------------------------
void Checkin::LoadList (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, _player_tag) == 0)
      {
        Player *player = new Player (GetPlayerType ());

        player->Load (n);
        Add (player);
        OnPlayerLoaded (player);
        player->Release ();
      }
      else if (strcmp ((char *) n->name, _players_tag) != 0)
      {
        OnListChanged ();
        return;
      }
    }
    LoadList (n->children);
  }
}

// --------------------------------------------------------------------------------
void Checkin::SaveList (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _players_tag);

  {
    GSList *current = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      if (p)
      {
        p->Save (xml_writer,
                 _player_tag);
      }
      current = g_slist_next (current);
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Checkin::OnListChanged ()
{
  RefreshAttendingDisplay ();
  PlayersList::OnListChanged ();
}

// --------------------------------------------------------------------------------
gboolean Checkin::PresentPlayerFilter (Player *player)
{
  Player::AttributeId  attr_id ("attending");
  Attribute           *attr = player->GetAttribute (&attr_id);

  return ((attr == NULL) || (attr->GetUIntValue () == TRUE));
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

    if (GetPlayerType () == Player::FENCER)
    {
      gtk_file_filter_set_name (filter,
                                gettext ("Fencer files"));
    }
    else
    {
      gtk_file_filter_set_name (filter,
                                gettext ("Referee files"));
    }
    gtk_file_filter_add_pattern (filter,
                                 "*.FFF");
    gtk_file_filter_add_pattern (filter,
                                 "*.CSV");
    gtk_file_filter_add_pattern (filter,
                                 "*.TXT");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

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

      if (   g_str_has_suffix (filename, ".fff")
          || g_str_has_suffix (filename, ".FFF"))
      {
        ImportFFF (filename);
      }
      else
      {
        ImportCSV (filename);
      }
    }
  }

  gtk_widget_destroy (chooser);

  OnListChanged ();
}

// --------------------------------------------------------------------------------
gchar *Checkin::GetFileContent (gchar *filename)
{
  gchar *file_content;
  gchar *raw_file;
  gsize  length;
  guint  j;

  g_file_get_contents ((const gchar *) filename,
                       &raw_file,
                       &length,
                       NULL);
  length++;

  file_content = (gchar *) g_malloc (length);

  j = 0;
  for (guint i = 0; i < length; i++)
  {
    if (raw_file[i] != '\r')
    {
      file_content[j] = raw_file[i];
      j++;
    }
  }

  return file_content;
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
void Checkin::ImportFFF (gchar *filename)
{
  gchar *file_content  = GetFileContent (filename);
  gchar *iso_8859_file = ConvertToUtf8 (file_content);

  g_free (file_content);
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

          player = new Player (GetPlayerType ());

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
              AttributeDesc *league_desc = AttributeDesc::GetDescFromCodeName ("league");

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
void Checkin::ImportCSV (gchar *filename)
{
  gchar *file_content  = GetFileContent (filename);
  gchar *iso_8859_file = ConvertToUtf8 (file_content);

  g_free (file_content);
  if (iso_8859_file)
  {
    gchar **header_line = g_strsplit_set (iso_8859_file,
                                          "\n",
                                          0);

    if (header_line)
    {
      guint           nb_attr = 0;
      AttributeDesc **columns = NULL;

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

          columns = g_new (AttributeDesc *, nb_attr);
          for (guint i = 0; i < nb_attr; i++)
          {
            columns[i] = AttributeDesc::GuessDescFromUserName (header_attr[i]);
          }

          g_strfreev (header_attr);
        }

        g_strfreev (header_line);
      }

      // Fencers
      {
        gchar **tokens = g_strsplit_set (iso_8859_file,
                                         ";\n",
                                         0);

        if (tokens)
        {
          for (guint i = nb_attr; tokens[i+1] != 0; i += nb_attr)
          {
            Player              *player = new Player (GetPlayerType ());
            Player::AttributeId  attr_id ("");

            for (guint c = 0; c < nb_attr; c++)
            {
              if (columns[c])
              {
                attr_id._name = columns[c]->_code_name;
                player->SetAttributeValue (&attr_id, tokens[i+c]);
              }
            }

            Add (player);
            player->Release ();
          }
          g_strfreev (tokens);
        }
      }

      g_free (columns);
    }
  }
  g_free (iso_8859_file);
}

// --------------------------------------------------------------------------------
void Checkin::OnAddPlayerFromForm (Player *player)
{
  Add (player);
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
void Checkin::OnPlugged ()
{
  OnAttrListUpdated ();
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
                                  Object    *owner)
{
  Checkin *checkin = dynamic_cast <Checkin *> (owner);
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
  _form->Show ();
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
Player::PlayerType Checkin::GetPlayerType ()
{
  if (strcmp (_player_tag, "Arbitre") == 0)
  {
    return Player::REFEREE;
  }
  else
  {
    return Player::FENCER;
  }
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
