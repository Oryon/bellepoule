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

#ifndef partner_hpp
#define partner_hpp

#include "util/object.hpp"
#include "network/uploader.hpp"

class Partner : public Object, Net::Uploader::Listener
{
  public:
    Partner (const gchar        *who,
             struct sockaddr_in *from);

    void Accept ();

    gboolean Is (const gchar *role);

    gboolean SendMessage (const gchar *where,
                          GKeyFile    *keyfile);

    gboolean SendMessage (const gchar *where,
                          const gchar *message);

  private:
    gchar *_ip;
    gchar *_role;
    guint  _port;

    ~Partner ();

  private:
    void OnUploadStatus (Net::Uploader::PeerStatus peer_status);

    void Use ();

    void Drop ();
};

#endif
