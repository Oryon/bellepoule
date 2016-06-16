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

#ifndef wifi_code_hpp
#define wifi_code_hpp

#include "util/object.hpp"
#include "network/wifi_network.hpp"
#include "flash_code.hpp"

class Player;

class WifiCode : public FlashCode
{
  public:
    WifiCode (const gchar *user_name);

    WifiCode (Player *player);

    static void SetPort (guint port);

    void SetWifiNetwork (Net::WifiNetwork *network);

    gchar *GetKey ();

    void ResetKey ();

  private:
    static Net::WifiNetwork *_wifi_network;
    static guint             _port;

    Player                  *_player;
    gchar                   *_key;

    gchar *GetNetwork ();

    gchar *GetIpAddress ();

    virtual ~WifiCode ();

    gchar *GetText ();
};

#endif
