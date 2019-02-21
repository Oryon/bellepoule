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
#include "util/drop_zone.hpp"
#include "network/message.hpp"
#include "application/weapon.hpp"
#include "slot.hpp"
#include "timeline.hpp"
#include "affinities.hpp"
#include "batch.hpp"
#include "competition.hpp"

#include "job.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Job::Job (Batch        *batch,
            Net::Message *message,
            guint         sibling_order,
            GdkColor     *gdk_color)
    : Object ("Job")
  {
    _listener         = nullptr;
    _fencer_list      = nullptr;
    _gdk_color        = gdk_color_copy (gdk_color);
    _name             = nullptr;
    _batch            = batch;
    _netid            = message->GetNetID ();
    _sibling_order    = sibling_order;
    _slot             = nullptr;
    _kinship          = 0;
    _regular_duration = 0;
    _over             = FALSE;
    _workload_units   = message->GetInteger ("workload_units");

    if (message->GetInteger ("duration_span"))
    {
      _duration_span = message->GetInteger ("duration_span");
    }
    else
    {
      _duration_span = 1;
    }

    Disclose ("BellePoule2D::Roadmap");
    _parcel->Set ("competition",   message->GetInteger ("competition"));
    _parcel->Set ("stage",         message->GetInteger ("stage"));
    _parcel->Set ("batch",         message->GetInteger ("batch"));
    _parcel->Set ("source",        _netid);
    _parcel->Set ("duration_span", _duration_span);
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
  void Job::SetWeapon (Weapon *weapon)
  {
    _regular_duration = _workload_units * weapon->GetStandardDuration () * G_TIME_SPAN_SECOND;
    _regular_duration += Timeline::STEP - (_regular_duration % Timeline::STEP);
  }

  // --------------------------------------------------------------------------------
  GTimeSpan Job::GetRegularDuration ()
  {
    return _regular_duration * _duration_span;
  }

  // --------------------------------------------------------------------------------
  void Job::ExtendDuration (guint span)
  {
    _duration_span = span;
    _parcel->Set ("duration_span", _duration_span);
  }

  // --------------------------------------------------------------------------------
  gboolean Job::IsOver ()
  {
    return _over;
  }

  // --------------------------------------------------------------------------------
  void Job::SetRealDuration (GTimeSpan duration)
  {
    if (_slot != nullptr)
    {
      if (duration > 0)
      {
        if (duration < _regular_duration)
        {
          _slot->SetDuration (duration);
        }
        _over = TRUE;
      }
      else if (_over == TRUE)
      {
        _over = FALSE;
        _slot->SetDuration (_regular_duration);
      }
    }
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
  void Job::MakeRefereeParcel ()
  {
    GList *referee_list = nullptr;

    if (_slot)
    {
      referee_list = _slot->GetRefereeList ();
    }

    if (referee_list)
    {
      gsize length           = g_list_length (referee_list);
      guint referees[length];

      for (guint i = 0; i < length; i++)
      {
        Player *referee = (Player *) g_list_nth_data (referee_list,
                                                      i);

        referees[i] = referee->GetRef ();
      }

      _parcel->SetList ("referees",
                        referees,
                        length);
    }
    else
    {
      _parcel->Remove ("referees");
    }
  }

  // --------------------------------------------------------------------------------
  void Job::OnRefereeAdded ()
  {
    MakeRefereeParcel ();

    if (_listener)
    {
      _listener->OnJobUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Job::OnRefereeRemoved ()
  {
    MakeRefereeParcel ();
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
    _parcel->Remove ("referees");

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
  gboolean Job::HasReferees ()
  {
    if (_slot)
    {
      return _slot->GetRefereeList () != nullptr;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  guint Job::GetKinship ()
  {
    return _kinship;
  }

  // --------------------------------------------------------------------------------
  guint Job::GetKinship (Player *with_referre)
  {
    guint       worst              = 0;
    Affinities *referee_affinities = (Affinities *) with_referre->GetPtrData (nullptr,
                                                                              "affinities");

    for (GList *fencers = GetFencerList (); fencers; fencers = g_list_next (fencers))
    {
      Affinities *fencer_affinities;
      Player     *fencer            = (Player *) fencers->data;

      fencer_affinities = (Affinities *) fencer->GetPtrData (nullptr,
                                                             "affinities");

      if (fencer_affinities)
      {
        guint kinship = fencer_affinities->KinshipWith (referee_affinities);

        if (kinship > worst)
        {
          worst = kinship;
        }
      }
    }

    return worst;
  }

  // --------------------------------------------------------------------------------
  void Job::RefreshStatus ()
  {
    _kinship = 0;

    if (_slot)
    {
      for (GList *referees = _slot->GetRefereeList (); referees; referees = g_list_next (referees))
      {
        Player *referee = (Player *) referees->data;
        guint   kinship = GetKinship (referee);

        if (kinship > _kinship)
        {
          _kinship = kinship;
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Job::Dump ()
  {
    printf ("%s::%s (%d)\n", _batch->GetName (), _name, _kinship);
  }
}
