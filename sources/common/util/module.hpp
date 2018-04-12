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
#include "sensitivity_trigger.hpp"

class DndConfig;
class Filter;
class Glade;

class Module : public virtual Object
{
  public:
    typedef enum
    {
      LOADING,
      OPERATIONAL,
      LEAVING
    } State;

    guint32 _rand_seed;

    void Plug (Module       *module,
               GtkWidget    *in,
               GtkToolbar   *toolbar          = NULL,
               GtkContainer *config_container = NULL);

    void UnPlug ();

    void AddSensitiveWidget (GtkWidget *w);

    void Print (const gchar *job_name,
                Object      *data = NULL);

    void PrintPDF (const gchar  *job_name,
                   const gchar  *filename);

    void PrintPreview (const gchar  *job_name,
                       Object      *data = NULL);

    void SelectAttributes ();

    void SetDataOwner (Object *data_owner);

    Object *GetDataOwner ();

    GtkWidget *GetConfigWidget ();

    GtkWidget *GetRootWidget ();

    GObject *GetGObject (const gchar *name);

    virtual void SetFilter (Filter *filter);

    virtual void OnAttrListUpdated () {};

    gboolean IsPlugged ();

    Filter *GetFilter ();

    virtual void MakeDirty ();

    virtual void DrawContainerPage (GtkPrintOperation *operation,
                                    GtkPrintContext   *context,
                                    gint               page_nr);

    virtual void OnEndPrint (GtkPrintOperation *operation,
                             GtkPrintContext   *context) {};

    virtual State GetState ();

    GdkPixbuf *GetPixbuf (const gchar *icon);

    virtual void DumpToHTML (FILE *file);

    DndConfig *GetDndConfig ();

  public:
    virtual guint PreparePrint (GtkPrintOperation *operation,
                                GtkPrintContext   *context) {return 0;};

    virtual void DrawPage (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr);

    virtual gchar *GetPrintName () {return NULL;};

  protected:
    DndConfig *_dnd_config;

  protected:
    typedef enum
    {
      ON_SHEET,
      NORMALIZED,
    } SizeReferential;

    Filter           *_filter;
    Glade            *_glade;
    Module           *_owner;
    GtkPrintSettings *_print_settings;
    GtkPrintSettings *_page_setup_print_settings;
    GtkPageSetup     *_default_page_setup;

    static const gdouble PRINT_HEADER_FRAME_HEIGHT;
    static const gdouble PRINT_FONT_HEIGHT;

    Module (const gchar *glade_file,
            const gchar *root = NULL);

    virtual void OnPlugged   () {};
    virtual void OnUnPlugged () {};

    GtkToolbar *GetToolbar ();

    GtkContainer *GetConfigContainer ();

    void EnableSensitiveWidgets ();
    void DisableSensitiveWidgets ();

    void SetCursor (GdkCursorType cursor_type);

    void ResetCursor ();

    virtual ~Module ();

    gint RunDialog (GtkDialog *dialog);

    GtkTreeModel *GetStatusModel ();

    void ConnectDndSource (GtkWidget *widget);

    void ConnectDndDest (GtkWidget *widget);

    virtual void OnBeginPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context);

    gdouble GetPrintHeaderSize (GtkPrintContext *context,
                                SizeReferential  referential);

    gdouble GetPrintBodySize (GtkPrintContext *context,
                              SizeReferential  referential);

    gdouble GetSizeOnSheet (GtkPrintContext *context,
                            gdouble          normalized_size);

  private:
    virtual void OnDrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr);
    virtual gboolean PreparePreview (GtkPrintOperation        *operation,
                                     GtkPrintOperationPreview *preview,
                                     GtkPrintContext          *context,
                                     GtkWindow                *parent) {return FALSE;};
    virtual void OnPreviewGotPageSize (GtkPrintOperationPreview *preview,
                                       GtkPrintContext          *context,
                                       GtkPageSetup             *page_setup) {};
    virtual void OnPreviewReady (GtkPrintOperationPreview *preview,
                                 GtkPrintContext          *context) {};

  private:
    static GtkTreeModel *_status_model;
    static GtkWindow    *_main_window;

    GtkWidget          *_root;
    GtkToolbar         *_toolbar;
    GtkContainer       *_config_container;
    SensitivityTrigger  _sensitivity_trigger;
    GSList             *_plugged_list;
    GtkWidget          *_config_widget;
    Object             *_data_owner;

    void Print (const gchar             *job_name,
                const gchar             *filename,
                Object                  *data,
                GtkPageSetup            *page_setup,
                GtkPrintOperationAction  action);

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
    static void on_preview_got_page_size (GtkPrintOperationPreview *preview,
                                          GtkPrintContext          *context,
                                          GtkPageSetup             *page_setup,
                                          Module                   *module);
    static void on_preview_ready (GtkPrintOperationPreview *preview,
                                  GtkPrintContext          *context,
                                  Module                   *module);

  protected:
    virtual gboolean OnDragMotion (GtkWidget      *widget,
                                   GdkDragContext *drag_context,
                                   gint            x,
                                   gint            y,
                                   guint           time);
    virtual void OnDragDataReceived (GtkWidget        *widget,
                                     GdkDragContext   *drag_context,
                                     gint              x,
                                     gint              y,
                                     GtkSelectionData *data,
                                     guint             info,
                                     guint             time);
    virtual void OnDragLeave (GtkWidget      *widget,
                              GdkDragContext *drag_context,
                              guint           time);
    virtual gboolean OnDragDrop (GtkWidget      *widget,
                                 GdkDragContext *drag_context,
                                 gint            x,
                                 gint            y,
                                 guint           time);

  private: // https://wiki.gnome.org/Newcomers/DragNDropTutorial
    virtual Object *GetDropObjectFromRef (guint32 ref,
                                          guint   key);

    static void DragDataGet (GtkWidget        *widget,
                             GdkDragContext   *drag_context,
                             GtkSelectionData *data,
                             guint             info,
                             guint             time,
                             Module           *owner);
    virtual void OnDragDataGet (GtkWidget        *widget,
                                GdkDragContext   *drag_context,
                                GtkSelectionData *data,
                                guint             info,
                                guint             time);

    static void DragEnd (GtkWidget      *widget,
                         GdkDragContext *drag_context,
                         Module         *owner);
    virtual void OnDragEnd (GtkWidget      *widget,
                            GdkDragContext *drag_context);

    static gboolean DragMotion (GtkWidget      *widget,
                                GdkDragContext *drag_context,
                                gint            x,
                                gint            y,
                                guint           time,
                                Module         *owner);

    static void DragLeave (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           guint           time,
                           Module         *owner);

    static gboolean DragDrop (GtkWidget      *widget,
                              GdkDragContext *drag_context,
                              gint            x,
                              gint            y,
                              guint           time,
                              Module         *owner);

    static void DragDataReceived (GtkWidget        *widget,
                                  GdkDragContext   *drag_context,
                                  gint              x,
                                  gint              y,
                                  GtkSelectionData *data,
                                  guint             info,
                                  guint             time,
                                  Module           *owner);
};
