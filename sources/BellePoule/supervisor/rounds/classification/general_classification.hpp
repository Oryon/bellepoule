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

#pragma once

#include <gtk/gtk.h>
#include <libxml/xpath.h>

#include "actors/players_list.hpp"
#include "../../stage.hpp"

namespace People
{
  class GeneralClassification : public Stage, public PlayersList
  {
    public:
      typedef enum
      {
        PDF,
        FFF,
        HTML,
        FRD
      } ExportType;

      static void Declare ();

      GeneralClassification (StageClass *stage_class);

      void OnPrintPoolToolbuttonClicked ();

      void OnExportToolbuttonClicked (ExportType export_type);

    private:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;
      gulong              _place_entry_handle;

      static Stage *CreateInstance (StageClass *stage_class);

      void Load (xmlNode *xml_node) override;

      void SaveAttendees (XmlScheme *xml_scheme) override;

      void Display () override;

      GSList *GetCurrentClassification () override;

      gboolean HasItsOwnRanking () override;

      void GiveShortListAFinalRank ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context) override;

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;

      void OnEndPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context) override;

      gchar *GetPrintName () override;

      void DumpToHTML (FILE *file) override;

      ~GeneralClassification () override;

      static void on_place_entry_insert_text (GtkEntry              *entry,
                                              const gchar           *text,
                                              gint                   length,
                                              gint                  *position,
                                              GeneralClassification *owner);
  };
}
