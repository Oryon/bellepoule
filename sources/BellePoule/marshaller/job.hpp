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

#pragma once

#include "util/object.hpp"

class Player;
class Weapon;

namespace Marshaller
{
  class Batch;
  class Slot;

  class Job : public Object
  {
    public:
      struct Listener
      {
        virtual void OnJobUpdated (Job *job) = 0;
      };

    public:
      Job (Batch        *batch,
           Net::Message *message,
           GdkColor     *gdk_color);

      void SetName (const gchar *name);

      void SetWeapon (Weapon *weapon);

      void SetListener (Listener *listener);

      void AddFencer (Player *fencer);

      GList *GetFencerList ();

      void SetSlot (Slot *slot);

      Slot *GetSlot ();

      const gchar *GetName ();

      GTimeSpan GetRegularDuration ();

      void ExtendDuration (guint span);

      gboolean IsOver ();

      void SetRealDuration (GTimeSpan duration);

      Batch *GetBatch ();

      GdkColor *GetGdkColor ();

      guint GetNetID ();

      guint GetDisplayPosition ();

      void OnRefereeAdded ();

      void OnRefereeRemoved ();

      void SetPiste (guint        piste_id,
                     const gchar *start_date,
                     const gchar *start_time);

      void ResetRoadMap ();

      gboolean HasPiste ();

      guint GetKinship ();

      void RefreshStatus ();

      guint GetKinship (Player *with_referre);

      static gint CompareStartTime (Job *a,
                                    Job *b);

      static gint ComparePosition (Job *a,
                                   Job *b);

      void Dump () override;

    private:
      gchar     *_name;
      guint      _netid;
      guint      _position;
      GdkColor  *_gdk_color;
      Batch     *_batch;
      Slot      *_slot;
      GList     *_fencer_list;
      Listener  *_listener;
      guint      _kinship;
      guint      _workload_units;
      GTimeSpan  _regular_duration;
      guint      _duration_span;
      gboolean   _over;
      gboolean   _need_roadmap;

      ~Job () override;

      void MakeRefereeParcel ();
  };
}
