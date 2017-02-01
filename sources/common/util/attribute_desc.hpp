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
#include <gtk/gtk.h>

#include "object.hpp"

class TreeModelIndex;

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
      LONG_TEXT,
      SHORT_TEXT,
      GRAPHICAL,

      NB_LOOK
    } Look;

    typedef enum
    {
      DISCRETE_CODE_uint,
      DISCRETE_XML_IMAGE_str,
      DISCRETE_LONG_TEXT_str,
      DISCRETE_SHORT_TEXT_str,
      DISCRETE_ICON_pix,
      DISCRETE_SELECTOR_str,

      NB_DISCRETE_COLUMNS
    } DiscreteColumnId;

    typedef gboolean (*CriteriaFunc) (AttributeDesc *desc);

    GType         _type;
    gchar        *_xml_name;
    gchar        *_code_name;
    gchar        *_user_name;
    Persistency   _persistency;
    Scope         _scope;
    Uniqueness    _uniqueness;
    Look          _favorite_look;
    gboolean      _free_value_allowed;
    Rights        _rights;
    GCompareFunc  _compare_func;
    GtkTreeModel *_discrete_model;
    gboolean      _is_selector;
    gboolean      _has_selector;

    static AttributeDesc *Declare (GType        type,
                                   const gchar *code_name,
                                   const gchar *xml_name,
                                   gchar       *user_name);

    static void CreateExcludingList (GSList **list, ...);

    static void CreateIncludingList (GSList **list, ...);

    static GList *GetList ();

    static void Cleanup ();

    static AttributeDesc *GetDescFromCodeName (const gchar *code_name);

    static AttributeDesc *GetDescFromXmlName (const gchar *xml_name);

    static AttributeDesc *GuessDescFromUserName (const gchar *code_name,
                                                 const gchar *criteria);

    void BindDiscreteValues (GObject     *object,
                             GtkComboBox *selector);

    void EnableSorting ();

    gboolean HasDiscreteValue ();

    void AddDiscreteValues (const gchar *first_xml_image,
                            const gchar *first_user_image,
                            const gchar *first_icon,
                            ...);

    void AddDiscreteValueSelector (const gchar *name);

    void AddLocalizedDiscreteValues (const gchar *name);

    gchar *GetDiscreteXmlImage (const gchar *from_user_image);

    gchar *GetDiscreteUserImage (guint from_code);

    GtkTreeIter *GetDiscreteIter (const gchar *from_user_image);

    GdkPixbuf *GetDiscretePixbuf (guint from_code);

    GdkPixbuf *GetDiscretePixbuf (const gchar *from_value);

    gchar *GetXmlImage (gchar *user_image);

    gchar *GetUserImage (gchar *xml_image,
                         Look   look);

    gchar *GetUserImage (GtkTreeIter *iter,
                         Look         look);

    gchar *GetXmlImage (GtkTreeIter *iter);

    static void Refilter (GtkComboBox *selector,
                          void        *data);

    void TreeStoreSetDefault (GtkTreeStore        *store,
                              GtkTreeIter         *iter,
                              gint                 column,
                              AttributeDesc::Look  look);

    GType GetGType (Look look);

    gboolean MatchCriteria (const gchar *criteria);

    static void SetCriteria (const gchar  *criteria,
                             CriteriaFunc  func);

    static void AddSwappable (AttributeDesc *desc);

    static GSList *GetSwappableList ();

  private:
    TreeModelIndex *_discrete_code_index;
    TreeModelIndex *_discrete_xml_index;
    TreeModelIndex *_discrete_long_text_index;
    TreeModelIndex *_discrete_short_text_index;

    void CreateDiscreteModel ();

  private:
    static GList        *_list;
    static GSList       *_swappable_list;
    static CriteriaFunc  _criteria_func;

    AttributeDesc (GType        type,
                   const gchar *code_name,
                   const gchar *xml_name,
                   const gchar *user_name);

    virtual ~AttributeDesc ();

    static gboolean DiscreteFilterForCombobox (GtkTreeModel *model,
                                               GtkTreeIter  *iter,
                                               GtkComboBox  *selector);

    void *GetDiscreteData (guint from_code,
                           guint column);

    void *GetDiscreteData (const gchar *from_user_image,
                           guint        image_type,
                           guint        column);

    void AddDiscreteValues (const gchar *dir,
                            const gchar *selector);

    const gchar *GetTranslation (const gchar *domain,
                                 const gchar *text);

    static gint CompareTextColumn (GtkTreeModel *model,
                                   GtkTreeIter  *a,
                                   GtkTreeIter  *b,
                                   gpointer      user_data);
};
