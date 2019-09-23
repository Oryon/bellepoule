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
#include <libxml/xpath.h>

#include "util/global.hpp"
#include "util/attribute.hpp"
#include "util/player.hpp"
#include "util/filter.hpp"
#include "util/user_config.hpp"
#include "util/glade.hpp"
#include "network/advertiser.hpp"
#include "../../attendees.hpp"
#include "../../contest.hpp"
#include "../../classification.hpp"

#include "general_classification.hpp"

namespace People
{
  const gchar *GeneralClassification::_class_name     = N_ ("General classification");
  const gchar *GeneralClassification::_xml_class_name = "ClassementGeneral";

  // --------------------------------------------------------------------------------
  GeneralClassification::GeneralClassification (StageClass *stage_class)
    : Object ("GeneralClassification"),
      Stage (stage_class),
      PlayersList ("general_classification.glade", NO_RIGHT)
  {
    // Player attributes to display
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("final_rank");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("league");
      filter->ShowAttribute ("region");
      filter->ShowAttribute ("country");

      SetFilter (filter);
      SetClassificationFilter (filter);
      filter->Release ();
    }

    {
      GdkColor *color = g_new (GdkColor, 1);

      gdk_color_parse ("#9DB8D2", color); // blue hilight

      gtk_widget_modify_bg (_glade->GetWidget ("place_shifting"),
                            GTK_STATE_NORMAL,
                            color);
      g_free (color);

      _place_entry_handle = g_signal_connect (_glade->GetWidget ("place_entry"),
                                              "insert-text",
                                              G_CALLBACK (on_place_entry_insert_text),
                                              this);
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

    gtk_widget_set_sensitive (_glade->GetWidget ("export_frd_toolbutton"),
                              _contest->HasAskFredEntry ());
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
    GSList *full_result = g_slist_copy (_attendees->GetPresents ());
    GSList *result      = nullptr;

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

    for (GSList *current = GetShortList (); current; current = g_slist_next (current))
    {
      Player    *player    = (Player *) current->data;
      Attribute *rank_attr = player->GetAttribute ( &rank_attr_id);

      player->SetAttributeValue (&final_rank_attr_id,
                                 rank_attr->GetUIntValue ());
    }
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);

    RetrieveAttendees ();
  }

  // --------------------------------------------------------------------------------
  void GeneralClassification::SaveAttendees (XmlScheme *xml_scheme)
  {
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
    gchar *filename          = nullptr;
    gchar *filename_modifier = (gchar *) "";

    {
      GtkWidget *chooser = _glade->GetWidget ("filechooserdialog");

      {
        gchar *last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
                                                     "Competiton",
                                                     "export_dir",
                                                     nullptr);
        if (last_dirname == nullptr)
        {
          last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
                                                "Competiton",
                                                "default_dir_name",
                                                nullptr);
        }
        if (last_dirname)
        {
          gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                               last_dirname);

          g_free (last_dirname);
        }
      }

      gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (chooser),
                                         nullptr);

      {
        GtkFileFilter *filter = gtk_file_filter_new ();
        const gchar   *pattern_upper;
        gchar         *pattern_lower;

        if (export_type == FFF)
        {
          gtk_file_filter_set_name (filter,
                                    gettext ("All FFF files (.FFF)"));
          pattern_upper = "*.FFF";

          gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (chooser),
                                             _glade->GetWidget ("place_shifting"));
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
        else if (export_type == FRD)
        {
          gtk_file_filter_set_name (filter,
                                    gettext ("All AskFred files (.FRD)"));
          pattern_upper = "*.FRD";
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

      if (RunDialog (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
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

      gtk_widget_hide (chooser);
    }

    if (filename)
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        if (export_type == FFF)
        {
          GtkEntry *entry          = (GtkEntry *) _glade->GetWidget ("place_entry");
          guint     place_shifting = g_ascii_strtoll (gtk_entry_get_text (entry),
                                                      nullptr,
                                                      10);

          if (place_shifting)
          {
            place_shifting--;
          }

          classification->DumpToFFF (filename,
                                     _contest,
                                     place_shifting);
        }
        else if (export_type == PDF)
        {
#ifdef G_OS_WIN32
          gchar *win32_name = g_win32_locale_filename_from_utf8 (filename);

          _contest->PrintPDF (gettext ("General classification"),
                              win32_name);
          g_free (win32_name);
#else
          _contest->PrintPDF (gettext ("General classification"),
                              filename);
#endif
        }
        else if (export_type == HTML)
        {
          _contest->DumpToHTML (filename);
        }
        else if (export_type == FRD)
        {
          _contest->DumpToFRD (filename);
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
    if (GetStageView (operation) == StageView::CLASSIFICATION)
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
  void GeneralClassification::on_place_entry_insert_text (GtkEntry              *entry,
                                                          const gchar           *text,
                                                          gint                   length,
                                                          gint                  *position,
                                                          GeneralClassification *owner)
  {
    gchar *fixed_string = g_new (gchar, length);
    guint  count        = 0;

    for (gint i = 0; i < length; i++)
    {
      if (g_ascii_isdigit (text[i]))
      {
        fixed_string[count++] = text[i];
      }
    }

    if (fixed_string)
    {
      g_signal_handler_disconnect (entry,
                                   owner->_place_entry_handle);
      gtk_editable_insert_text (GTK_EDITABLE (entry),
                                fixed_string,
                                count,
                                position);
      owner->_place_entry_handle = g_signal_connect (entry,
                                                     "insert-text",
                                                     G_CALLBACK (on_place_entry_insert_text),
                                                     owner);
    }

    g_signal_stop_emission_by_name (G_OBJECT (entry),
                                    "insert_text");

    g_free (fixed_string);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_general_classification_filter_button_clicked (GtkWidget *widget,
                                                                                   Object    *owner)
  {
    GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

    g->OnFilterClicked (nullptr);
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

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_export_frd_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
  {
    GeneralClassification *g = dynamic_cast <GeneralClassification *> (owner);

    g->OnExportToolbuttonClicked (GeneralClassification::FRD);
  }
}
