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

#include "actors/players_list.hpp"

class Contest;

class Classification : public People::PlayersList
{
  public:
    Classification ();

    void DumpToFFF (gchar   *filename,
                    Contest *contest,
                    guint    place_shifting);

    void SortDisplay ();

    void SetSortFunction (GtkTreeIterCompareFunc sort_func,
                          gpointer               user_data);

    void Conceal () override;

    void Spread () override;

    void Recall () override;

  private:
    guint _fff_place_shifting;

    void OnPlugged () override;

    void WriteFFFString (FILE        *file,
                         Player      *player,
                         const gchar *attr_name);

    ~Classification () override;

    gboolean IsTableBorder (guint place) override;

    void FeedParcel (Net::Message *parcel) override;
};
