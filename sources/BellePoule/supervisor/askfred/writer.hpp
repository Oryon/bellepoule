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
#include "util/xml_scheme.hpp"

namespace AskFred
{
  namespace Writer
  {
    class Element;

    class IdFixer : public Object
    {
      public:
        IdFixer (const gchar *primary_title,
                 gboolean     generate_primary_id);

        gboolean Knows (Element     *owner,
                        const gchar *id);

        void PushId (const gchar *name,
                     const gchar *value);

        const gchar *PopId ();

        const gchar *GetId (const gchar *of);

        const gchar *GetPrimaryTitle ();

        void Foreach (GDataForeachFunc  callback,
                      Object           *owner);

      private:
        gboolean     _generate_primary_id;
        const gchar *_primary_title;
        GData       *_populations;
        gchar       *_current_id;
        guint        _population_count;

        ~IdFixer ();
    };

    class Element : public Object
    {
      public:
        Element (const gchar *name,
                 GData       *scheme);

        const gchar *GetName ();

        GData *GetScheme ();

        void CountSeq ();

        guint GetSeq ();

        void SetInvisible ();

        gboolean IsVisible ();

      private:
        const gchar *_name;
        GData       *_scheme;
        guint        _seq;
        gboolean     _visible;

        ~Element ();
    };

    class Scheme : public XmlScheme
    {
      public:
        Scheme (const gchar *filename);

      private:
        static GList *_element_stack;
        static GData *_element_base;

        IdFixer *_fencer_id_fixer;
        IdFixer *_club_id_fixer;

        virtual ~Scheme ();

        void PushElement (const gchar *name);

        gboolean SaveFencersAndTeamsSeparatly ();

        gboolean CurrentElementIsVisible ();

        const gchar *Translate (const gchar *term);

        const gchar *TranslateValue (const gchar *term,
                                     const gchar *value);

        const gchar *TranslateVValue (const gchar *term,
                                      const gchar *format_value,
                                      va_list      argptr);

        void RemoveNonBreakingSpaces (guchar *term);

        void StartElement (const gchar *name);

        void EndElement ();

        void EndElement (const char *name);

        void WritePendingId (IdFixer *id_fixer);

        void WriteCustom (const gchar *what);

        static void WriteClub (GQuark   quark,
                               gchar   *id,
                               Object  *owner);
    };
  }
}
