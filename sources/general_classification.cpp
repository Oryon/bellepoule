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

#include <string.h>

#include "attribute.hpp"
#include "classification.hpp"
#include "player.hpp"

#include "general_classification.hpp"

const gchar *GeneralClassification::_class_name     = "Classement général";
const gchar *GeneralClassification::_xml_class_name = "ClassementGeneral";

// --------------------------------------------------------------------------------
GeneralClassification::GeneralClassification (StageClass *stage_class)
: Stage (stage_class),
  PlayersList ("general_classification.glade",
               NO_RIGHT)
{
  // Player attributes to display
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "HS",
                               "previous_stage_rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("final_rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");

    SetFilter (filter);
    SetClassificationFilter (filter);
    filter->Release ();
  }
}

// --------------------------------------------------------------------------------
GeneralClassification::~GeneralClassification ()
{
}

// --------------------------------------------------------------------------------
void GeneralClassification::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      0);
}

// --------------------------------------------------------------------------------
Stage *GeneralClassification::CreateInstance (StageClass *stage_class)
{
  return new GeneralClassification (stage_class);
}

// --------------------------------------------------------------------------------
void GeneralClassification::Display ()
{
  {
    GSList *current = _attendees;

    for (guint i = 0; current != NULL; i++)
    {
      Player              *player;
      Player::AttributeId  attr_id ("final_rank");

      player = (Player *) current->data;

      player->SetAttributeValue (&attr_id,
                                 i+1);

      current = g_slist_next (current);
    }
  }

  ToggleClassification (TRUE);
}

// --------------------------------------------------------------------------------
GSList *GeneralClassification::GetCurrentClassification ()
{
  return g_slist_copy (_attendees);
}

// --------------------------------------------------------------------------------
void GeneralClassification::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) xml_node->name, _xml_class_name) == 0)
      {
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        LoadAttendees (n);
      }
      else
      {
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void GeneralClassification::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);
  SaveAttendees (xml_writer);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void GeneralClassification::OnFilterClicked ()
{
  Classification *classification = GetClassification ();

  if (classification)
  {
    classification->SelectAttributes ();
  }
}

// --------------------------------------------------------------------------------
void GeneralClassification::OnPrintPoolToolbuttonClicked ()
{
  Classification *classification = GetClassification ();

  if (classification)
  {
    classification->Print ("Classement général");
  }
}

// --------------------------------------------------------------------------------
void GeneralClassification::OnExportToolbuttonClicked ()
{
  char *filename = NULL;

  {
    GtkWidget *chooser;

    chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choisissez un fichier de destination ...",
                                                       GTK_WINDOW (_glade->GetRootWidget ()),
                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                       GTK_STOCK_CANCEL,
                                                       GTK_RESPONSE_CANCEL,
                                                       GTK_STOCK_SAVE_AS,
                                                       GTK_RESPONSE_ACCEPT,
                                                       NULL));
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser),
                                                    true);

    {
      gchar *last_dirname = g_key_file_get_string (_config_file,
                                                   "Competiton",
                                                   "export_dir",
                                                   NULL);
      if (last_dirname == NULL)
      {
        last_dirname = g_key_file_get_string (_config_file,
                                              "Competiton",
                                              "default_dir_name",
                                              NULL);
      }
      if (last_dirname)
      {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                             last_dirname);

        g_free (last_dirname);
      }
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

    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      if (filename)
      {
        {
          gchar *dirname = g_path_get_dirname (filename);

          g_key_file_set_string (_config_file,
                                 "Competiton",
                                 "export_dir",
                                 dirname);
          g_free (dirname);
        }

        if (strcmp ((const char *) ".csv", (const char *) &filename[strlen (filename) - strlen (".csv")]) != 0)
        {
          gchar *with_suffix;

          with_suffix = g_strdup_printf ("%s.csv", filename);
          g_free (filename);
          filename = with_suffix;

          {
            gchar *dirname = g_path_get_dirname (filename);

            g_key_file_set_string (_config_file,
                                   "Competiton",
                                   "export_dir",
                                   dirname);
            g_free (dirname);
          }
        }
      }
    }

    gtk_widget_destroy (chooser);
  }

  if (filename)
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->Dump (filename,
                            _filter->GetAttrList ());
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_general_classification_filter_button_clicked (GtkWidget *widget,
                                                                                 Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnFilterClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_general_classification_print_toolbutton_clicked (GtkWidget *widget,
                                                                                    Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnPrintPoolToolbuttonClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_export_toolbutton_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnExportToolbuttonClicked ();
}
