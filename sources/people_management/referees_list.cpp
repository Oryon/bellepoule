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

#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>

#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "attribute.hpp"
#include "player.hpp"
#include "filter.hpp"
#include "contest.hpp"

#include "referees_list.hpp"

// --------------------------------------------------------------------------------
RefereesList::RefereesList (Contest *contest)
  : Checkin ("referees.glade",
             "Arbitre")
{
  _contest = contest;

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
#endif
                               "start_rank",
                               "status",
                               "global_status",
                               "previous_stage_rank",
                               "exported",
                               "final_rank",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
                               "HS",
                               "rank",
                               "ranking",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("attending");
    filter->ShowAttribute ("availability");
    filter->ShowAttribute ("participation_rate");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("level");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("league");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("birth_date");
    filter->ShowAttribute ("licence");

    SetFilter (filter);
    CreateForm (filter);
    filter->Release ();
  }

  {
    SetDndSource (_tree_view);
    gtk_drag_source_set_icon_name (_tree_view,
                                   "preferences-desktop-theme");
  }
}

// --------------------------------------------------------------------------------
RefereesList::~RefereesList ()
{
}

// --------------------------------------------------------------------------------
void RefereesList::OnPlayerLoaded (Player *referee)
{
  Player::AttributeId  attending_attr_id ("attending");
  Attribute           *attending_attr = referee->GetAttribute (&attending_attr_id);

  if (attending_attr && attending_attr->GetUIntValue ())
  {
    Player::AttributeId availability_attr_id ("availability");

    if (strcmp (referee->GetAttribute (&availability_attr_id)->GetStrValue (),
                "Absent") == 0)
    {
      referee->SetAttributeValue (&availability_attr_id,
                                  "Free");
    }
  }
}

// --------------------------------------------------------------------------------
void RefereesList::Monitor (Player *player)
{
  Checkin::Monitor (player);

  player->SetChangeCbk ("attending",
                        (Player::OnChange) OnAttendingChanged,
                        this);
  player->SetChangeCbk ("availability",
                        (Player::OnChange) OnAvailabilityChanged,
                        this);
  player->SetChangeCbk ("participation_rate",
                        (Player::OnChange) OnParticipationRateChanged,
                        this);

  {
    Player::AttributeId availability_attr_id ("availability");

    if (player->GetAttribute (&availability_attr_id) == NULL)
    {
      Player::AttributeId  attending_attr_id ("attending");
      Attribute           *attending_attr = player->GetAttribute (&attending_attr_id);
      guint                attending = attending_attr->GetUIntValue ();

      if (attending == TRUE)
      {
        player->SetAttributeValue (&availability_attr_id,
                                   "Free");
      }
      else if (attending == FALSE)
      {
        player->SetAttributeValue (&availability_attr_id,
                                   "Absent");
      }
    }
  }
}

// --------------------------------------------------------------------------------
void RefereesList::Add (Player *player)
{
  Player *original = _contest->Share (player);

  if (original)
  {
    Checkin::Add (original);
  }
  else
  {
    Checkin::Add (player);
  }
}

// --------------------------------------------------------------------------------
void RefereesList::OnAttendingChanged (Player    *player,
                                       Attribute *attr,
                                       Object    *owner)
{
  guint               value = attr->GetUIntValue ();
  Player::AttributeId attr_id ("availability");

  if (value == 1)
  {
    player->SetAttributeValue (&attr_id,
                               "Free");
  }
  else if (value == 0)
  {
    player->SetAttributeValue (&attr_id,
                               "Absent");
  }
}

// --------------------------------------------------------------------------------
void RefereesList::OnAvailabilityChanged (Player    *player,
                                          Attribute *attr,
                                          Object    *owner)
{
  Checkin *checkin = dynamic_cast <Checkin *> (owner);

  checkin->Update (player);
}

// --------------------------------------------------------------------------------
void RefereesList::OnParticipationRateChanged (Player    *player,
                                               Attribute *attr,
                                               Object    *owner)
{
  Checkin *checkin = dynamic_cast <Checkin *> (owner);

  checkin->Update (player);
}

// --------------------------------------------------------------------------------
void RefereesList::OnDragDataGet (GtkWidget        *widget,
                                  GdkDragContext   *drag_context,
                                  GtkSelectionData *data,
                                  guint             info,
                                  guint             time)
{
  if (info == INT_TARGET)
  {
    GSList  *selected    = GetSelectedPlayers ();
    Player  *referee     = (Player *) selected->data;
    guint32  referee_ref = referee->GetRef ();

    gtk_selection_data_set (data,
                            data->target,
                            32,
                            (guchar *) &referee_ref,
                            sizeof (referee_ref));
  }
}
