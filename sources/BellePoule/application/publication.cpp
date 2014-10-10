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

#include "publication.hpp"

typedef enum
{
  FTP_NAME_str,
  FTP_PIXBUF_pix,
  FTP_URL_str,
  FTP_USER_str,
  FTP_PASSWD_str
} FTPColumn;

// --------------------------------------------------------------------------------
Publication::Publication (Glade *glade)
  : Object ("Publication")
{
  _glade = glade;

  // Publication repository
  {
    GtkListStore *model  = GTK_LIST_STORE (_glade->GetGObject ("FavoriteFTP"));
    gchar        *path   = g_build_filename (_program_path, "resources", "glade", "escrime_info.jpg", NULL);
    GdkPixbuf    *pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    GtkTreeIter   iter;

    g_free (path);
    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
                        FTP_NAME_str,   "<b><big>Escrime Info  </big></b>",
                        FTP_PIXBUF_pix, pixbuf,
                        FTP_URL_str,    "ftp://www.escrime-info.com/web",
                        FTP_USER_str,   "belle_poule",
                        FTP_PASSWD_str, "tH3MF8huHX",
                        -1);
    g_object_unref (pixbuf);
  }
}

// --------------------------------------------------------------------------------
Publication::~Publication ()
{
}

// --------------------------------------------------------------------------------
Net::Uploader *Publication::GetUpLoader ()
{
  return new Net::Uploader (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (_glade->GetWidget ("ftp_comboboxentry"))))),
                            NULL, NULL,
                            gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("user_entry"))),
                            gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("passwd_entry"))));
}

// --------------------------------------------------------------------------------
void Publication::OnRemoteHostChanged (GtkComboBox *widget)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (widget,
                                     &iter))
  {
    gchar *url;
    gchar *user;
    gchar *passwd;

    gtk_tree_model_get (gtk_combo_box_get_model (widget),
                        &iter,
                        FTP_URL_str,    &url,
                        FTP_USER_str,   &user,
                        FTP_PASSWD_str, &passwd,
                        -1);

    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("user_entry")),
                        user);
    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("passwd_entry")),
                        passwd);

    g_free (url);
    g_free (user);
    g_free (passwd);
  }
}
