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

#include "object.hpp"

class FieTime : public Object
{
  public:
    FieTime (GDateTime *time);

    FieTime (const gchar *time);

    const gchar *GetImage ();

    const gchar *GetXmlImage ();

    GDateTime *GetGDateTime ();

  private:
    gchar     *_image;
    gchar     *_xml_image;
    GDateTime *_g_date_time;

    ~FieTime () override;

    void MakeImages (GDateTime *date_time);
};
