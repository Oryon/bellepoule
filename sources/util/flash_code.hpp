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

#ifndef flash_code_hpp
#define flash_code_hpp

#include "util/object.hpp"
#include "network/wifi_network.hpp"

class Player;

class FlashCode : public Object
{
  public:
    FlashCode (const gchar *user_name);

    FlashCode (Player *player);

    void SetWifiNetwork (Net::WifiNetwork *network);

    GdkPixbuf *GetPixbuf ();

  private:
    static Net::WifiNetwork *_wifi_network;

    gchar     *_user_name;
    gchar     *_key;
    Player    *_player;
    GdkPixbuf *_pixbuf;

    gchar *GetNetwork ();

    gchar *GetIpAddress ();

    gchar *GetKey ();

    static void DestroyPixbuf (guchar   *pixels,
                               gpointer  data);

    virtual ~FlashCode ();
};

#endif
