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

#ifndef module_hpp
#define module_hpp

#include <gtk/gtk.h>

#include "object.hpp"
#include "sensitivity_trigger.hpp"
#include "filter.hpp"
#include "glade.hpp"

class Player;
class Module : public virtual Object
{
  public:
    guint32  _rand_seed;

    void Plug (Module     *module,
               GtkWidget  *in,
               GtkToolbar *toolbar = NULL);
    void UnPlug ();

    virtual void Print (const gchar *job_name);

    void SelectAttributes ();

    void SetDataOwner (Object *data_owner);

    Object *GetDataOwner ();

    GtkWidget *GetConfigWidget ();

    GtkWidget *GetRootWidget ();

    GtkWidget *GetWidget (const gchar *name);

    virtual void SetFilter (Filter *filter);

    virtual void OnAttrListUpdated () {};

  protected:
    Filter  *_filter;
    Glade   *_glade;

    static const gdouble PRINT_HEADER_HEIGHT;
    static const gdouble PRINT_FONT_HEIGHT;

    static GKeyFile *_config_file;

    Module (const gchar *glade_file,
            const gchar *root = NULL);

    virtual void OnPlugged   () {};
    virtual void OnUnPlugged () {};

    GtkToolbar *GetToolbar ();

    void AddSensitiveWidget (GtkWidget *w);
    void EnableSensitiveWidgets ();
    void DisableSensitiveWidgets ();

    void SetCursor (GdkCursorType cursor_type);

    void ResetCursor ();

    GString *GetPlayerImage (Player *player);

    virtual ~Module ();

    GtkTreeModel *GetStatusModel ();

    virtual void OnDrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr);

  private:
    virtual void OnBeginPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context) {};
    virtual gboolean OnPreview (GtkPrintOperation        *operation,
                                GtkPrintOperationPreview *preview,
                                GtkPrintContext          *context,
                                GtkWindow                *parent) {return TRUE;};
    virtual void OnEndPrint (GtkPrintOperation *operation,
                             GtkPrintContext   *context) {};

  private:
    static GtkTreeModel *_status_model;

    GtkWidget          *_root;
    GtkToolbar         *_toolbar;
    SensitivityTrigger *_sensitivity_trigger;
    GSList             *_plugged_list;
    Module             *_owner;
    GtkWidget          *_config_widget;
    Object             *_data_owner;

    static void on_begin_print (GtkPrintOperation *operation,
                                GtkPrintContext   *context,
                                Module            *module);
    static gboolean on_preview (GtkPrintOperation        *operation,
                                GtkPrintOperationPreview *preview,
                                GtkPrintContext          *context,
                                GtkWindow                *parent,
                                Module                   *module);
    static void on_draw_page (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              gint               page_nr,
                              Module            *module);
    static void on_end_print (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              Module            *module);
};

#endif
