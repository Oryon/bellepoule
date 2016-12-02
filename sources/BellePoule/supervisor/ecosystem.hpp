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
#include "util/glade.hpp"
#include "util/wifi_code.hpp"
#include "network/wifi_network.hpp"
#include "network/file_uploader.hpp"

class EcoSystem : public Object
{
  public:
    EcoSystem (Glade *glade);

    Net::FileUploader *GetUpLoader ();

    WifiCode *GetAdminCode ();

  private:
    Glade            *_glade;
    Net::WifiNetwork *_wifi_network;
    WifiCode         *_admin_wifi_code;

    virtual ~EcoSystem ();

    static void RefreshScannerCode (GtkEditable *widget,
                                    EcoSystem   *ecosystem);

    static void OnRemoteHostChanged (GtkEditable *widget,
                                     EcoSystem   *ecosystem);
};
