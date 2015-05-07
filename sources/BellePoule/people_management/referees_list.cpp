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

#include "util/attribute.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "application/contest.hpp"

#include "referees_list.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  RefereesList::RefereesList (Contest *contest)
    : Checkin ("referees.glade",
               "Referee",
               NULL)
  {
    _contest = contest;

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
                                          "HS",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "ranking",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("attending");
      filter->ShowAttribute ("connection");
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
      CreateForm (filter,
                  "Referee");
      filter->Release ();
    }

    {
      _dnd_key = _dnd_config->CreateTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      gtk_drag_source_set (GTK_WIDGET (_tree_view),
                           GDK_MODIFIER_MASK,
                           _dnd_config->GetTargetTable (),
                           _dnd_config->GetTargetTableSize (),
                           GDK_ACTION_COPY);

      ConnectDndSource (GTK_WIDGET (_tree_view));
      gtk_drag_source_set_icon_name (GTK_WIDGET (_tree_view),
                                     "preferences-desktop-theme");
    }

    SetPasteVisibility (TRUE);
  }

  // --------------------------------------------------------------------------------
  RefereesList::~RefereesList ()
  {
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnPlayerLoaded (Player *referee,
                                     Player *owner)
  {
    Player::AttributeId  attending_attr_id ("attending");
    Attribute           *attending_attr = referee->GetAttribute (&attending_attr_id);

    if (attending_attr && attending_attr->GetUIntValue ())
    {
      Player::AttributeId  availability_attr_id ("availability");
      Attribute           *availability_attr = referee->GetAttribute (&availability_attr_id);

      if (strcmp (availability_attr->GetStrValue (),
                  "Absent") == 0)
      {
        referee->SetAttributeValue (&availability_attr_id,
                                    "Free");
      }
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnPlayerEventFromForm (Player            *referee,
                                            Form::PlayerEvent  event)
  {
    {
      Player::AttributeId  attending_attr_id ("attending");
      Attribute           *attending_attr = referee->GetAttribute (&attending_attr_id);
      Player::AttributeId  availability_attr_id ("availability");

      if (attending_attr && attending_attr->GetUIntValue ())
      {
        referee->SetAttributeValue (&availability_attr_id,
                                    "Free");
      }
      else
      {
        referee->SetAttributeValue (&availability_attr_id,
                                    "Absent");
      }
    }

    Checkin::OnPlayerEventFromForm (referee,
                                    event);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Monitor (Player *referee)
  {
    Checkin::Monitor (referee);

    referee->SetChangeCbk ("connection",
                           (Player::OnChange) OnConnectionChanged,
                           this);
    referee->SetChangeCbk ("attending",
                           (Player::OnChange) OnAttendingChanged,
                           this);
    referee->SetChangeCbk ("availability",
                           (Player::OnChange) OnAvailabilityChanged,
                           this);
    referee->SetChangeCbk ("participation_rate",
                           (Player::OnChange) OnParticipationRateChanged,
                           this);

    {
      Player::AttributeId availability_attr_id ("availability");

      if (referee->GetAttribute (&availability_attr_id) == NULL)
      {
        Player::AttributeId  attending_attr_id ("attending");
        Attribute           *attending_attr = referee->GetAttribute (&attending_attr_id);
        guint                attending = attending_attr->GetUIntValue ();

        if (attending == TRUE)
        {
          referee->SetAttributeValue (&availability_attr_id,
                                      "Free");
        }
        else if (attending == FALSE)
        {
          referee->SetAttributeValue (&availability_attr_id,
                                      "Absent");
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnAttendingChanged (Player    *referee,
                                         Attribute *attr,
                                         Object    *owner,
                                         guint      step)
  {
    guint               value = attr->GetUIntValue ();
    Player::AttributeId attr_id ("availability");

    if (value == 1)
    {
      guint token = referee->GetUIntData (NULL,
                                          "Referee::token");

      if (token)
      {
        referee->SetAttributeValue (&attr_id,
                                    "Busy");
      }
      else
      {
        referee->SetAttributeValue (&attr_id,
                                    "Free");
      }
    }
    else if (value == 0)
    {
      referee->SetAttributeValue (&attr_id,
                                  "Absent");
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnConnectionChanged (Player    *referee,
                                          Attribute *attr,
                                          Object    *owner,
                                          guint      step)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (owner);

    checkin->Update (referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnAvailabilityChanged (Player    *referee,
                                            Attribute *attr,
                                            Object    *owner,
                                            guint      step)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (owner);

    checkin->Update (referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnParticipationRateChanged (Player    *referee,
                                                 Attribute *attr,
                                                 Object    *owner,
                                                 guint      step)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (owner);

    checkin->Update (referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnDragDataGet (GtkWidget        *widget,
                                    GdkDragContext   *drag_context,
                                    GtkSelectionData *data,
                                    guint             key,
                                    guint             time)
  {
    if (key == _dnd_key)
    {
      GSList  *selected    = GetSelectedPlayers ();
      Player  *referee     = (Player *) selected->data;
      guint32  referee_ref = referee->GetDndRef ();

      gtk_selection_data_set (data,
                              gtk_selection_data_get_target (data),
                              32,
                              (guchar *) &referee_ref,
                              sizeof (referee_ref));
    }
  }
}
