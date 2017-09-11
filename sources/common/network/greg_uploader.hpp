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

#include "file_uploader.hpp"

namespace Net
{
  class GregUploader : public FileUploader
  {
    public:
      GregUploader (const gchar *url,
                    const gchar *www);

      void SetTeamEvent    (gboolean     is_team_event);
      void SetRunningState (gboolean     is_over);
      void SetDate         (guint        day, guint month, guint year);
      void SetLocation     (const gchar *location);
      void SetLevel        (const gchar *level);
      void SetDivision     (const gchar *division);
      void SetWeapon       (const gchar *weapon);
      void SetGender       (guint        gender);
      void SetCategory     (const gchar *category);

    private:
      guint                 _current_job;
      gchar                **_job_url;
      gboolean              _has_location;
      GData                *_form_fields;
      struct curl_httppost *_form_head;
      struct curl_httppost *_form_tail;
      gchar                *_local_file;
      gchar                *_remote_file;

      virtual ~GregUploader ();

      void UploadFile (const gchar *file_path);

      const gchar *GetUrl ();

      void SetCurlOptions (CURL *curl);

      void Looper ();

      static void FeedFormField (GQuark        quark,
                                 const gchar  *value,
                                 GregUploader *uploader);
  };
}
