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

#ifndef object_hpp
#define object_hpp

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef WIN32
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#define MAGENTA ""
#define CYAN    ""
#define WHITE   ""
#define ESC     ""
#define BP_FONT "Sans "
#else
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[0;37m"
#define ESC     "\033[0m"
#if PANGO_VERSION_MAJOR == 1 && PANGO_VERSION_MINOR > 30
#define BP_FONT "Nimbus Sans L "
#else
#define BP_FONT "Sans "
#endif
#endif

#if GTK_MAJOR_VERSION >= 3
#define GTK_ANCHOR_CENTER GOO_CANVAS_ANCHOR_CENTER
#define GTK_ANCHOR_EAST   GOO_CANVAS_ANCHOR_EAST
#define GTK_ANCHOR_NORTH  GOO_CANVAS_ANCHOR_NORTH
#define GTK_ANCHOR_NW     GOO_CANVAS_ANCHOR_NW
#define GTK_ANCHOR_SE     GOO_CANVAS_ANCHOR_SE
#define GTK_ANCHOR_SW     GOO_CANVAS_ANCHOR_SW
#define GTK_ANCHOR_W      GOO_CANVAS_ANCHOR_W
#define GTK_ANCHOR_WEST   GOO_CANVAS_ANCHOR_WEST
#endif

#ifdef __GNUC__
#define __gcc_extension__ __extension__
#else
#define __gcc_extension__
#endif

class FlashCode;

namespace Net
{
  class Message;
}

class Object
{
  public:
    struct Listener
    {
      virtual void OnObjectDeleted (Object *object) = 0;
    };

  public:
    Object (const gchar *class_name = NULL);

    void SetData (Object         *owner,
                  const gchar    *key,
                  void           *data,
                  GDestroyNotify  destroy_cbk = NULL);

    void *GetPtrData (Object      *owner,
                      const gchar *key);

    guint GetUIntData (Object      *owner,
                       const gchar *key);

    gint GetIntData (Object      *owner,
                     const gchar *key);

    void RemoveData (Object      *owner,
                     const gchar *key);

    void Retain ();

    void Release ();

    void RemoveAllData ();

    void SetFlashRef (const gchar *ref);

    FlashCode *GetFlashCode ();

    void AddObjectListener (Listener *listener);

    void RemoveObjectListener (Listener *listener);

    virtual void Disclose (const gchar *as);

    virtual void Conceal ();

    virtual void FeedParcel (Net::Message *parcel);

    gboolean OnMessage (Net::Message *message);

    virtual void Spread ();

    virtual void Dump ();

    static void TryToRelease (Object *object);

    static void DumpList ();

    static void Track (const gchar *klass);

  protected:
    FlashCode    *_flash_code;
    Net::Message *_parcel;

    virtual ~Object ();

    static gchar *GetUndivadableText (const gchar *text);

  private:
    GData       *_datalist;
    guint        _ref_count;
    const gchar *_class_name;
    GList       *_listener_list;

#ifdef DEBUG
    static GList       *_list;
    static guint        _nb_objects;
    static const gchar *_klass_to_track;
#endif
};

#endif
