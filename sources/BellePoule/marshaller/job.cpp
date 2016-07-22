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

#include "network/message.hpp"
#include "slot.hpp"

#include "job.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Job::Job (Batch    *batch,
            guint     uuid,
            guint     sibling_order,
            GdkColor *gdk_color)
    : Object ("Job")
  {
     _gdk_color     = gdk_color_copy (gdk_color);
     _name          = NULL;
     _batch         = batch;
     _uuid          = uuid;
     _sibling_order = sibling_order;
     _slot          = NULL;

    Disclose ("Roadmap");
    _parcel->Set ("listener", uuid);
  }

  // --------------------------------------------------------------------------------
  Job::~Job ()
  {
    if (_slot)
    {
      _slot->RemoveJob (this);
    }

    g_free         (_name);
    gdk_color_free (_gdk_color);
  }

  // --------------------------------------------------------------------------------
  void Job::SetName (const gchar *name)
  {
    _name = g_strdup (name);
  }

  // --------------------------------------------------------------------------------
  void Job::SetSlot (Slot *slot)
  {
    _slot = slot;
  }

  // --------------------------------------------------------------------------------
  Slot *Job::GetSlot ()
  {
    return _slot;
  }

  // --------------------------------------------------------------------------------
  const gchar *Job::GetName ()
  {
    return _name;
  }

  // --------------------------------------------------------------------------------
  Net::Message *Job::GetRoadMap ()
  {
    return _parcel;
  }

  // --------------------------------------------------------------------------------
  guint Job::GetUUID ()
  {
    return _uuid;
  }

  // --------------------------------------------------------------------------------
  Batch *Job::GetBatch ()
  {
    return _batch;
  }

  // --------------------------------------------------------------------------------
  GdkColor *Job::GetGdkColor ()
  {
    return _gdk_color;
  }

  // --------------------------------------------------------------------------------
  gint Job::CompareStartTime (Job *a,
                              Job *b)
  {
    Slot *slot_a = a->GetSlot ();
    Slot *slot_b = b->GetSlot ();

    return g_date_time_compare (slot_a->GetStartTime (),
                                slot_b->GetStartTime ());
  }

  // --------------------------------------------------------------------------------
  gint Job::CompareSiblingOrder (Job *a,
                                 Job *b)
  {
    return a->_sibling_order - b->_sibling_order;
  }
}
