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

#include <zip.h>
#include "util/object.hpp"

class Zipper : public Object
{
  public:
    Zipper (const gchar *zip_name);

    void Archive (const gchar *what,
                  const gchar *title);

    // Workaround to Windows issue
    void Archive2 (const gchar *what,
                   const gchar *title);

  private:
    const gchar *_zip_name;
    gchar       *_temp_zip_name;
    zip_t       *_to_archive;

    ~Zipper () override;

    int get_data (void       **datap,
                  size_t      *sizep,
                  const char  *archive);

    void AddEntry (zip_t       *archive,
                   zip_source  *source,
                   const gchar *title);
};
