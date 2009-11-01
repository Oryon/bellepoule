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
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "table.hpp"

const gchar *Table::_class_name     = "table";
const gchar *Table::_xml_class_name = "table_stage";

// --------------------------------------------------------------------------------
Table::Table (StageClass *stage_class)
: Stage_c (stage_class),
  CanvasModule_c ("table.glade")
{
  {
    ShowAttribute ("name");
    ShowAttribute ("club");
  }
}

// --------------------------------------------------------------------------------
Table::~Table ()
{
  Wipe ();
}

// --------------------------------------------------------------------------------
void Table::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
void Table::OnPlugged ()
{
  CanvasModule_c::OnPlugged ();
  Display ();
}

// --------------------------------------------------------------------------------
void Table::Display ()
{
}

// --------------------------------------------------------------------------------
Stage_c *Table::CreateInstance (StageClass *stage_class)
{
  return new Table (stage_class);
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
}

// --------------------------------------------------------------------------------
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Table::Wipe ()
{
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                   Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->Print ();
}

// --------------------------------------------------------------------------------
void Table::Enter ()
{
  EnableSensitiveWidgets ();

  Display ();
}

// --------------------------------------------------------------------------------
void Table::OnLocked ()
{
  DisableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void Table::OnUnLocked ()
{
  EnableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void Table::OnAttrListUpdated ()
{
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->SelectAttributes ();
}
