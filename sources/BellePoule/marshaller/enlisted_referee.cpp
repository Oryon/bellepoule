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

#include "slot.hpp"

#include "enlisted_referee.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  EnlistedReferee::EnlistedReferee ()
    : Referee ()
  {
    _slots     = NULL;
    _work_load = 0;

     {
       _workload_rate_attr_id = new AttributeId ("workload_rate");
       SetAttributeValue (_workload_rate_attr_id, 0u);
     }
  }

  // --------------------------------------------------------------------------------
  EnlistedReferee::~EnlistedReferee ()
  {
    g_list_free (_slots);
    _workload_rate_attr_id->Release ();
  }

  // --------------------------------------------------------------------------------
  void EnlistedReferee::AddSlot (Slot *slot)
  {
    _work_load += slot->GetDuration () / G_TIME_SPAN_MINUTE;

    _slots = g_list_insert_sorted (_slots,
                                   slot,
                                   (GCompareFunc) Slot::CompareAvailbility);

  }

  // --------------------------------------------------------------------------------
  void EnlistedReferee::OnRemovedFromSlot (EnlistedReferee *referee,
                                           Slot            *slot)
  {
    GList *node = g_list_find (referee->_slots,
                               slot);

    if (node)
    {
      referee->_slots = g_list_delete_link (referee->_slots,
                                            node);
    }

    referee->_work_load -= slot->GetDuration () / G_TIME_SPAN_MINUTE;
  }

  // --------------------------------------------------------------------------------
  gboolean EnlistedReferee::IsAvailableFor (Slot *slot)
  {
    Player::AttributeId  attr_id ("attending");
    Attribute           *attr = GetAttribute (&attr_id);

    if (attr && (attr->GetUIntValue () == TRUE))
    {
      GList *current = _slots;

      while (current)
      {
        Slot *current_slot = (Slot *) current->data;

        if (slot->Overlaps (current_slot))
        {
          return FALSE;
        }

        current = g_list_next (current);
      }

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gint EnlistedReferee::CompareWorkload (EnlistedReferee *a,
                                         EnlistedReferee *b)
  {
    return a->_work_load - b->_work_load;
  }

  // --------------------------------------------------------------------------------
  gint EnlistedReferee::GetWorkload ()
  {
    return _work_load;
  }

  // --------------------------------------------------------------------------------
  void EnlistedReferee::SetAllRefereWorkload (gint all_referee_workload)
  {
    gint rate = 0;

    if (all_referee_workload)
    {
      rate = (_work_load * 100) / all_referee_workload;
    }

    SetAttributeValue (_workload_rate_attr_id, rate);
  }
}
