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

#include "util/object.hpp"

namespace Net
{
  class Message : public Object
  {
    public:
      Message (const gchar *name);

      Message (const guint8 *data);

      Message *Clone ();

      gboolean Is (const gchar *name);

      gboolean IsValid ();

      void SetNetID (guint id);

      guint GetNetID ();

      void Recall ();

      void SetPassPhrase256 (const gchar *passphrase256);

      const gchar *GetPassPhrase256 ();

      void SetFitness (const guint value);

      guint GetFitness ();

      void Set (const gchar *field,
                const gchar *value);

      void Set (const gchar *field,
                const guint  value);

      void SetList (const gchar *field,
                    const guint  list[],
                    gsize        length);

      void Set (const gchar *field,
                const gint   value);

      void Remove (const gchar *field);

      gchar *GetString (const gchar *field);

      guint GetInteger (const gchar *field);

      guint *GetIntegerList (const gchar *field,
                             gsize       *length);

      gint GetSignedInteger (const gchar *field);

      gchar *GetParcel ();

      void Dump (gboolean regular = TRUE);

      void Spread ();

      void ResetSpread ();

      gboolean IsSpread ();

    private:
      gboolean  _spread;
      gboolean  _is_valid;
      GKeyFile *_key_file;
      gchar    *_passphrase256;

      Message ();

      virtual ~Message ();
  };
}
