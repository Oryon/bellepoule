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

#include <gtk/gtk.h>

#include "object.hpp"
#include "attribute_desc.hpp"

class Attribute : public Object
{
  public:
    static Attribute *New (const gchar *name);

    static gint Compare (Attribute *a, Attribute *b);

  public:
    AttributeDesc *GetDesc ();

    gchar *GetCodeName ();

    gchar *GetXmlName ();

    GType GetType ();

    virtual void SetValue (const gchar *value) = 0;

    virtual void SetValue (guint value) = 0;

    virtual char *GetStrValue ();

    virtual gint GetIntValue ();

    virtual guint GetUIntValue ();

    virtual gboolean EntryIsTextBased () = 0;

    virtual gchar *GetUserImage (AttributeDesc::Look look) = 0;

    virtual gchar *GetXmlImage () = 0;

    virtual GdkPixbuf *GetPixbuf () = 0;

    virtual void TreeStoreSet (GtkTreeStore        *store,
                               GtkTreeIter         *iter,
                               gint                 column,
                               AttributeDesc::Look  look) = 0;

    virtual gint CompareWith (Attribute *with) = 0;

    virtual gint CompareWith (const gchar *with) = 0;

    virtual Attribute *Duplicate () = 0;

  protected:
    AttributeDesc *_desc;

    Attribute (AttributeDesc *desc);
    virtual ~Attribute ();
};

// --------------------------------------------------------------------------------
class TextAttribute : public Attribute
{
  public:
     TextAttribute (AttributeDesc *desc);

    char *GetStrValue ();

  private:
    gchar *_value;

    virtual ~TextAttribute ();

    void SetValue (const gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage (AttributeDesc::Look look);

    gchar *GetXmlImage ();

    GdkPixbuf *GetPixbuf ();

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look);

    gint CompareWith (Attribute *with);

    gint CompareWith (const gchar *with);

    Attribute *Duplicate ();
};


// --------------------------------------------------------------------------------
class BooleanAttribute : public Attribute
{
  public:
    BooleanAttribute (AttributeDesc *desc);

    guint GetUIntValue ();

  private:
    gboolean _value;

    virtual ~BooleanAttribute ();

    void SetValue (const gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage (AttributeDesc::Look look);

    gchar *GetXmlImage ();

    GdkPixbuf *GetPixbuf ();

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look);

    gint CompareWith (Attribute *with);

    gint CompareWith (const gchar *with);

    Attribute *Duplicate ();
};

// --------------------------------------------------------------------------------
class IntAttribute : public Attribute
{
  public:
    IntAttribute (AttributeDesc *desc);

    gint GetIntValue ();

    guint GetUIntValue ();

  private:
    guint _value;

    virtual ~IntAttribute ();

    void SetValue (const gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage (AttributeDesc::Look look);

    gchar *GetXmlImage ();

    GdkPixbuf *GetPixbuf ();

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look);

    gint CompareWith (Attribute *with);

    gint CompareWith (const gchar *with);

    Attribute *Duplicate ();
};
