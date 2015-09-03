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

#include "util/global.hpp"
#include "util/attribute.hpp"
#include "util/player.hpp"
#include "../../contest.hpp"
#include "../../classification.hpp"

#include "general_classification.hpp"

namespace People
{
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

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("final_rank");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("league");
      filter->ShowAttribute ("country");

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
                        CreateInstance);
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
  gboolean GeneralClassification::HasItsOwnRanking ()
  {
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GSList *GeneralClassification::GetCurrentClassification ()
  {
    Player::AttributeId final_rank_attr_id ("final_rank");
    GSList *full_result = g_slist_copy (_attendees->GetGlobalList ());
    GSList *result      = NULL;

    GiveShortListAFinalRank ();

    // Sort final list
    if (full_result)
    {
      final_rank_attr_id.MakeRandomReady (_rand_seed);
      full_result = g_slist_sort_with_data (full_result,
                                            (GCompareDataFunc) Player::Compare,
                                            &final_rank_attr_id);
    }

    // Remove black carded and exported
    {
      guint   excluded_count = 0;
      guint   fencer_count   = g_slist_length (full_result);
      GSList *current        = full_result;

      while (current)
      {
        Player::AttributeId  exported_attr_id ("exported");
        Player    *player   = (Player *) current->data;
        Attribute *exported = player->GetAttribute (&exported_attr_id);

        if (exported && (exported->GetUIntValue () == TRUE))
        {
          excluded_count++;
        }
        else
        {
          Player::AttributeId  status_attr_id ("global_status");
          Attribute           *status_attr = player->GetAttribute (&status_attr_id);

          if (status_attr)
          {
            gchar *status = status_attr->GetStrValue ();

            if (status && (*status == 'E'))
            {
              excluded_count++;
            }
            else
            {
              Attribute *rank_attr = player->GetAttribute (&final_rank_attr_id);

              if (rank_attr)
              {
              player->SetAttributeValue (&final_rank_attr_id,
                                         rank_attr->GetUIntValue () - excluded_count);
              }
              else
              {
                player->SetAttributeValue (&final_rank_attr_id,
                                           fencer_count - excluded_count);
              }
              result = g_slist_append (result,
                                       player);
            }
          }
        }

        current = g_slist_next (current);
      }
    }

    g_slist_free (full_result);

    return result;
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::GiveShortListAFinalRank ()
  {
    Player::AttributeId rank_attr_id       ("rank",   GetPreviousStage ());
    Player::AttributeId final_rank_attr_id ("final_rank");
    GSList *current = _attendees->GetShortList ();

    while (current)
    {
      Player    *player    = (Player *) current->data;
      Attribute *rank_attr = player->GetAttribute ( &rank_attr_id);

      player->SetAttributeValue (&final_rank_attr_id,
                                 rank_attr->GetUIntValue ());

      current = g_slist_next (current);
    }
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
    gchar *filename          = NULL;
    gchar *filename_modifier = (gchar *) "";

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
        gchar *last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
                                                     "Competiton",
                                                     "export_dir",
                                                     NULL);
        if (last_dirname == NULL)
        {
          last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
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

        if (export_type == FFF)
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
        else if (export_type == HTML)
        {
          gtk_file_filter_set_name (filter,
                                    gettext ("All HTML files (.HTML)"));
          pattern_upper     = "*.HTML";
          filename_modifier = gettext (_class_name);
        }
        else
        {
          return;
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

          export_name = g_strdup_printf ("%s%s.%s", cotcot_name, filename_modifier, pattern_lower+2);

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

          g_key_file_set_string (Global::_user_config->_key_file,
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
        if (export_type == FFF)
        {
          classification->DumpToFFF (filename,
                                     _contest);
        }
        else if (export_type == PDF)
        {
          _contest->PrintPDF (gettext ("General classification"),
                              filename);
        }
        else if (export_type == HTML)
        {
          _contest->DumpToHTML (filename);
        }
      }
      g_free (filename);
    }
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::DumpToHTML (FILE *file)
  {
    Classification *classification = GetClassification ();

    classification->DumpToHTML (file);
  }

  // --------------------------------------------------------------------------------
  gchar *GeneralClassification::GetPrintName ()
  {
    return g_strdup_printf (gettext ("General classification"));
  }

  // --------------------------------------------------------------------------------
  guint GeneralClassification::PreparePrint (GtkPrintOperation *operation,
                                             GtkPrintContext   *context)
  {
    if (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
    {
      return 0;
    }
    else
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        return classification->PreparePrint (operation,
                                             context);
      }
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::DrawPage (GtkPrintOperation *operation,
                                        GtkPrintContext   *context,
                                        gint               page_nr)
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      DrawContainerPage (operation,
                         context,
                         page_nr);

      classification->DrawBarePage (operation,
                                    context,
                                    page_nr);
    }
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::OnEndPrint (GtkPrintOperation *operation,
                                          GtkPrintContext   *context)
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->OnEndPrint (operation,
                                  context);
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_general_classification_filter_button_clicked (GtkWidget *widget,
                                                                                   Object    *owner)
  {
    GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

    g->OnFilterClicked (NULL);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_general_classification_print_toolbutton_clicked (GtkWidget *widget,
                                                                                      Object    *owner)
  {
    GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

    g->OnPrintPoolToolbuttonClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_export_html_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
  {
    GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

    g->OnExportToolbuttonClicked (GeneralClassification::HTML);
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
}
