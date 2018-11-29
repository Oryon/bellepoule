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
#include "util/attribute_desc.hpp"

class Player;
class Match;

namespace Table
{
  class HtmlTable : public Object
  {
    public:
      HtmlTable (Object *owner);

      void Prepare (guint column_count);

      void Clean ();

      void Put (Match *match,
                guint  row,
                guint  column);

      void Connect (Match *m1,
                    Match *m2);

      void DumpToHTML (FILE *file);

    private:
      void   **_table;
      guint    _table_width;
      guint    _table_height;
      Object  *_owner;

      virtual ~HtmlTable ();

      void DumpToHTML (FILE                *file,
                       Player              *fencer,
                       const gchar         *attr_name,
                       AttributeDesc::Look  look);
  };
}
