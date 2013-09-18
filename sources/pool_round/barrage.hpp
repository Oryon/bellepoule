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

#ifndef barrage_hpp
#define barrage_hpp

#include "util/module.hpp"
#include "common/stage.hpp"
#include "pool_round/pool.hpp"

namespace Pool
{
  class Barrage : public virtual Stage, public Module
  {
    public:
      static void Declare ();

      Barrage (StageClass *stage_class);

      void OnFilterClicked ();

      void OnStuffClicked ();

      void OnPrintPoolToolbuttonClicked ();

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      void OnLocked ();

      void OnUnLocked ();

      void Display ();

      void OnAttrListUpdated ();

      void Wipe ();

      GSList *GetCurrentClassification ();

      void Save (xmlTextWriter *xml_writer);

      void Load (xmlNode *xml_node);

      void Garnish ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      void OnEndPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);

    private:
      static Stage *CreateInstance (StageClass *stage_class);

      virtual ~Barrage ();

      void OnPlugged ();

      void OnUnPlugged ();

    private:
      Pool      *_pool;
      GtkWidget *_print_dialog;
  };
}

#endif
