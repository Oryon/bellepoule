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

#include <libxml/xmlwriter.h>

#include "xml_scheme.hpp"

// --------------------------------------------------------------------------------
XmlScheme::XmlScheme (const gchar *filename)
  : Object ("XmlScheme")
{
  _elements = nullptr;

  _xml_writer = xmlNewTextWriterFilename (filename,
                                          0);
  xmlTextWriterSetIndent (_xml_writer,
                          TRUE);
  xmlTextWriterStartDocument (_xml_writer,
                              nullptr,
                              "UTF-8",
                              nullptr);
}

// --------------------------------------------------------------------------------
XmlScheme::XmlScheme (xmlBuffer *xml_buffer)
  : Object ("XmlScheme")
{
  _elements = nullptr;

  _xml_writer = xmlNewTextWriterMemory (xml_buffer,
                                        0);
  xmlTextWriterSetIndent (_xml_writer,
                          FALSE);
}

// --------------------------------------------------------------------------------
XmlScheme::~XmlScheme ()
{
  if (_xml_writer)
  {
    xmlTextWriterEndDocument (_xml_writer);
    xmlFreeTextWriter (_xml_writer);
  }
}

// --------------------------------------------------------------------------------
const gchar *XmlScheme::GetCurrentElement ()
{
  if (_elements)
  {
    return (const gchar *) _elements->data;
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
void XmlScheme::StartComment ()
{
  xmlTextWriterStartComment (_xml_writer);
}

// --------------------------------------------------------------------------------
void XmlScheme::EndComment ()
{
  xmlTextWriterEndComment (_xml_writer);
}

// --------------------------------------------------------------------------------
void XmlScheme::StartDTD (const gchar *name)
{
  xmlTextWriterStartDTD (_xml_writer,
                         BAD_CAST name,
                         nullptr,
                         nullptr);
}

// --------------------------------------------------------------------------------
void XmlScheme::EndDTD ()
{
  xmlTextWriterEndDTD (_xml_writer);
}

// --------------------------------------------------------------------------------
void XmlScheme::StartElement (const gchar *element)
{
  _elements = g_list_prepend (_elements,
                              (gpointer) element);

  if (CurrentElementIsVisible ())
  {
    xmlTextWriterStartElement (_xml_writer,
                               BAD_CAST Translate (element));
  }
}

// --------------------------------------------------------------------------------
void XmlScheme::EndElement ()
{
  if (CurrentElementIsVisible ())
  {
    xmlTextWriterEndElement (_xml_writer);
  }

  _elements = g_list_delete_link (_elements,
                                  _elements);
}

// --------------------------------------------------------------------------------
gboolean XmlScheme::CurrentElementIsVisible ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean XmlScheme::SaveFencersAndTeamsSeparatly ()
{
  return FALSE;
}

// --------------------------------------------------------------------------------
void XmlScheme::WriteAttribute (const gchar *attribute,
                                const gchar *value)
{
  if (CurrentElementIsVisible ())
  {
    const gchar *translated_value = TranslateValue (attribute, value);

    if (translated_value)
    {
      xmlTextWriterWriteAttribute (_xml_writer,
                                   BAD_CAST Translate (attribute),
                                   BAD_CAST translated_value);
    }
  }
}

// --------------------------------------------------------------------------------
void XmlScheme::WriteFormatAttribute (const gchar *attribute,
                                      const gchar *format_value,
                                      ...)
{
  if (CurrentElementIsVisible ())
  {
    va_list vargs;
    va_list vargs_copy;

    va_start (vargs, format_value);
    va_copy (vargs_copy, vargs);

    {
      const gchar *translated_value = TranslateVValue (attribute, format_value, vargs_copy);

      if (translated_value)
      {
        xmlTextWriterWriteVFormatAttribute (_xml_writer,
                                            BAD_CAST Translate (attribute),
                                            translated_value,
                                            vargs);
      }
    }

    va_end (vargs_copy);
    va_end (vargs);
  }
}

// --------------------------------------------------------------------------------
const gchar *XmlScheme::TranslateVValue (const gchar *term,
                                         const gchar *format_value,
                                         va_list      argptr)
{
  return format_value;
}

// --------------------------------------------------------------------------------
void XmlScheme::WriteFormatString (const gchar *format_string,
                                   ...)
{
  va_list vargs;

  va_start (vargs, format_string);
  xmlTextWriterWriteVFormatString (_xml_writer,
                                   format_string,
                                   vargs);
  va_end (vargs);
}

// --------------------------------------------------------------------------------
void XmlScheme::WriteCustom (const gchar *what)
{
}

// --------------------------------------------------------------------------------
const gchar *XmlScheme::Translate (const gchar *term)
{
  return term;
}

// --------------------------------------------------------------------------------
const gchar *XmlScheme::TranslateValue (const gchar *term,
                                        const gchar *value)
{
  return value;
}
