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
    ~Attribute () override;
};

// --------------------------------------------------------------------------------
class TextAttribute : public Attribute
{
  public:
     TextAttribute (AttributeDesc *desc);

    char *GetStrValue () override;

  private:
    gchar *_value;

    ~TextAttribute () override;

    void SetValue (const gchar *value) override;

    void SetValue (guint value) override;

    gboolean EntryIsTextBased () override;

    gchar *GetUserImage (AttributeDesc::Look look) override;

    gchar *GetXmlImage () override;

    GdkPixbuf *GetPixbuf () override;

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look) override;

    gint CompareWith (Attribute *with) override;

    gint CompareWith (const gchar *with) override;

    Attribute *Duplicate () override;
};


// --------------------------------------------------------------------------------
class BooleanAttribute : public Attribute
{
  public:
    BooleanAttribute (AttributeDesc *desc);

    guint GetUIntValue () override;

  private:
    gboolean _value;

    ~BooleanAttribute () override;

    void SetValue (const gchar *value) override;

    void SetValue (guint value) override;

    gboolean EntryIsTextBased () override;

    gchar *GetUserImage (AttributeDesc::Look look) override;

    gchar *GetXmlImage () override;

    GdkPixbuf *GetPixbuf () override;

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look) override;

    gint CompareWith (Attribute *with) override;

    gint CompareWith (const gchar *with) override;

    Attribute *Duplicate () override;
};

// --------------------------------------------------------------------------------
class IntAttribute : public Attribute
{
  public:
    IntAttribute (AttributeDesc *desc);

    gint GetIntValue () override;

    guint GetUIntValue () override;

  private:
    guint _value;

    ~IntAttribute () override;

    void SetValue (const gchar *value) override;

    void SetValue (guint value) override;

    gboolean EntryIsTextBased () override;

    gchar *GetUserImage (AttributeDesc::Look look) override;

    gchar *GetXmlImage () override;

    GdkPixbuf *GetPixbuf () override;

    void TreeStoreSet (GtkTreeStore        *store,
                       GtkTreeIter         *iter,
                       gint                 column,
                       AttributeDesc::Look  look) override;

    gint CompareWith (Attribute *with) override;

    gint CompareWith (const gchar *with) override;

    Attribute *Duplicate () override;
};
