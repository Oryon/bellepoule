// Copyright (C) 2011 Yannick Le Roux.
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

#include "network/uploader.hpp"

namespace Oauth
{
  namespace V1
  {
    class Request;
  }

  class Uploader : public Net::Uploader
  {
    public:
      class Listener
      {
        public:
          virtual void OnServerResponse (V1::Request *request) = 0;
          virtual void Use  () = 0;
          virtual void Drop () = 0;
      };

    public:
      Uploader (Listener *listener);

      void UpLoadRequest (V1::Request *request);

    protected:
      virtual ~Uploader ();

    private:
      Listener           *_listener;
      V1::Request        *_request;
      struct curl_slist  *_http_header;
      gchar              *_postfields;
      GHashTable         *_response_header;

      static gpointer ThreadFunction (Uploader *uploader);

      static gboolean OnThreadDone (Uploader *uploader);

      void SetCurlOptions (CURL *curl);

      const gchar *GetUrl ();

      static size_t OnResponseHeader (char     *buffer,
                                      size_t    size,
                                      size_t    nitems,
                                      Uploader *uploader);
  };
}
