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

#include <stdarg.h>
#include <libxml/xmlwriter.h>

#include "util/object.hpp"

class XmlScheme : public virtual Object
{
  public:
    XmlScheme (const gchar *filename);

    XmlScheme (xmlBuffer *xml_buffer);

    virtual gboolean SaveFencersAndTeamsSeparatly ();

  public:
    void StartComment ();
    void EndComment ();

    void StartDTD (const gchar *name);
    void EndDTD ();

    virtual void StartElement (const gchar *element);
    virtual void EndElement ();

    void WriteAttribute (const gchar *attribute,
                         const gchar *value);

    void WriteFormatAttribute (const gchar *attribute,
                               const gchar *format_value,
                               ...);

    void WriteFormatString (const gchar *format_string,
                            ...);

    virtual void WriteCustom (const gchar *what);

  protected:
    ~XmlScheme () override;

  private:
    xmlTextWriter *_xml_writer;
    GList         *_elements;

    const gchar *GetCurrentElement ();

  private:
    virtual gboolean CurrentElementIsVisible ();

    virtual const gchar *Translate (const gchar *term);

    virtual const gchar *TranslateValue (const gchar *term,
                                         const gchar *value);

    virtual const gchar *TranslateVValue (const gchar *term,
                                          const gchar *format_value,
                                          va_list      argptr);
};
