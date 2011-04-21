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
class AttributeDesc : public Object
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

    typedef enum
    {
      PERSISTENT,
      NOT_PERSISTENT
    } Persistency;

    typedef enum
    {
      GLOBAL,
      LOCAL
    } Scope;

    typedef enum
    {
      DISCRETE_CODE,
      DISCRETE_XML_IMAGE,
      DISCRETE_USER_IMAGE,
      DISCRETE_ICON,
      DISCRETE_ICON_NAME
    } DiscreteColumnId;

    GType         _type;
    gchar        *_xml_name;
    gchar        *_code_name;
    gchar        *_user_name;
    Persistency   _persistency;
    Scope         _scope;
    Uniqueness    _uniqueness;
    gboolean      _free_value_allowed;
    Rights        _rights;
    GCompareFunc  _compare_func;
    GtkTreeStore *_discrete_store;

    static void SetPath (gchar *path);

    static AttributeDesc *Declare (GType        type,
                                   const gchar *code_name,
                                   const gchar *xml_name,
                                   gchar       *user_name);

    static void CreateList (GSList **list, ...);

    static GSList *GetList ();

    static AttributeDesc *GetDesc (const gchar *name);

    void BindDiscreteValues (GObject         *object,
                             GtkCellRenderer *renderer = NULL);

    gboolean HasDiscreteValue ();

    void AddDiscreteValues (const gchar *first_xml_image,
                            gchar       *first_user_image,
                            const gchar *first_icon,
                            ...);

    void AddDiscreteValues (const gchar *file);

    gchar *GetDiscreteUserImage (guint from_code);

    gchar *GetDiscreteIcon (guint from_code);

    gchar *GetXmlImage (gchar *user_image);

    gchar *GetUserImage (gchar *xml_image);

    gchar *GetUserImage (GtkTreeIter *iter);

  private:
    static gchar  *_path;
    static GSList *_list;

    AttributeDesc (GType        type,
                   const gchar *code_name,
                   const gchar *xml_name,
                   const gchar *user_name);

    ~AttributeDesc ();

    void *GetDiscreteData (guint from_code,
                           guint column);
};

// --------------------------------------------------------------------------------
class Attribute : public Object
{
  public:
    static Attribute *New (gchar *name);

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

    virtual gchar *GetUserImage () = 0;

    virtual gchar *GetXmlImage () = 0;

    virtual void *GetListStoreValue () = 0;

    virtual gint CompareWith (Attribute *with) = 0;

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

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    void *GetListStoreValue ();

    gint CompareWith (Attribute *with);

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

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    void *GetListStoreValue ();

    gint CompareWith (Attribute *with);

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

    gchar *GetUserImage ();

    gchar *GetXmlImage ();

    void *GetListStoreValue ();

    gint CompareWith (Attribute *with);

    Attribute *Duplicate ();
};

#endif
