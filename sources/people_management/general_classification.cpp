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
#include "contest.hpp"
#include "classification.hpp"
#include "player.hpp"

#include "general_classification.hpp"

const gchar *GeneralClassification::_class_name     = N_ ("General classification");
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
#ifndef DEBUG
                               "ref",
#endif
                               "availability",
                               "participation_rate",
                               "level",
                               "global_status",
                               "status",
                               "start_rank",
                               "rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
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
void GeneralClassification::Declare ()
{
  RegisterStageClass (gettext (_class_name),
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
  ToggleClassification (TRUE);
}

// --------------------------------------------------------------------------------
GSList *GeneralClassification::GetCurrentClassification ()
{
  GSList *result = NULL;

  {
    GSList *current = _attendees->GetGlobalList ();

    while (current)
    {
      Player *player;

      player = (Player *) current->data;
      {
        Player::AttributeId  exported_attr_id ("exported");
        Attribute           *exported = player->GetAttribute (&exported_attr_id);

        if ((exported == NULL) || (exported->GetUIntValue () == FALSE))
        {
          Player::AttributeId  status_attr_id ("global_status");
          Attribute           *status_attr = player->GetAttribute (&status_attr_id);

          if (status_attr)
          {
            gchar *status = status_attr->GetStrValue ();

            if (status && (*status != 'E'))
            {
              result = g_slist_prepend (result,
                                        player);
            }
          }
        }
      }
      current = g_slist_next (current);
    }
  }

  if (result)
  {
    Player::AttributeId attr_id ("final_rank");

    attr_id.MakeRandomReady (_rand_seed);
    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) Player::Compare,
                                     &attr_id);
  }
  return result;
}

// --------------------------------------------------------------------------------
void GeneralClassification::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  RetrieveAttendees ();
}

// --------------------------------------------------------------------------------
void GeneralClassification::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);

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
    classification->Print (gettext ("General classification"));
  }
}

// --------------------------------------------------------------------------------
void GeneralClassification::OnExportToolbuttonClicked (ExportType export_type)
{
  char *filename = NULL;

  {
    GtkWidget *chooser;

    chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a target file..."),
                                                       NULL,
                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                       GTK_STOCK_CANCEL,
                                                       GTK_RESPONSE_CANCEL,
                                                       GTK_STOCK_SAVE_AS,
                                                       GTK_RESPONSE_ACCEPT,
                                                       NULL));
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser),
                                                    TRUE);

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
      const gchar   *pattern_upper;
      gchar         *pattern_lower;

      if (export_type == CSV)
      {
        gtk_file_filter_set_name (filter,
                                  gettext ("All Excel files (.CSV)"));
        pattern_upper = "*.CVS";
      }
      else if (export_type == FFF)
      {
        gtk_file_filter_set_name (filter,
                                  gettext ("All FFF files (.FFF)"));
        pattern_upper = "*.FFF";
      }
      else if (export_type == PDF)
      {
        gtk_file_filter_set_name (filter,
                                  gettext ("All PDF files (.PDF)"));
        pattern_upper = "*.PDF";
      }

      gtk_file_filter_add_pattern (filter,
                                   pattern_upper);

      pattern_lower = g_ascii_strdown (pattern_upper, -1);
      gtk_file_filter_add_pattern (filter,
                                   pattern_lower);

      gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                   filter);

      {
        gchar *cotcot_name = _contest->GetDefaultFileName ();
        gchar *suffix      = g_strrstr (cotcot_name, ".cotcot");
        gchar *export_name;

        if (suffix)
        {
          *suffix = 0;
        }

        export_name = g_strdup_printf ("%s.%s", cotcot_name, pattern_lower+2);

        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
                                           export_name);

        g_free (export_name);
        g_free (cotcot_name);
      }
      g_free (pattern_lower);
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

    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      if (filename)
      {
        gchar *dirname = g_path_get_dirname (filename);

        g_key_file_set_string (_config_file,
                               "Competiton",
                               "export_dir",
                               dirname);
        g_free (dirname);
      }
    }

    gtk_widget_destroy (chooser);
  }

  if (filename)
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      if (export_type == CSV)
      {
        classification->DumpToCSV (filename,
                                   _filter->GetAttrList ());
      }
      else if (export_type == FFF)
      {
        classification->DumpToFFF (filename,
                                   _contest);
      }
      if (export_type == PDF)
      {
        classification->PrintPDF (gettext ("General classification"),
                                  filename);
      }
    }
    g_free (filename);
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
extern "C" G_MODULE_EXPORT void on_export_csv_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnExportToolbuttonClicked (GeneralClassification::CSV);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_export_fff_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnExportToolbuttonClicked (GeneralClassification::FFF);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_export_pdf_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

  g->OnExportToolbuttonClicked (GeneralClassification::PDF);
}
