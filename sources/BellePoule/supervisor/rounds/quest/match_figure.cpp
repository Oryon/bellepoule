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

#include "util/attribute.hpp"
#include "../../match.hpp"

#include "match_figure.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  MatchFigure::MatchFigure (const gchar *name,
                            Object      *owner)
    : Object ("Quest::MatchFigure")
  {
    _key = g_strdup_printf ("MatchFigure::%s", name);

    _attr_id = new Player::AttributeId (name,
                                        owner);
  }

  // --------------------------------------------------------------------------------
  void MatchFigure::AddToFencer (Player *fencer,
                                 Match  *match,
                                 guint   value)
  {
    Attribute *attr = fencer->GetAttribute (_attr_id);

    fencer->SetData (match,
                     _key,
                     GUINT_TO_POINTER (value));

    if (attr)
    {
      value += attr->GetUIntValue ();
    }
    fencer->SetAttributeValue (_attr_id,
                               value);
  }

  // --------------------------------------------------------------------------------
  void MatchFigure::RemoveFromFencer (Player *fencer,
                                      Match  *match)
  {
    Attribute *attr = fencer->GetAttribute (_attr_id);

    if (attr)
    {
      guint previous_value = fencer->GetUIntData (match,
                                                  _key);

      fencer->SetAttributeValue (_attr_id,
                                 attr->GetUIntValue () - previous_value);
      fencer->RemoveData (match,
                          _key);
    }
  }

  // --------------------------------------------------------------------------------
  MatchFigure::~MatchFigure ()
  {
    g_free (_key);
    _attr_id->Release ();
  }
}
