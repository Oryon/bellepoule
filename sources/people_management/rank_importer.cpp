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

#include <libxml/encoding.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>

#include "rank_importer.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  RankImporter::RankImporter (GKeyFile *config_file)
    : Object ("RankImporter")
  {
    _rank_table = NULL;
    DisplayChooser (config_file);
  }

  // --------------------------------------------------------------------------------
  RankImporter::~RankImporter ()
  {
    if (_rank_table)
    {
      g_hash_table_destroy (_rank_table);
    }
  }

  // --------------------------------------------------------------------------------
  void RankImporter::UpdateWorstRank (guint rank_candidate)
  {
    if (   (_worst_rank == 0)
        || (_worst_rank < rank_candidate))
    {
      _worst_rank = rank_candidate;
    }
  }

  // --------------------------------------------------------------------------------
  void RankImporter::ModifyRank (Player *fencer)
  {
    if (_rank_table)
    {
      Attribute           *attr;
      Player::AttributeId  ranking_attr_id ("ranking");
      Player::AttributeId  name_attr_id    ("name");
      Player::AttributeId  first_attr_id   ("first_name");
      gchar               *name;
      gchar               *first_name;
      gchar               *id;
      guint                new_rank;

      attr = fencer->GetAttribute (&name_attr_id);
      name = attr->GetUserImage (AttributeDesc::LONG_TEXT);

      attr = fencer->GetAttribute (&first_attr_id);
      first_name = attr->GetUserImage (AttributeDesc::LONG_TEXT);

      id = g_strdup_printf ("%s:%s", name, first_name);
      new_rank = GPOINTER_TO_UINT (g_hash_table_lookup (_rank_table,
                                                        id));
      if (new_rank == 0)
      {
        new_rank = _worst_rank+1;
      }

      fencer->SetAttributeValue (&ranking_attr_id,
                                 new_rank);

      g_free (id);
      g_free (first_name);
      g_free (name);
    }
  }

  // --------------------------------------------------------------------------------
  void RankImporter::DisplayChooser (GKeyFile *config_file)
  {
    GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext ("Choose a rank file to apply..."),
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
                                gettext ("Rank files: cotcot, xml, txt (France)"));

      gtk_file_filter_add_pattern (filter,
                                   "*.TXT");
      gtk_file_filter_add_pattern (filter,
                                   "*.txt");
      gtk_file_filter_add_pattern (filter,
                                   "*.XML");
      gtk_file_filter_add_pattern (filter,
                                   "*.xml");
      gtk_file_filter_add_pattern (filter,
                                   "*.cotcot");
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
      gchar *last_dirname = g_key_file_get_string (config_file,
                                                   "CheckinSupervisor",
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

          g_key_file_set_string (config_file,
                                 "CheckinSupervisor",
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

        _rank_table = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             g_free,
                                             NULL);

        _worst_rank = 0;

        if (   g_str_has_suffix (filename, ".cotcot")
            || g_str_has_suffix (filename, ".xml")
            || g_str_has_suffix (filename, ".XML"))
        {
          LoadXml (filename);
        }
        else
        {
          LoadTxt (filename);
        }
      }
    }

    gtk_widget_destroy (chooser);
  }

  // --------------------------------------------------------------------------------
  void RankImporter::LoadTxt (const gchar *filename)
  {
    gchar *file_content;

    if (g_file_get_contents (filename,
                             &file_content,
                             NULL,
                             NULL))
    {
      gchar *utf8_content = g_convert (file_content,
                                       -1,
                                       "UTF-8",
                                       "ISO-8859-1",
                                       NULL,
                                       NULL,
                                       NULL);

      g_free (file_content);
      if (utf8_content)
      {
        gchar *fencers = g_strstr_len (utf8_content, -1, "TIREURS");

        if (fencers)
        {
          gchar **lines;

          fencers = fencers + strlen ("TIREURS") + 1;
          lines = g_strsplit_set (fencers,
                                  "\n",
                                  0);

          if (lines)
          {
            for (guint l = 0; lines[l] != NULL; l++)
            {
              gchar **tokens = g_strsplit_set (lines[l],
                                               "/",
                                               0);

              if (tokens && tokens[0])
              {
                gchar       *name       = NULL;
                gchar       *first_name = NULL;
                const gchar *rank       = NULL;
                guint        rank_value;

                for (guint t = 0; tokens[t] != NULL; t++)
                {
                  if (t == 0)
                  {
                    name = GetUndivadableText (tokens[t]);
                  }
                  else if (t == 1)
                  {
                    first_name = GetUndivadableText (tokens[t]);
                  }
                  rank = tokens[t];
                }
                rank_value = atoi (rank);
                g_hash_table_insert (_rank_table,
                                     g_strdup_printf ("%s:%s", name, first_name),
                                     (gpointer) rank_value);
                g_free (name);
                g_free (first_name);
                UpdateWorstRank (rank_value);

                g_strfreev (tokens);
              }
            }
            g_strfreev (lines);
          }
        }

        g_free (utf8_content);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void RankImporter::LoadXml (const gchar *filename)
  {
    xmlDoc *doc = xmlParseFile (filename);

    if (doc)
    {
      xmlXPathInit ();

      {
        xmlXPathContext *xml_context;
        xmlXPathObject  *xml_object;

        xml_context = xmlXPathNewContext (doc);
        xml_object  = xmlXPathEval (BAD_CAST "/CompetitionIndividuelle/Tireurs", xml_context);

        if (xml_object->nodesetval->nodeNr)
        {
          xmlNodeSet *xml_nodeset = xml_object->nodesetval;

          LoadList (xml_nodeset->nodeTab[0]);
        }

        xmlXPathFreeObject  (xml_object);
        xmlXPathFreeContext (xml_context);
      }

      xmlFreeDoc (doc);
    }
  }

  // --------------------------------------------------------------------------------
  void RankImporter::LoadList (xmlNode *xml_node)
  {
    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) n->name, "Tireurs") == 0)
        {
        }
        else if (strcmp ((char *) n->name, "Tireur") == 0)
        {
          gchar *rank                   = (gchar *) xmlGetProp (n, BAD_CAST "Classement");
          guint  rank_value             = atoi (rank);
          gchar *name                   = (gchar *) xmlGetProp (n, BAD_CAST "Nom");
          gchar *first_name             = (gchar *) xmlGetProp (n, BAD_CAST "Prenom");
          gchar *name_undivadabel       = GetUndivadableText (name);
          gchar *first_name_undivadabel = GetUndivadableText (first_name);

          g_hash_table_insert (_rank_table,
                               g_strdup_printf ("%s:%s", name_undivadabel, first_name_undivadabel),
                               (gpointer) rank_value);
          UpdateWorstRank (rank_value);

          g_free (name_undivadabel);
          g_free (first_name_undivadabel);

          xmlFree (name);
          xmlFree (first_name);
          xmlFree (rank);
        }
        else
        {
          return;
        }
      }

      LoadList (n->children);
    }
  }
}
