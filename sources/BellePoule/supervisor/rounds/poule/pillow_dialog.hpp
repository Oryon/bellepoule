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

#include <gtk/gtk.h>

#include "util/object.hpp"

class Module;

namespace People
{
  class Checkin;
}

namespace Pool
{
  class PillowDialog : public Object
  {
    public:
    struct Listener
    {
      virtual gboolean OnAttendeeToggled (PillowDialog *from,
                                          Player       *attendee,
                                          gboolean      attending) = 0;
    };

    public:
      PillowDialog (Module      *owner,
                    Listener    *listener,
                    const gchar *name);

      void Populate (GSList *fencers,
                     guint   nb_pool);

      void RevertAll ();

      void Clear ();

      void DisplayForm ();

      guint GetNbPools ();

      static const gchar *GetRevertContext ();

    private:
      static const gchar *_revert_context;

      Listener        *_listener;
      People::Checkin *_checkin;
      gboolean         _revert_value;
      guint            _nb_pool;
      GtkWidget       *_warning_box;
      GtkLabel        *_warning_label;
      GList           *_postponed_revert;

      virtual ~PillowDialog ();

      static void OnAttendingChanged (Player    *player,
                                      Attribute *attr,
                                      Object    *owner,
                                      guint      step);

      static gboolean DeferedRevert (PillowDialog *pillow);

  };
}
