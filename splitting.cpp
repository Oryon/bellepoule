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

#include "splitting.hpp"

const gchar *Splitting::_class_name     = "splitting";
const gchar *Splitting::_xml_class_name = "splitting_stage";

// --------------------------------------------------------------------------------
Splitting::Splitting (StageClass *stage_class)
: Stage_c (stage_class),
  CanvasModule_c ("splitting.glade")
{
  {
    ShowAttribute ("name");
    ShowAttribute ("club");
  }
}

// --------------------------------------------------------------------------------
Splitting::~Splitting ()
{
  Wipe ();
}

// --------------------------------------------------------------------------------
void Splitting::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
void Splitting::OnPlugged ()
{
  CanvasModule_c::OnPlugged ();
  Display ();
}

// --------------------------------------------------------------------------------
void Splitting::Display ()
{
}

// --------------------------------------------------------------------------------
Stage_c *Splitting::CreateInstance (StageClass *stage_class)
{
  return new Splitting (stage_class);
}

// --------------------------------------------------------------------------------
void Splitting::Load (xmlNode *xml_node)
{
}

// --------------------------------------------------------------------------------
void Splitting::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Splitting::Wipe ()
{
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_splitting_print_toolbutton_clicked (GtkWidget *widget,
                                                                       Object_c  *owner)
{
  Splitting *t = dynamic_cast <Splitting *> (owner);

  t->Print ();
}

// --------------------------------------------------------------------------------
void Splitting::Enter ()
{
  EnableSensitiveWidgets ();

  Display ();
}

// --------------------------------------------------------------------------------
void Splitting::OnLocked ()
{
  DisableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void Splitting::OnUnLocked ()
{
  EnableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void Splitting::OnAttrListUpdated ()
{
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_splitting_filter_toolbutton_clicked (GtkWidget *widget,
                                                                        Object_c  *owner)
{
  Splitting *t = dynamic_cast <Splitting *> (owner);

  t->SelectAttributes ();
}
