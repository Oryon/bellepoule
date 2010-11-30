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

#ifndef checkin_hpp
#define checkin_hpp

#include <gtk/gtk.h>

#include "attribute.hpp"
#include "players_list.hpp"

class Contest;

class Classification : public PlayersList
{
  public:
    Classification (Filter *filter);

    void DumpToCSV (gchar  *filename,
                    GSList *attr_list);

    void DumpToFFF (gchar   *filename,
                    Contest *contest);

    void SortDisplay ();

    void SetSortFunction (GtkTreeIterCompareFunc sort_func,
                          gpointer               user_data);

  private:
    void OnPlugged ();

    void WriteFFFString (FILE   *file,
                         Player *player,
                         gchar  *attr_name);

    ~Classification ();
};

#endif
