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
      Job (Batch    *batch,
           guint     netid,
           guint     sibling_order,
           GdkColor *gdk_color);

      void SetName (const gchar *name);

      void SetListener (Listener *listener);

      void AddFencer (Player *fencer);

      GList *GetFencerList ();

      void SetSlot (Slot *slot);

      Slot *GetSlot ();

      const gchar *GetName ();

      GTimeSpan GetRegularDuration ();

      Batch *GetBatch ();

      GdkColor *GetGdkColor ();

      guint GetNetID ();

      void SetReferee (guint referee_ref);

      void RemoveReferee (Player *referee);

      void SetPiste (guint        piste_id,
                     const gchar *start_time);

      void ResetRoadMap ();

      static gint CompareStartTime (Job *a,
                                    Job *b);

      static gint CompareSiblingOrder (Job *a,
                                       Job *b);

      static void Dump (Job *what);

    private:
      guint     _sibling_order;
      gchar    *_name;
      guint     _netid;
      GdkColor *_gdk_color;
      Batch    *_batch;
      Slot     *_slot;
      GList    *_fencer_list;
      Listener *_listener;

      ~Job ();
  };
}
