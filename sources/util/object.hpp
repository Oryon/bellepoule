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
#else
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[0;37m"
#define ESC     "\033[0m"
#endif

class FlashCode;

class Object
{
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

    virtual void Dump ();

    static void TryToRelease (Object *object);

    static void SetProgramPath (gchar *path);

    static void DumpList ();

    static void Track (const gchar *klass);

  protected:
    static gchar *_program_path;
    FlashCode    *_flash_code;

    virtual ~Object ();

    static gchar *GetUndivadableText (const gchar *text);

  private:
    GData       *_datalist;
    guint        _ref_count;
    const gchar *_class_name;

#ifdef DEBUG
    static GList       *_list;
    static guint        _nb_objects;
    static const gchar *_klass_to_track;
#endif
};

#endif
