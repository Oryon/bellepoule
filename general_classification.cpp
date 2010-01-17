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
#include "player.hpp"

#include "general_classification.hpp"

const gchar *GeneralClassification::_class_name     = "general_classification";
const gchar *GeneralClassification::_xml_class_name = "general_classification_stage";

// --------------------------------------------------------------------------------
GeneralClassification::GeneralClassification (StageClass *stage_class)
: Stage_c (stage_class),
  PlayersList ("general_classification.glade",
               NO_RIGHT)
{
  // Player attributes to display
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "attending",
                               "exported",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");

    SetFilter (filter);
    filter->Release ();
  }
}

// --------------------------------------------------------------------------------
GeneralClassification::~GeneralClassification ()
{
  Stage_c::Wipe ();
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
Stage_c *GeneralClassification::CreateInstance (StageClass *stage_class)
{
  return new GeneralClassification (stage_class);
}

// --------------------------------------------------------------------------------
void GeneralClassification::Wipe ()
{
  PlayersList::Wipe ();
}

// --------------------------------------------------------------------------------
void GeneralClassification::Display ()
{
  OnAttrListUpdated ();

  for (guint i = 0; i < g_slist_length (_attendees); i++)
  {
    Player_c *player;

    player = (Player_c *) g_slist_nth_data (_attendees,
                                            i);

    Add (player);
  }
}

// --------------------------------------------------------------------------------
void GeneralClassification::Save (xmlTextWriter *xml_writer)
{
  int result;

  result = xmlTextWriterStartElement (xml_writer,
                                      BAD_CAST _xml_class_name);

  result = xmlTextWriterEndElement (xml_writer);
}
