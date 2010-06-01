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

#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "contest.hpp"
#include "player.hpp"
#include "tournament.hpp"

#include "splitting.hpp"

const gchar *Splitting::_class_name     = "Scission";
const gchar *Splitting::_xml_class_name = "PointDeScission";

Tournament *Splitting::_tournament = NULL;

// --------------------------------------------------------------------------------
Splitting::Splitting (StageClass *stage_class)
: Stage (stage_class),
  PlayersList ("splitting.glade",
               SORTABLE)
{
  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("splitting_move_toolbutton"));
  }

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "attending",
                               "victories_ratio",
                               "indice",
                               "HS",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("exported");
    filter->ShowAttribute ("previous_stage_rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("birth_date");
    filter->ShowAttribute ("gender");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("licence");

    SetFilter (filter);
    filter->Release ();
  }

  SetAttributeRight ("exported",
                     TRUE);
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
void Splitting::Garnish ()
{
}

// --------------------------------------------------------------------------------
void Splitting::SetHostTournament (Tournament *tournament)
{
  _tournament = tournament;
}

// --------------------------------------------------------------------------------
Stage *Splitting::CreateInstance (StageClass *stage_class)
{
  return new Splitting (stage_class);
}

// --------------------------------------------------------------------------------
void Splitting::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  Stage::SaveConfiguration (xml_writer);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Splitting::Wipe ()
{
  PlayersList::Wipe ();
}

// --------------------------------------------------------------------------------
void Splitting::Display ()
{
  for (guint i = 0; i < g_slist_length (_attendees); i ++)
  {
    Player *player;

    player = (Player *) g_slist_nth_data (_attendees,
                                          i);
    Add (player);
  }
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Splitting::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);

  if (reason == USER_ACTION)
  {
    Contest             *contest = _contest->Duplicate ();
    GSList              *current = _attendees;
    Player::AttributeId  exported_attr ("exported");

    _tournament->Manage (contest);

    for (guint i = 0; current != NULL; i++)
    {
      Player    *player;
      Attribute *attr;

      player = (Player *) current->data;
      attr   = player->GetAttribute (&exported_attr);
      if ((gboolean) attr->GetValue () == TRUE)
      {
        Player *new_player = player->Duplicate ();

        new_player->SetAttributeValue (&exported_attr,
                                       (guint) FALSE);
        contest->AddPlayer (new_player,
                            i+1);
      }

      current = g_slist_next (current);
    }
    contest->LatchPlayerList ();
  }
}

// --------------------------------------------------------------------------------
GSList *Splitting::GetCurrentClassification ()
{
  GSList *result = CreateCustomList (PresentPlayerFilter);

  if (result)
  {
    Player::AttributeId attr_id ("rank",
                                 this);

    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) Player::Compare,
                                     &attr_id);
  }
  return result;
}

// --------------------------------------------------------------------------------
gboolean Splitting::PresentPlayerFilter (Player *player)
{
  Player::AttributeId  attr_id ("exported");
  Attribute           *attr = player->GetAttribute (&attr_id);

  return ((gboolean) attr->GetValue () == FALSE);
}

// --------------------------------------------------------------------------------
void Splitting::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_splitting_print_toolbutton_clicked (GtkWidget *widget,
                                                                       Object    *owner)
{
  Splitting *s = dynamic_cast <Splitting *> (owner);

  s->Print ("Liste des tireurs à extraire");
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_splitting_filter_toolbutton_clicked (GtkWidget *widget,
                                                                        Object    *owner)
{
  Splitting *s = dynamic_cast <Splitting *> (owner);

  s->SelectAttributes ();
}
