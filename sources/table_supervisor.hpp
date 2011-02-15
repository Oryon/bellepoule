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

#ifndef table_supervisor_hpp
#define table_supervisor_hpp

#include <gtk/gtk.h>

#include "data.hpp"
#include "module.hpp"
#include "match.hpp"
#include "score_collector.hpp"

#include "stage.hpp"

class TableSupervisor : public virtual Stage, public Module
{
  public:
    static void Init ();

    TableSupervisor (StageClass *stage_class);

    void OnFromTableComboboxChanged ();
    void OnStuffClicked ();
    void OnInputToggled (GtkWidget *widget);
    void OnDisplayToggled (GtkWidget *widget);
    void OnFilterClicked ();
    void OnPrint ();
    void OnZoom (gdouble value);

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void OnPlugged ();
    void OnUnPlugged ();
    void Wipe ();

  private:
    GtkTreeStore       *_display_treestore;
    xmlTextWriter      *_xml_writer;
    xmlNode            *_xml_node;
    GSList             *_result_list;
    gboolean            _print_full_table;

    void Display ();

    void Garnish ();

    void CreateSubTables ();

    void DeleteSubTables ();

    void OnAttrListUpdated ();

    gboolean IsOver ();

    GSList *GetCurrentClassification ();

    static Stage *CreateInstance (StageClass *stage_class);

    void Save (xmlTextWriter *xml_writer);

    void SaveConfiguration (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    void LoadConfiguration (xmlNode *xml_node);

    void FillInConfig ();

    void ApplyConfig ();

    void FeedDisplayStore (guint        from_place,
                           guint        nb_levels,
                           GtkTreeIter *parent);

    ~TableSupervisor ();
};

#endif
