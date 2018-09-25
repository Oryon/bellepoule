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
#include "util/attribute.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/user_config.hpp"
#include "util/glade.hpp"
#include "util/xml_scheme.hpp"
#include "actors/player_factory.hpp"
#include "actors/tally_counter.hpp"
#include "tally_counter.hpp"

#include "checkin.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  Checkin::Checkin (const gchar *glade,
                    const gchar *base_class,
                    const gchar *gathering_class)
    : Object ("Checkin"),
    PlayersList (glade)
  {
    _form            = NULL;
    _tally_counter   = new TallyCounter ();
    _base_class      = base_class;
    _gathering_class = gathering_class;
    _listener        = NULL;

    RefreshAttendingDisplay ();
  }

  // --------------------------------------------------------------------------------
  Checkin::~Checkin ()
  {
    Object::TryToRelease (_form);
    _tally_counter->Release ();
  }

  // --------------------------------------------------------------------------------
  Form *Checkin::CreateForm (Filter      *filter,
                             const gchar *player_class)
  {
    if (_form == NULL)
    {
      _form = new Form (this,
                        filter,
                        player_class);
    }

    return _form;
  }

  // --------------------------------------------------------------------------------
  void Checkin::Add (Player *player)
  {
    PlayersList::Add (player);
    Monitor (player);
  }

  // --------------------------------------------------------------------------------
  xmlNode *Checkin::GetXmlNode (xmlXPathContext *xml_context,
                                const gchar     *from_node,
                                const gchar     *player_class)
  {
    xmlNode        *node                  = NULL;
    const gchar    *player_class_xml_tag  = PlayerFactory::GetXmlTag (player_class);
    gchar          *players_class_xml_tag = g_strdup_printf ("%ss", player_class_xml_tag);
    gchar          *path                  = g_strdup_printf ("%s/%s", from_node, players_class_xml_tag);
    xmlXPathObject *xml_object            = xmlXPathEval (BAD_CAST path, xml_context);
    xmlNodeSet     *xml_nodeset           = xml_object->nodesetval;

    if (xml_object->nodesetval->nodeNr)
    {
      node = xml_nodeset->nodeTab[0];
    }

    xmlXPathFreeObject (xml_object);
    g_free (path);
    g_free (players_class_xml_tag);

    return node;
  }

  // --------------------------------------------------------------------------------
  void Checkin::LoadList (xmlXPathContext *xml_context,
                          const gchar     *from_node,
                          const gchar     *player_class)
  {
    if (player_class == NULL)
    {
      player_class = _base_class;
    }

    xmlNode *node = GetXmlNode (xml_context,
                                from_node,
                                player_class);

    if (node)
    {
      const gchar *player_class_xml_tag  = PlayerFactory::GetXmlTag (player_class);
      gchar       *players_class_xml_tag = g_strdup_printf ("%ss", player_class_xml_tag);

      LoadList (node,
                player_class,
                player_class_xml_tag,
                players_class_xml_tag);

      g_free (players_class_xml_tag);
    }
  }

  // --------------------------------------------------------------------------------
  void Checkin::LoadList (xmlNode     *xml_node,
                          const gchar *player_class,
                          const gchar *player_class_xml_tag,
                          const gchar *players_class_xml_tag,
                          Player      *owner)
  {
    const gchar *base_class_xml_tag = PlayerFactory::GetXmlTag (_base_class);

    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (g_strcmp0 ((char *) n->name, player_class_xml_tag) == 0)
        {
          owner = LoadPlayer (n,
                              player_class,
                              NULL);
        }
        else if (   _gathering_class
                 && (g_strcmp0 (player_class, _gathering_class) == 0)
                 && (g_strcmp0 ((char *) n->name, base_class_xml_tag) == 0))
        {
          LoadPlayer (n,
                      _base_class,
                      owner);
        }
        else if (g_strcmp0 ((char *) n->name, players_class_xml_tag) != 0)
        {
          owner = NULL;
          OnListChanged ();
          return;
        }
      }
      LoadList (n->children,
                player_class,
                player_class_xml_tag,
                players_class_xml_tag,
                owner);
    }
  }

  // --------------------------------------------------------------------------------
  Player *Checkin::LoadPlayer (xmlNode     *xml_node,
                               const gchar *player_class,
                               Player      *owner)
  {
    Player *player = PlayerFactory::CreatePlayer (player_class);

    player->Load (xml_node);

    RegisterPlayer (player,
                    owner);

    return player;
  }

  // --------------------------------------------------------------------------------
  void Checkin::RegisterPlayer (Player *player,
                                Player *owner)
  {
    // FFE issue
    GuessPlayerOrganization (player,
                             "league");
    GuessPlayerOrganization (player,
                             "region");

    OnPlayerLoaded (player,
                    owner);
    Add (player);
    player->Release ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::GuessPlayerOrganization (Player      *player,
                                         const gchar *organization)
  {
    Player::AttributeId  country_attr_id ("country");
    Attribute           *country_attr = player->GetAttribute (&country_attr_id);

    if (country_attr && (g_ascii_strcasecmp  (country_attr->GetStrValue (), "FRA") == 0))
    {
      Player::AttributeId  licence_attr_id ("licence");
      Attribute           *licence_attr = player->GetAttribute (&licence_attr_id);

      if (licence_attr)
      {
        gchar *licence_string = g_strdup (licence_attr->GetStrValue ());

        if (strlen (licence_string) == 12)
        {
          licence_string[2] = 0;

          AttributeDesc *organization_desc   = AttributeDesc::GetDescFromCodeName (organization);
          gchar         *player_organization = organization_desc->GetDiscreteUserImage (atoi (licence_string));

          if (organization)
          {
            Player::AttributeId  organization_attr_id (organization);

            player->SetAttributeValue (&organization_attr_id, player_organization);
            g_free (player_organization);
          }
        }
        g_free (licence_string);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Checkin::SavePlayer (XmlScheme   *xml_scheme,
                            const gchar *player_class,
                            Player      *player)
  {
    if (player && player->Is (player_class))
    {
      player->Save (xml_scheme);
    }
  }

  // --------------------------------------------------------------------------------
  void Checkin::SaveList (XmlScheme   *xml_scheme,
                          const gchar *player_class)
  {
    if (player_class == NULL)
    {
      player_class = _base_class;
    }

    {
      const gchar *player_class_xml_tag  = PlayerFactory::GetXmlTag (player_class);
      gchar       *players_class_xml_tag = g_strdup_printf ("%ss", player_class_xml_tag);

      xml_scheme->StartElement (players_class_xml_tag);

      {
        GList *current = _player_list;

        while (current)
        {
          Player *p = (Player *) current->data;

          SavePlayer (xml_scheme,
                      player_class,
                      p);

          current = g_list_next (current);
        }
      }

      xml_scheme->EndElement ();

      g_free (players_class_xml_tag);
    }
  }

  // --------------------------------------------------------------------------------
  void Checkin::OnListChanged ()
  {
    RefreshAttendingDisplay ();
    PlayersList::OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  gboolean Checkin::PresentPlayerFilter (Player      *player,
                                         PlayersList *owner)
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
      gchar         *name = g_strdup_printf ("%s files", _base_class);
      GtkFileFilter *filter = gtk_file_filter_new ();

#define GETTEXT_TRICK N_ ("Fencer files") N_ ("Referee files")
      gtk_file_filter_set_name (filter,
                                gettext (name));

      gtk_file_filter_add_pattern (filter,
                                   "*.FFF");
      gtk_file_filter_add_pattern (filter,
                                   "*.fff");
      gtk_file_filter_add_pattern (filter,
                                   "*.CSV");
      gtk_file_filter_add_pattern (filter,
                                   "*.csv");
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
      const gchar *prg_name = g_get_prgname ();

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
      gchar *last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
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

    if (RunDialog (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      if (filename)
      {
        {
          gchar *dirname = g_path_get_dirname (filename);

          g_key_file_set_string (Global::_user_config->_key_file,
                                 "Checkin",
                                 "default_import_dir_name",
                                 dirname);
          g_free (dirname);
        }

        {
          gchar *uri = g_filename_to_uri (filename,
                                          NULL,
                                          NULL);

          gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
                                       uri);
          g_free (uri);
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
    gchar *file_content = NULL;
    gchar *raw_file;
    gsize  length;

    if (g_file_get_contents ((const gchar *) filename,
                             &raw_file,
                             &length,
                             NULL))
    {
      guint j;

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

      g_free (raw_file);
    }

    return file_content;
  }

  // --------------------------------------------------------------------------------
  void Checkin::ConvertFromBaseToResult ()
  {
  }

  // --------------------------------------------------------------------------------
  void Checkin::ImportFFF (gchar *filename)
  {
    gchar  *file_content = GetFileContent (filename);
    gchar  *utf8_content;

    {
      GError *error         = NULL;
      gsize   bytes_written;

      utf8_content = g_convert (file_content,
                                -1,
                                "UTF-8",
                                "ISO-8859-1",
                                NULL,
                                &bytes_written,
                                &error);
      g_free (file_content);

      if (error)
      {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GetRootWidget ())),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    gettext ("The imported FFF file is not ISO-8859 encoded."));
        RunDialog (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_error_free (error);
      }
    }

    if (utf8_content)
    {
      gchar **lines = g_strsplit_set (utf8_content,
                                      "\n",
                                      0);

      if (lines && lines[0] && lines[1])
      {
        GDateYear current_year;

        {
          GTimeVal  time_of_day;
          GDate    *current_date = g_date_new ();

          g_get_current_time (&time_of_day);
          g_date_set_time_val (current_date,
                               &time_of_day);

          current_year = g_date_get_year (current_date);
          g_date_free (current_date);
        }

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

            player = PlayerFactory::CreatePlayer (_base_class);

            if (tokens)
            {
              attr_id._name = (gchar *) "name";
              player->SetAttributeValue (&attr_id, tokens[0]);

              attr_id._name = (gchar *) "first_name";
              player->SetAttributeValue (&attr_id, tokens[1]);

              attr_id._name = (gchar *) "birth_date";
              {
                gchar *french_date = tokens[2];
                gchar **splitted_date;

                splitted_date = g_strsplit_set (french_date,
                                                "/",
                                                0);
                if (   splitted_date
                    && splitted_date[0]
                    && splitted_date[1]
                    && splitted_date[2])
                {
                  GDateYear long_year;

                  // Short year fixing
                  {
                    if (strlen (splitted_date[2]) <= 2)
                    {
                      GDateYear short_year = (GDateYear) atoi (splitted_date[2]);

                      long_year = short_year + current_year - (current_year%100);
                      if (short_year >= (current_year%100))
                      {
                        long_year -= 100;
                      }
                    }
                    else
                    {
                      long_year = (GDateYear) atoi (splitted_date[2]);
                    }
                  }

                  // GDate --> String
                  {
                    gchar  buffer[50];
                    GDate *date = g_date_new ();

                    g_date_set_day   (date, (GDateDay)   atoi (splitted_date[0]));
                    g_date_set_month (date, (GDateMonth) atoi (splitted_date[1]));
                    g_date_set_year  (date, long_year);

                    g_date_strftime (buffer,
                                     sizeof (buffer),
                                     "%x",
                                     date);
                    player->SetAttributeValue (&attr_id, buffer);

                    g_date_free (date);
                  }
                }
                g_strfreev (splitted_date);
              }

              attr_id._name = (gchar *) "gender";
              if ((*tokens[3] == 'H') || (*tokens[3] == 'h'))
              {
                // Workaround against malformed files
                *tokens[3] = 'M';
              }
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

            GuessPlayerOrganization (player,
                                     "league");
            GuessPlayerOrganization (player,
                                     "region");
          }

          if (line)
          {
            if (player)
            {
              OnPlayerLoaded (player, NULL);
              Add (player);
              player->Release ();
            }

            g_strfreev (line);
          }
        }

        g_strfreev (lines);
      }
      g_free (utf8_content);
    }

    RefreshAttendingDisplay ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::ImportCSV (gchar *filename)
  {
    gchar  *file_content = GetFileContent (filename);
    gchar  *utf8_content;

    {
      GError *error = NULL;

      utf8_content = g_locale_to_utf8 (file_content,
                                       -1,
                                       NULL,
                                       NULL,
                                       &error);
      g_free (file_content);

      if (error)
      {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GetRootWidget ())),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    "The imported CSV file is probably based on template generated from another computer.\n\n"
                                                    "Use a template generated from this computer!");
        RunDialog (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_error_free (error);
      }
    }

    if (utf8_content)
    {
      gchar **rows = g_strsplit_set (utf8_content,
                                     "\n",
                                     0);

      if (rows && rows[0])
      {
        guint           nb_attr    = 0;
        AttributeDesc **columns    = NULL;
        const char     *delimiters = ",";

        if (strchr (rows[0], ';'))
        {
          delimiters = ";";
        }

        // Header
        {
          gchar **header_attr = g_strsplit_set (rows[0],
                                                delimiters,
                                                0);

          if (header_attr)
          {
            for (guint i = 0; header_attr[i] != NULL; i++)
            {
              g_strdelimit (header_attr[i],
                            "\"",
                            ' ');
              g_strchug  (header_attr[i]);
              g_strchomp (header_attr[i]);

              nb_attr++;
            }

            columns = g_new (AttributeDesc *, nb_attr);
            for (guint i = 0; i < nb_attr; i++)
            {
              columns[i] = AttributeDesc::GuessDescFromUserName (header_attr[i],
                                                                 "CSV ready");
            }

            g_strfreev (header_attr);
          }
        }

        // Fencers
        for (guint f = 1; rows[f] != NULL; f++)
        {
          gchar **tokens = g_strsplit_set (rows[f],
                                           delimiters,
                                           0);

          if (tokens)
          {
            Player              *player = PlayerFactory::CreatePlayer (_base_class);
            Player::AttributeId  attr_id ("");
            gboolean             has_attribute = FALSE;

            for (guint c = 0; c < nb_attr; c++)
            {
              if (tokens[c] == NULL)
              {
                break;
              }

              if (columns[c])
              {
                attr_id._name = columns[c]->_code_name;

                g_strdelimit (tokens[c],
                              "\"",
                              ' ');
                g_strchug  (tokens[c]);
                g_strchomp (tokens[c]);
                player->SetAttributeValue (&attr_id, tokens[c]);
                has_attribute = TRUE;
              }
            }

            if (has_attribute)
            {
              OnPlayerLoaded (player, NULL);
              Add (player);
            }
            player->Release ();

            g_strfreev (tokens);
          }
        }

        g_free (columns);
      }
      g_strfreev (rows);
    }
    g_free (utf8_content);

    RefreshAttendingDisplay ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::OnFormEvent (Player          *player,
                             Form::FormEvent  event)
  {
    if (event == Form::NEW_PLAYER)
    {
      {
        Player::AttributeId  attending_attr_id ("attending");
        Attribute           *attending_attr = player->GetAttribute (&attending_attr_id);
        Player::AttributeId  global_status_attr_id ("global_status");

        if (attending_attr->GetUIntValue () == 1)
        {
          player->SetAttributeValue (&global_status_attr_id,
                                     "Q");
        }
        else
        {
          player->SetAttributeValue (&global_status_attr_id,
                                     "F");
        }
      }

      Add (player);
    }
    else
    {
      Update (player);
    }

    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::Monitor (Player *player)
  {
    _tally_counter->Monitor (player);

    player->SetChangeCbk ("attending",
                          (Player::OnChange) OnAttrAttendingChanged,
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
    _tally_counter->Drop (player);
  }

  // --------------------------------------------------------------------------------
  void Checkin::OnAttrAttendingChanged (Player    *player,
                                        Attribute *attr,
                                        Object    *owner,
                                        guint      step)
  {
    Checkin             *checkin = dynamic_cast <Checkin *> (owner);
    guint                value   = attr->GetUIntValue ();
    Player::AttributeId  global_status_attr_id ("global_status");

    if (value == 1)
    {
      player->SetAttributeValue (&global_status_attr_id,
                                 "Q");
    }
    else if (value == 0)
    {
      player->SetAttributeValue (&global_status_attr_id,
                                 "F");
    }

    checkin->_tally_counter->OnAttendingChanged (player,
                                                 value);

    checkin->RefreshAttendingDisplay ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::SelectTreeMode ()
  {
    _tally_counter->SetTeamMode ();
    RefreshAttendingDisplay ();

    PlayersList::SelectTreeMode ();
  }

  // --------------------------------------------------------------------------------
  void Checkin::SelectFlatMode ()
  {
    _tally_counter->DisableTeamMode ();
    RefreshAttendingDisplay ();

    PlayersList::SelectFlatMode ();
  }

  // --------------------------------------------------------------------------------
  TallyCounter *Checkin::GetTallyCounter ()
  {
    return _tally_counter;
  }

  // --------------------------------------------------------------------------------
  void Checkin::RefreshAttendingDisplay ()
  {
    GtkLabel *label;

    label = GTK_LABEL (_glade->GetWidget ("attending_label"));
    if (label)
    {
      gchar *text = g_strdup_printf ("%d", _tally_counter->GetPresentsCount ());

      gtk_label_set_text (label,
                          text);
      g_free (text);
    }

    label = GTK_LABEL (_glade->GetWidget ("absent_label"));
    if (label)
    {
      gchar *text = g_strdup_printf ("%d", _tally_counter->GetAbsentsCount ());

      gtk_label_set_text (label,
                          text);
      g_free (text);
    }
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
  void Checkin::AddListener (Listener *listener)
  {
    _listener = listener;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_players_list_row_activated  (GtkTreeView       *tree_view,
                                                                  GtkTreePath       *path,
                                                                  GtkTreeViewColumn *column,
                                                                  Object            *owner)
  {
    Checkin *p = dynamic_cast <Checkin *> (owner);

    p->on_players_list_row_activated (path);
  }

  // --------------------------------------------------------------------------------
  void Checkin::on_players_list_row_activated (GtkTreePath *path)
  {
    if (_listener && _listener->OnPlayerListRowActivated (this))
    {
      return;
    }
    else
    {
      GtkTreeModel *model = gtk_tree_view_get_model (_tree_view);
      Player       *player;
      GtkTreeIter   iter;

      gtk_tree_model_get_iter (model,
                               &iter,
                               path);
      gtk_tree_model_get (model, &iter,
                          gtk_tree_model_get_n_columns (model) - 1,
                          &player, -1);

      if (player && (player->IsSticky () == FALSE))
      {
        _form->Show (player);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gchar *Checkin::GetPrintName ()
  {
    gchar *name;

    if (_print_missing && _print_attending)
    {
      name = g_strdup_printf ("%s (%d)",
                              gettext ("List of registered"),
                              _tally_counter->GetTotalCount ());
    }
    else if ((_print_missing == FALSE) && _print_attending)
    {
      name = g_strdup_printf ("%s (%d/%d)",
                              gettext ("List of presents"),
                              _tally_counter->GetPresentsCount (),
                              _tally_counter->GetTotalCount ());
    }
    else if ((_print_attending == FALSE) && _print_missing)
    {
      name = g_strdup_printf ("%s (%d/%d)",
                              gettext ("List of absents"),
                              _tally_counter->GetAbsentsCount (),
                              _tally_counter->GetTotalCount ());
    }
    else
    {
      name = g_strdup (gettext ("Fencer list"));
    }

    return name;
  }

  // --------------------------------------------------------------------------------
  void Checkin::OnPrint ()
  {
    GtkWidget *print_dialog = _glade->GetWidget ("print_dialog");

    if (RunDialog (GTK_DIALOG (print_dialog)) == GTK_RESPONSE_OK)
    {
      gchar *name;

      _print_attending = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("attending_checkbutton")));
      _print_missing   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("missing_checkbutton")));

      name = GetPrintName ();
      Print (name);
      g_free (name);
    }

    gtk_widget_hide (print_dialog);
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
    GList               *current_player = _player_list;

    while (current_player)
    {
      Player *p = (Player *) current_player->data;

      TogglePlayerAttr (p,
                        &attr_id,
                        present);
      current_player = g_list_next (current_player);
    }
    OnListChanged ();
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
}
