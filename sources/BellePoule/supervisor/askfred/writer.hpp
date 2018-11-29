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
                        const gchar *name);

        void PushId (const gchar *name,
                     const gchar *value);

        const gchar *GetId (const gchar *of);

        void Foreach (GDataForeachFunc  callback,
                      Object           *owner);

      private:
        gboolean     _generate_primary_id;
        const gchar *_primary_title;
        GData       *_populations;
        guint        _population_count;

        ~IdFixer () override;
    };

    class Round : public Object
    {
      public:
        Round ();

        void Withdraw (const gchar *id,
                       gpointer     data);

        gboolean IsWithdrawn (const gchar *id);

        static void Delete (Round *round);

      private:
        GData *_table;

        ~Round () override;
    };

    class Element : public Object
    {
      public:
        Element (const gchar *name,
                 GData       *scheme,
                 Round       *round);

        static gboolean DeleteNode (GNode    *node,
                                    gpointer  data);

        const gchar *GetName ();

        GData *GetScheme ();

        void CountSeq ();

        void CancelSeq ();

        guint GetSeq ();

        void MakeInvisible ();

        GList *GetOutline ();

        void Withdraw ();

        gboolean IsWithdrawn ();

        void StartOutline (const gchar *name);

        void AddToOutline (const gchar *name,
                           const gchar *value);

        const gchar *GetValue (const gchar *of);

      private:
        gchar       *_name;
        const gchar *_ref;
        GData       *_scheme;
        guint        _seq;
        gboolean     _visible;
        GList       *_outline;
        Round       *_round;

        ~Element () override;
    };

    class Scheme : public XmlScheme
    {
      public:
        Scheme (const gchar *filename);

      private:
        typedef enum
        {
          COLLECTING_MODE,
          POST_COLLECTING_MODE,
          FLUSHING_MODE
        } Mode;

        static GData *_element_base;

        GNode *_root_element;
        GNode *_current_node;
        GList *_rounds;

        IdFixer    *_fencer_id_fixer;
        IdFixer    *_club_id_fixer;
        GData      *_id_aliases;

        Mode _mode;

        ~Scheme () override;

        gboolean FlushElement (Element *element,
                               Element *parent);

        gboolean FlushNode (GNode *node);

        static gboolean CheckVisibilityNodeFunc (GNode  *node,
                                                 Scheme *scheme);

        static gboolean FlushNodeFunc (GNode  *node,
                                       Scheme *scheme);

        Element *PushElement (const gchar *name);

        gboolean SaveFencersAndTeamsSeparatly () override;

        const gchar *Translate (const gchar *term) override;

        const gchar *TranslateValue (const gchar *term,
                                     const gchar *value) override;

        const gchar *TranslateVValue (const gchar *term,
                                      const gchar *format_value,
                                      va_list      argptr) override;

        void RemoveNonBreakingSpaces (guchar *term);

        void StartElement (const gchar *name) override;

        void Flush (GNode *from);

        void EndElement () override;

        void EndElement (const char *name);

        void WriteCustom (const gchar *what) override;

        static void WriteClub (GQuark   quark,
                               gchar   *id,
                               Object  *owner);
    };
  }
}
