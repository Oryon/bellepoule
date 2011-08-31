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

#ifndef tournament_hpp
#define tournament_hpp

#include <gtk/gtk.h>

#include "module.hpp"
#include "glade.hpp"

class Contest;

class Tournament : public Module
{
  public:
     Tournament (gchar *filename);
    ~Tournament ();

    static void Init ();

    void OnNew ();

    void OnOpen (gchar *current_folder = NULL);

    void OnRecent ();

    void OnAbout ();

    void OnOpenExample ();

    void OnOpenUserManual ();

    void OnSave ();

    void OnBackupfileLocation ();

    Contest *GetContest (gchar *filename);

    void Manage (Contest *contest);

    void OnContestDeleted (Contest *contest);

    const gchar *GetBackupLocation ();

    void OnActivateBackup ();

  private:
    GSList *_contest_list;

    void ReadConfiguration ();

    void SetBackupLocation (gchar *location);
};

#endif
