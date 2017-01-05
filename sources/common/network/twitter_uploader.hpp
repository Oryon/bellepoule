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

#include "uploader.hpp"

namespace Oauth
{
  class HttpRequest;
}

namespace Net
{
  class TwitterUploader : public Uploader
  {
    public:
      class Listener
      {
        public:
          virtual void OnTwitterResponse (Oauth::HttpRequest *request) = 0;
          virtual void Use  () = 0;
          virtual void Drop () = 0;
      };

    public:
      TwitterUploader (Listener *listener);

      void UpLoadRequest (Oauth::HttpRequest *request);

    protected:
      virtual ~TwitterUploader ();

    private:
      Listener           *_listener;
      Oauth::HttpRequest *_request;
      struct curl_slist  *_http_header;

      static gpointer ThreadFunction (TwitterUploader *uploader);

      static gboolean OnThreadDone (TwitterUploader *uploader);

    private:
      void SetCurlOptions (CURL *curl);

      const gchar *GetUrl ();

      void OnUploadDone (const gchar *response);
  };
}
