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

#ifndef attribute_hpp
#define attribute_hpp

#include <stdarg.h>
#include <gtk/gtk.h>

#include "object.hpp"

// --------------------------------------------------------------------------------
class AttributeDesc : public Object_c
{
  public:
    typedef enum
    {
      SINGULAR,
      NOT_SINGULAR
    } Uniqueness;

    typedef enum
    {
      PUBLIC,
      PRIVATE
    } Rights;

    GType         _type;
    gchar        *_xml_name;
    gchar        *_user_name;
    Uniqueness    _uniqueness;
    Rights        _rights;
    GCompareFunc  _compare_func;

    static AttributeDesc *Declare (GType  type,
                                   gchar *xml_name,
                                   gchar *user_name);

    static void CreateList (GSList **list, ...);

    static AttributeDesc *GetDesc (gchar *name);

    void BindRenderer (GtkCellRenderer *renderer);

    void BindCellLayout (GtkCellLayout   *layout,
                         GtkCellRenderer *renderer);

    gboolean HasDiscreteValue ();

    void AddDiscreteValues (gchar *first_xml_image,
                            gchar *first_user_image,
                            ...);

    gchar *GetXmlImage (gchar *user_image);

    gchar *GetUserImage (gchar *xml_image);

    gchar *GetUserImage (GtkTreeIter *iter);

  private:
    static GSList *_list;
    GtkTreeStore *_discrete_store;

    AttributeDesc (GType  type,
                   gchar *xml_name,
                   gchar *user_name);

    ~AttributeDesc ();
};

// --------------------------------------------------------------------------------
class Attribute_c : public Object_c
{
  public:
    static Attribute_c *New (gchar *name);

    static gint Compare (Attribute_c *a, Attribute_c *b);

  public:
    AttributeDesc *GetDesc ();

    gchar *GetName ();

    GType GetType ();

    virtual void SetValue (gchar *value) = 0;

    virtual void SetValue (guint value) = 0;

    virtual void *GetValue () = 0;

    virtual gboolean EntryIsTextBased () = 0;

    virtual gchar *GetUserImage () = 0;

    virtual gchar *GetXmlImage () = 0;

    virtual gint CompareWith (Attribute_c *with) = 0;

  protected:
    AttributeDesc *_desc;

    Attribute_c (AttributeDesc *desc);
    virtual ~Attribute_c ();
};

// --------------------------------------------------------------------------------
class TextAttribute_c : public Attribute_c
{
  public:
     TextAttribute_c (AttributeDesc *desc);

    void *GetValue ();

  private:
    gchar *_value;

    virtual ~TextAttribute_c ();

    void SetValue (gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    gint CompareWith (Attribute_c *with);
};


// --------------------------------------------------------------------------------
class BooleanAttribute_c : public Attribute_c
{
  public:
     BooleanAttribute_c (AttributeDesc *desc);

    void *GetValue ();

  private:
    gboolean _value;

    virtual ~BooleanAttribute_c ();

    void SetValue (gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    gint CompareWith (Attribute_c *with);
};

// --------------------------------------------------------------------------------
class IntAttribute_c : public Attribute_c
{
  public:
     IntAttribute_c (AttributeDesc *desc);

    void *GetValue ();

  private:
    guint _value;

    virtual ~IntAttribute_c ();

    void SetValue (gchar *value);

    void SetValue (guint value);

    gboolean EntryIsTextBased ();

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    gint CompareWith (Attribute_c *with);
};

#endif
