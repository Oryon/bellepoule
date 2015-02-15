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

#include "util/attribute.hpp"
#include "util/wifi_code.hpp"
#include "util/object.hpp"
#include "network/uploader.hpp"

class WifiCode;
class Weapon;

class Player : public Object
{
  public:
    class AttributeId : public Object
    {
      public:
        AttributeId (const gchar *name,
                     Object      *owner = NULL)
          : Object ("Player::AttributeId")
        {
          _name      = name;
          _owner     = owner;
          _rand_seed = 0;
        }

        void MakeRandomReady (guint32 rand_seed)
        {
          _rand_seed = rand_seed;
        };

        static AttributeId *Create (AttributeDesc *desc,
                                    Object        *owner);

        const gchar *_name;
        Object      *_owner;
        guint32      _rand_seed;
    };

    typedef void (*OnChange) (Player    *player,
                              Attribute *attr,
                              void      *data,
                              guint      step);

  public:
    gboolean Is (const gchar *player_class);

    Player *Duplicate ();

    virtual gboolean IsSticky ();

  public:
    static const guint BEFORE_CHANGE = 0x01;
    static const guint AFTER_CHANGE  = 0x02;

    Attribute *GetAttribute (AttributeId *attr_id);

    void SetAttributeValue (AttributeId *attr_id,
                            const gchar *value);

    void SetAttributeValue (AttributeId *attr_id,
                            guint        value);

    void SetAttribute (Attribute *attr);

    void RemoveAttribute (AttributeId *attr_id);

    void SetChangeCbk (const gchar *attr_name,
                       OnChange     change_cbk,
                       Object      *owner,
                       guint        steps = AFTER_CHANGE);

    void NotifyChange (const gchar *attr_name);

    void RemoveCbkOwner (Object *owner);

  public:
    guint GetDndRef ();

    guint GetRef ();
    void  SetRef (guint ref);

    Weapon *GetWeapon ();
    void  SetWeapon (Weapon *weapon);

    void  AddMatchs    (guint nb_matchs);
    void  RemoveMatchs (guint nb_matchs);
    guint GetNbMatchs  ();

    gchar *GetName ();
    void SetName (const gchar *name);
    void Dump ();
    FlashCode *GetFlashCode ();

  public:
    gboolean SendMessage (const gchar *where,
                          const gchar *message);

  public:
    virtual void Save (xmlTextWriter *xml_writer);

    virtual void Load (xmlNode *xml_node);

    virtual const gchar *GetXmlTag () = 0;

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

  protected:
    Player (const gchar *player_class);

    virtual Player *Clone () = 0;

    virtual ~Player ();

    void SaveAttributes (xmlTextWriter *xml_writer);

  private:
    struct Client : public Object
    {
      Client ()
        : Object ("Player::Client")
      {
        _attr_name = NULL;
      };

      virtual ~Client ()
      {
        g_free (_attr_name);
      };

      gchar    *_attr_name;
      OnChange  _change_cbk;
      Object   *_owner;
      guint     _steps;
    };

    GSList *_clients;

    static guint   _next_ref;
    static GSList *_attributes_model;

    guint        _dnd_ref;
    guint        _ref;
    guint        _nb_matchs;
    Weapon      *_weapon;
    const gchar *_player_class;
    WifiCode    *_wifi_code;

    void NotifyChange (Attribute *attr,
                       guint      step);

    static void OnUploaderStatus (Net::Uploader::PeerStatus  peer_status,
                                  Object                    *object);
};

#endif
