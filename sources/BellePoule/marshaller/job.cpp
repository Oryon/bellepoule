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

#include "util/player.hpp"
#include "network/message.hpp"
#include "slot.hpp"
#include "competition.hpp"
#include "batch.hpp"

#include "job.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Job::Job (Batch    *batch,
            guint     netid,
            guint     sibling_order,
            GdkColor *gdk_color)
    : Object ("Job")
  {
    _listener      = NULL;
    _fencer_list   = NULL;
    _gdk_color     = gdk_color_copy (gdk_color);
    _name          = NULL;
    _batch         = batch;
    _netid         = netid;
    _sibling_order = sibling_order;
    _slot          = NULL;

    Disclose ("Roadmap");
    _parcel->Set ("competition", batch->GetCompetition ()->GetId ());
    _parcel->Set ("batch",       batch->GetId ());
    _parcel->Set ("source",      netid);
    _parcel->SetFitness (1);
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

    FreeFullGList (Player, _fencer_list);
  }

  // --------------------------------------------------------------------------------
  void Job::SetName (const gchar *name)
  {
    _name = g_strdup (name);
  }

  // --------------------------------------------------------------------------------
  void Job::SetListener (Listener *listener)
  {
    _listener = listener;
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
  GTimeSpan Job::GetRegularDuration ()
  {
    return 30*G_TIME_SPAN_MINUTE;
  }

  // --------------------------------------------------------------------------------
  void Job::AddFencer (Player *fencer)
  {
    fencer->Retain ();
    _fencer_list = g_list_append (_fencer_list,
                                  fencer);
  }

  // --------------------------------------------------------------------------------
  GList *Job::GetFencerList ()
  {
    return _fencer_list;
  }

  // --------------------------------------------------------------------------------
  void Job::SetReferee (guint referee_ref)
  {
    _parcel->Set ("referee", referee_ref);

    if (_listener)
    {
      _listener->OnJobUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Job::RemoveReferee (Player *referee)
  {
    _parcel->Remove ("referee");
  }

  // --------------------------------------------------------------------------------
  void Job::SetPiste (guint        piste_id,
                      const gchar *start_time)
  {
    _parcel->Set ("piste",      piste_id);
    _parcel->Set ("start_time", start_time);

    if (_listener)
    {
      _listener->OnJobUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Job::ResetRoadMap ()
  {
    _parcel->Recall ();

    _parcel->Remove ("piste");
    _parcel->Remove ("start_time");
    _parcel->Remove ("referee");

    if (_listener)
    {
      _listener->OnJobUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  guint Job::GetNetID ()
  {
    return _netid;
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

  // --------------------------------------------------------------------------------
  void Job::Dump (Job *what)
  {
    printf ("%s\n", what->_name);
  }
}
