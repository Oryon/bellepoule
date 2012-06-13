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

#ifndef player_hpp
#define player_hpp

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

#include "attribute.hpp"
#include "object.hpp"

class Player : public Object
{
  public:
    class AttributeId : public Object
    {
      public:
        AttributeId (const gchar *name,
                     Object      *owner = NULL)
        {
          _name      = g_strdup (name);
          _owner     = owner;
          _rand_seed = 0;
        }

        void MakeRandomReady (guint32 rand_seed)
        {
          _rand_seed = rand_seed;
        };

        static AttributeId *Create (AttributeDesc *desc,
                                    Object        *owner);

        gchar   *_name;
        Object  *_owner;
        guint32  _rand_seed;
    };

    typedef void (*OnChange) (Player    *player,
                              Attribute *attr,
                              void      *data);

  public:
    typedef enum
    {
      FENCER,
      REFEREE
    } PlayerType;

    Player (PlayerType player_type);

    gboolean IsFencer ();

    Player *Duplicate ();

  public:
    Attribute *GetAttribute (AttributeId *attr_id);

    void SetAttributeValue (AttributeId *attr_id,
                            const gchar *value);

    void SetAttributeValue (AttributeId *attr_id,
                            guint        value);

    void SetChangeCbk (const gchar *attr_name,
                       OnChange     change_cbk,
                       Object      *owner);

    void RemoveCbkOwner (Object *owner);

  public:
    guint GetRef ();
    void  SetRef (guint ref);

    gchar GetWeaponCode ();
    void  SetWeaponCode (gchar weapon);

    void  AddMatchs    (guint nb_matchs);
    void  RemoveMatchs (guint nb_matchs);
    guint GetNbMatchs  ();

    gchar *GetName ();
    void Dump ();

  public:
    void Save (xmlTextWriter *xml_writer,
               const gchar   *player_tag);

    void Load (xmlNode *xml_node);

  public:
    static gint CompareWithRef (Player *player,
                                guint   ref);

    static gint Compare (Player      *a,
                         Player      *b,
                         AttributeId *attr_id);

    static gint MultiCompare (Player *a,
                              Player *b,
                              GSList *attr_list);

    static gint RandomCompare (Player  *A,
                               Player  *B,
                               guint32  rand_seed);

  private:
    struct Client : public Object
    {
      gchar    *_attr_name;
      OnChange  _change_cbk;
      Object   *_owner;
    };

    GSList *_clients;

    static guint   _next_ref;
    static GSList *_attributes_model;

    guint      _ref;
    guint      _nb_matchs;
    gchar      _weapon;
    PlayerType _player_type;

    ~Player ();

    void NotifyChange (Attribute *attr);
};

#endif
