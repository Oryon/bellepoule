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

#include "actors/player.hpp"

class WebReferee : public Referee
{
  public:
    static void RegisterPlayerClass ();

    static Player *CreateInstance ();

    const gchar *GetXmlTag () override;

    void GiveAnId ();

  protected:
    static const gchar *_class_name;
    static const gchar *_xml_tag;

    Referee ();

    ~Referee () override;

  private:
    static const gchar *_iv;

    Player *Clone () override;

    void SaveAttributes (XmlScheme *xml_scheme,
                         gboolean   full_profile) override;
};
