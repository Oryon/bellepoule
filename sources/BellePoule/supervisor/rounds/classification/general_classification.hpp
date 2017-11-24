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
#include <libxml/xmlwriter.h>
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
        HTML
      } ExportType;

      static void Declare ();

      GeneralClassification (StageClass *stage_class);

      void OnPrintPoolToolbuttonClicked ();

      void OnExportToolbuttonClicked (ExportType export_type);

    private:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

      static Stage *CreateInstance (StageClass *stage_class);

      void Load (xmlNode *xml_node);

      void Save (xmlTextWriter *xml_writer);

      void Display ();

      GSList *GetCurrentClassification ();

      gboolean HasItsOwnRanking ();

      void GiveShortListAFinalRank ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      void OnEndPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);

      gchar *GetPrintName ();

      void DumpToHTML (FILE *file);

      virtual ~GeneralClassification ();
  };
}
