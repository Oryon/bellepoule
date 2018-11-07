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

#include <locale.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "util/drop_zone.hpp"
#include "util/dnd_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/glade.hpp"
#include "util/wifi_code.hpp"
#include "util/player.hpp"
#include "network/message.hpp"
#include "network/ring.hpp"
#include "application/weapon.hpp"
#include "actors/referees_list.hpp"

#include "referee_pool.hpp"
#include "batch.hpp"
#include "competition.hpp"
#include "affinities.hpp"
#include "enlisted_referee.hpp"
#include "smartphones.hpp"

#include "marshaller.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Marshaller::Marshaller ()
    : Object ("Marshaller"),
    Module ("marshaller.glade")
  {
    Filter::PreventPersistence ();

    {
      Affinities::Manage ("country", "#fff36c");
      Affinities::Manage ("league",  "#ffb245");
      Affinities::Manage ("club",    "#ffa191");
    }

    _referee_pool = new RefereePool ();

    gtk_window_maximize (GTK_WINDOW (_glade->GetWidget ("root")));

    _referee_notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));

    {
      GList *current = Weapon::GetList ();

      for (guint page = 0 ; current; page++)
      {
        Weapon               *weapon   = (Weapon *) current->data;
        People::RefereesList *list     = new People::RefereesList (this);
        GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

        list->SetWeapon (weapon);
        list->SetData (this,
                       "page",
                       GUINT_TO_POINTER (page));
        _referee_pool->ManageList (list);

        gtk_notebook_append_page (_referee_notebook,
                                  viewport,
                                  list->GetHeader ());
        Plug (list,
              viewport);

        current = g_list_next (current);
      }
    }

    {
      _hall = new Hall (_referee_pool,
                        this);

      Plug (_hall,
            _glade->GetWidget ("hall_viewport"));
    }

    JobDetails::SetRefereePool (_referee_pool);
  }

  // --------------------------------------------------------------------------------
  Marshaller::~Marshaller ()
  {
    _hall->Release         ();
    _referee_pool->Release ();
    _smartphones->Release  ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::Start ()
  {
    _smartphones = new SmartPhones (_glade);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnShowAccessCode (gboolean with_steps)
  {
    {
      GtkWidget *w;

      w = _glade->GetWidget ("step1");
      gtk_widget_set_visible (w, with_steps);

      w = _glade->GetWidget ("step2");
      gtk_widget_set_visible (w, with_steps);
    }

    {
      GtkDialog *dialog = GTK_DIALOG (_glade->GetWidget ("pin_code_dialog"));

      {
        FlashCode *flash_code = Net::Ring::_broker->GetFlashCode ();
        GdkPixbuf *pixbuf     = flash_code->GetPixbuf ();
        GtkImage  *image      = GTK_IMAGE (_glade->GetWidget ("qrcode_image"));

        gtk_image_set_from_pixbuf (image,
                                   pixbuf);
        g_object_unref (pixbuf);
      }

      RunDialog (dialog);
      gtk_widget_hide (GTK_WIDGET (dialog));
    }
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnHanshakeResult (Net::Ring::HandshakeResult result)
  {
    if (result == Net::Ring::AUTHENTICATION_FAILED)
    {
      OnShowAccessCode (TRUE);
    }
    else if (result == Net::Ring::ROLE_REJECTED)
    {
      GtkWidget *dialog = _glade->GetWidget ("join_rejected_dialog");

      if (gtk_window_is_active (GTK_WINDOW (dialog)) == FALSE)
      {
        RunDialog (GTK_DIALOG (dialog));

        gtk_main_quit ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Marshaller::OnMessage (Net::Message *message)
  {
    if (message->GetFitness () > 0)
    {
      if (message->Is ("BellePoule::Competition"))
      {
        _hall->ManageCompetition (message,
                                  GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
        return TRUE;
      }
      else if (message->Is ("BellePoule::Batch"))
      {
        _hall->ManageBatch (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Job"))
      {
        _hall->ManageJob (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Referee"))
      {
        _referee_pool->ManageReferee (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Fencer"))
      {
        _hall->ManageFencer (message);
        return TRUE;
      }
      else if (message->Is ("SmartPoule::RefereeHandshake"))
      {
        _referee_pool->ManageHandShake (message);
        return TRUE;
      }
      else if (message->Is ("SmartPoule::PartnerAppCredentials"))
      {
        GtkDialog *dialog     = GTK_DIALOG (_glade->GetWidget ("pin_code_dialog"));
        gchar     *passphrase = message->GetString ("user_app_key");

        gtk_dialog_response (dialog,
                             GTK_RESPONSE_OK);

        if (passphrase)
        {
          Net::Ring::_broker->ChangePassphrase (passphrase);
          g_free (passphrase);

          Net::Ring::_broker->AnnounceAvailability ();
        }
      }
    }
    else
    {
      if (message->Is ("BellePoule::Competition"))
      {
        _hall->DeleteCompetition (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Batch"))
      {
        _hall->DeleteBatch (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Job"))
      {
        _hall->DeleteJob (message);
        return TRUE;
      }
      else if (message->Is ("BellePoule::Fencer"))
      {
        _hall->DeleteFencer (message);
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  const gchar *Marshaller::GetSecretKey (const gchar *authentication_scheme)
  {
    if (authentication_scheme)
    {
      gchar **tokens = g_strsplit_set (authentication_scheme,
                                       "/",
                                       0);

      if (tokens)
      {
        if (tokens[0] && tokens[1] && tokens[2])
        {
          WifiCode        *wifi_code = NULL;
          guint            ref       = atoi (tokens[2]);
          EnlistedReferee *referee   = _referee_pool->GetReferee (ref);

          if (referee)
          {
            wifi_code = (WifiCode *) referee->GetFlashCode ();
          }
          g_strfreev (tokens);

          if (wifi_code)
          {
            return wifi_code->GetKey ();
          }
        }
      }
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnRefereeUpdated (People::RefereesList *referee_list,
                                     Player               *referee)
  {
    for (GList *weapons = Weapon::GetList (); weapons; weapons = g_list_next (weapons))
    {
      Weapon               *weapon       = (Weapon *) weapons->data;
      People::RefereesList *current_list = _referee_pool->GetListOf (weapon->GetXmlImage ());

      if (current_list != referee_list)
      {
        current_list->Update (referee);
      }
    }

    _referee_pool->UpdateAffinities (referee);
    _hall->OnNewWarningPolicy ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnOpenCheckin (People::RefereesList *referee_list)
  {
    {
      GtkPaned      *paned      = GTK_PANED (_glade->GetWidget ("hpaned"));
      GtkAllocation  allocation;

      gtk_widget_get_allocation (GetRootWidget (),
                                 &allocation);

      gtk_paned_set_position (paned, allocation.width);
    }

    gtk_notebook_set_current_page (_referee_notebook,
                                   referee_list->GetIntData (this,
                                                             "page"));

    _referee_pool->ExpandAll ();

    gtk_notebook_set_show_tabs (_referee_notebook,
                                FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("hall_viewport"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("collapse"),
                            TRUE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnRefereeListCollapse ()
  {
    {
      GtkPaned *paned = GTK_PANED (_glade->GetWidget ("hpaned"));

      gtk_paned_set_position (paned, 0);
    }

    _referee_pool->CollapseAll ();

    gtk_notebook_set_show_tabs (_referee_notebook,
                                TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("hall_viewport"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("collapse"),
                            FALSE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnBlur ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("referee_vbox"),
                              FALSE);
    gtk_widget_set_sensitive (_glade->GetWidget ("batch_vbox"),
                              FALSE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnUnBlur ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("referee_vbox"),
                              TRUE);
    gtk_widget_set_sensitive (_glade->GetWidget ("batch_vbox"),
                              TRUE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnExposeWeapon (const gchar *weapon_code)
  {
    GList *current = Weapon::GetList ();

    for (guint i = 0; current != NULL; i++)
    {
      Weapon *weapon = (Weapon *) current->data;

      if (g_strcmp0 (weapon->GetXmlImage (), weapon_code) == 0)
      {
        gtk_notebook_set_current_page (_referee_notebook,
                                       i);
        break;
      }

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  GList *Marshaller::GetRefereeList ()
  {
    GList *full_list = NULL;
    GList *weapons   = Weapon::GetList ();

    while (weapons)
    {
      Weapon               *weapon       = (Weapon *) weapons->data;
      People::RefereesList *current_list = _referee_pool->GetListOf (weapon->GetXmlImage ());

      if (current_list->GetList ())
      {
        full_list = g_list_concat (full_list,
                                   g_list_copy (current_list->GetList ()));
      }
      weapons = g_list_next (weapons);
    }

    return full_list;
  }

  // --------------------------------------------------------------------------------
  guint Marshaller::PreparePrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context)
  {
    GList *referee_list = GetRefereeList ();
    guint  list_length  = g_list_length (referee_list);
    guint  n;

    if (_print_meal_tickets)
    {
      n = list_length / (NB_TICKET_Y_PER_SHEET * NB_TICKET_X_PER_SHEET);

      if (list_length % (NB_TICKET_Y_PER_SHEET * NB_TICKET_X_PER_SHEET))
      {
        n++;
      }
    }
    else
    {
      n = list_length / NB_REFEREE_PER_SHEET;

      if (list_length % NB_REFEREE_PER_SHEET)
      {
        n++;
      }
    }

    g_list_free (referee_list);
    return n;
  }

  // --------------------------------------------------------------------------------
  void Marshaller::DrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr)
  {
    GList         *referee_list = GetRefereeList ();
    gdouble        paper_w      = gtk_print_context_get_width (context);
    gdouble        paper_h      = gtk_print_context_get_height (context);
    cairo_t       *cr           = gtk_print_context_get_cairo_context (context);
    GooCanvas     *canvas       = Canvas::CreatePrinterCanvas (context);
    GooCanvasItem *main_table;
    GooCanvasItem *item;
    GList         *current;

    cairo_save (cr);

    if (_print_meal_tickets)
    {
      gdouble spacing  = 2.0;
      gdouble ticket_w = (100.0/NB_TICKET_X_PER_SHEET) - (NB_TICKET_X_PER_SHEET-1) *spacing;
      gdouble ticket_h = (100.0/NB_TICKET_Y_PER_SHEET) *paper_h/paper_w - (NB_TICKET_Y_PER_SHEET-1) *spacing;
      gdouble border_w = 0.7;

      main_table = goo_canvas_table_new (goo_canvas_get_root_item (canvas),
                                         "row-spacing",         2.0,
                                         "column-spacing",      2.0,
                                         "homogeneous-columns", TRUE,
                                         "homogeneous-rows",    TRUE,
                                         NULL);

      current = g_list_nth (referee_list,
                            NB_TICKET_Y_PER_SHEET*NB_TICKET_X_PER_SHEET*page_nr);

      for (guint r = 0; current && (r < NB_TICKET_Y_PER_SHEET); r++)
      {
        for (guint c = 0; current && (c < NB_TICKET_X_PER_SHEET); c++)
        {
          Player        *referee = (Player *) current->data;
          GooCanvasItem *ticket_table = goo_canvas_table_new (main_table,
                                                              "x-border-spacing", 1.0,
                                                              "y-border-spacing", 1.0,
                                                              NULL);
          // Border
          {
            item = goo_canvas_rect_new (main_table,
                                        0.0,
                                        0.0,
                                        ticket_w,
                                        ticket_h,
                                        "stroke-color", "Grey",
                                        "line-width",   border_w,
                                        NULL);
            Canvas::PutInTable (main_table,
                                item,
                                r,
                                c);
          }

          {
            {
              GooCanvasItem *name_table = goo_canvas_table_new (ticket_table, NULL);

              // Name
              {
                gchar               *font   = g_strdup_printf (BP_FONT "Bold %fpx", 2*PRINT_FONT_HEIGHT);
                Player::AttributeId  attr_id ("name");
                Attribute           *attr   = referee->GetAttribute (&attr_id);
                gchar               *string = attr->GetUserImage (AttributeDesc::LONG_TEXT);

                Canvas::NormalyzeDecimalNotation (font);
                item = Canvas::PutTextInTable (name_table,
                                               string,
                                               0,
                                               0);
                g_object_set (G_OBJECT (item),
                              "font",      font,
                              "ellipsize", PANGO_ELLIPSIZE_END,
                              "width",     ticket_w/4.0 - 2.0*border_w,
                              NULL);
                g_free (string);
                g_free (font);
              }

              // First name
              {
                gchar               *font   = g_strdup_printf (BP_FONT "%fpx", 1.5*PRINT_FONT_HEIGHT);
                Player::AttributeId  attr_id ("first_name");
                Attribute           *attr   = referee->GetAttribute (&attr_id);
                gchar               *string = attr->GetUserImage (AttributeDesc::LONG_TEXT);

                Canvas::NormalyzeDecimalNotation (font);
                item = Canvas::PutTextInTable (name_table,
                                               string,
                                               1,
                                               0);
                g_object_set (G_OBJECT (item),
                              "font", font,
                              "ellipsize", PANGO_ELLIPSIZE_END,
                              "width",     ticket_w/4.0 - 2.0*border_w,
                              NULL);
                g_free (string);
                g_free (font);
              }

              Canvas::PutInTable (ticket_table,
                                  name_table,
                                  0,
                                  0);
              Canvas::SetTableItemAttribute (name_table, "x-expand", 1U);
              Canvas::SetTableItemAttribute (name_table, "x-fill",   1U);
            }

            // Food
            {
              GooCanvasItem *food_table = goo_canvas_table_new (ticket_table, NULL);
              gchar         *font   = g_strdup_printf (BP_FONT "%fpx", 1.8*PRINT_FONT_HEIGHT);
              const gchar   *strings[] = {gettext ("Meal"),
                gettext ("Drink"),
                gettext ("Dessert"),
                gettext ("Coffee"), NULL};

              Canvas::NormalyzeDecimalNotation (font);

              g_object_set (G_OBJECT (food_table),
                            "horz-grid-line-width", 0.3,
                            "stroke-color", "White",
                            "fill-color",   "grey",
                            NULL);
              for (guint i = 0; strings[i] != NULL; i++)
              {
                item = Canvas::PutTextInTable (food_table,
                                               gettext (strings[i]),
                                               i,
                                               0);
                g_object_set (G_OBJECT (item),
                              "font",       font,
                              "fill-color", "Black",
                              NULL);
                Canvas::SetTableItemAttribute (item, "x-align", 1.0);
                Canvas::SetTableItemAttribute (item, "x-fill",   0U);
              }

              g_free (font);

              Canvas::PutInTable (ticket_table,
                                  food_table,
                                  0,
                                  1);
              Canvas::SetTableItemAttribute (food_table, "right-padding", 1.0);
            }
          }

          Canvas::PutInTable (main_table,
                              ticket_table,
                              r,
                              c);
          Canvas::SetTableItemAttribute (ticket_table, "x-expand", 1U);
          Canvas::SetTableItemAttribute (ticket_table, "x-fill",   1U);
          current = g_list_next (current);
        }
      }
    }
    else
    {
      gchar         *main_font = g_strdup_printf (BP_FONT "%fpx", 1.0*PRINT_FONT_HEIGHT);
      GooCanvasItem *header;

      Canvas::NormalyzeDecimalNotation (main_font);

      current = g_list_nth (referee_list,
                            NB_REFEREE_PER_SHEET*page_nr);

      main_table = goo_canvas_table_new (goo_canvas_get_root_item (canvas),
                                         "row-spacing",          4.0,
                                         "y-border-spacing",     2.0,
                                         "column-spacing",       10.0,
                                         "horz-grid-line-width", 0.2,
                                         "homogeneous-rows",     TRUE,
                                         NULL);

      {
        gchar *font = g_strdup_printf (BP_FONT "Bold %fpx", 3.0*PRINT_FONT_HEIGHT);

        Canvas::NormalyzeDecimalNotation (font);
        header = goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                                      gettext ("Payment book"),
                                      50.0,
                                      1.0,
                                      -1.0,
                                      GTK_ANCHOR_CENTER,
                                      "font", font,
                                      NULL);
        g_free (font);
      }

      {
        gchar *font   = g_strdup_printf (BP_FONT "Bold %fpx", 1.0*PRINT_FONT_HEIGHT);
        gchar *string = g_strdup_printf ("%s %d", gettext ("Page"), page_nr+1);

        Canvas::NormalyzeDecimalNotation (font);
        goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                             string,
                             100.0,
                             100.0 * paper_h/paper_w,
                             -1.0,
                             GTK_ANCHOR_SE,
                             "font", font,
                             NULL);
        g_free (string);
        g_free (font);
      }

      for (guint r = 0; current && (r < NB_REFEREE_PER_SHEET); r++)
      {
        Player *referee = (Player *) current->data;
        guint   c       = 0;

        {
          const gchar *column[] = {"name", "first_name", "level", NULL};

          for (guint i = 0; column[i] != NULL; i++)
          {
            Player::AttributeId  attr_id (column[i]);
            Attribute           *attr   = referee->GetAttribute (&attr_id);
            gchar               *string;

            if (attr)
            {
              string = attr->GetUserImage (AttributeDesc::LONG_TEXT);
            }
            else
            {
              string = g_strdup ("-");
            }

            item = Canvas::PutTextInTable (main_table,
                                           string,
                                           r,
                                           c);
            Canvas::SetTableItemAttribute (item, "y-align", 0.5);
            g_object_set (G_OBJECT (item),
                          "font", main_font,
                          NULL);
            g_free (string);
            c++;
          }
        }

        {
          struct lconv *local_conv = localeconv ();
          const gchar  *column[]   = {local_conv->currency_symbol, gettext ("Signature"), NULL};

          for (guint i = 0; column[i] != NULL; i++)
          {
            item = Canvas::PutTextInTable (main_table,
                                           gettext (column[i]),
                                           r,
                                           c);
            Canvas::SetTableItemAttribute (item, "x-align", 0.5);
            Canvas::SetTableItemAttribute (item, "y-align", 0.5);
            g_object_set (G_OBJECT (item),
                          "font", main_font,
                          "fill-color", "grey",
                          NULL);
            c++;
          }
        }

        current = g_list_next (current);
      }

      Canvas::Anchor (main_table,
                      header,
                      NULL,
                      40);
#if 0
      if (_print_scale != 1.0)
      {
        cairo_matrix_t matrix;

        goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                       &matrix);
        cairo_matrix_scale (&matrix,
                            _print_scale,
                            _print_scale);

        goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                       &matrix);
      }
#endif
    }

    goo_canvas_render (canvas,
                       cr,
                       NULL,
                       1.0);
    gtk_widget_destroy (GTK_WIDGET (canvas));

    cairo_restore (cr);
    g_list_free (referee_list);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnMenuDialog (const gchar *dialog)
  {
    GtkWidget *w = _glade->GetWidget (dialog);

    RunDialog (GTK_DIALOG (w));
    gtk_widget_hide (w);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::PrintMealTickets ()
  {
    _print_meal_tickets = TRUE;
    Print (gettext ("Meal tickets"));
  }

  // --------------------------------------------------------------------------------
  void Marshaller::PrintPaymentBook ()
  {
    _print_meal_tickets = FALSE;
    Print (gettext ("Payment book"));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_batch_notebook_switch_page (GtkNotebook *notebook,
                                                                 gpointer     page,
                                                                 guint        page_num,
                                                                 Object      *owner)
  {
    Marshaller  *m           = dynamic_cast <Marshaller *> (owner);
    Competition *competition = (Competition *) g_object_get_data (G_OBJECT (page), "competition");

    m->OnExposeWeapon (competition->GetWeaponCode ());
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_SmartPoule_button_clicked (GtkButton *widget,
                                                                gpointer   user_data)
  {
#ifdef WINDOWS_TEMPORARY_PATCH
    ShellExecute (NULL,
                  "open",
                  "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                  NULL,
                  NULL,
                  SW_SHOWNORMAL);
#else
    gtk_show_uri (NULL,
                  "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                  GDK_CURRENT_TIME,
                  NULL);
#endif
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_collapse_clicked (GtkToolButton *widget,
                                                       Object        *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnRefereeListCollapse ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_network_config_menuitem_activate (GtkWidget *w,
                                                                       Object    *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnMenuDialog ("network_dialog");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_scanner_menuitem_activate (GtkWidget *w,
                                                                Object    *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnMenuDialog ("scanner_dialog");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_tickets_menuitem_activate (GtkMenuItem *menuitem,
                                                                Object      *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->PrintMealTickets ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_payment_menuitem_activate (GtkMenuItem *menuitem,
                                                                Object      *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->PrintPaymentBook ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_access_code_activate (GtkWidget *w,
                                                           Object    *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnShowAccessCode (FALSE);
  }
}
