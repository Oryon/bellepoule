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

#include "util/global.hpp"
#include "network/greg_uploader.hpp"

#include "ecosystem.hpp"

typedef enum
{
  FTP_NAME_str,
  FTP_PIXBUF_pix,
  FTP_FTP_str,
  FTP_USER_str,
  FTP_PASSWD_str,
  FTP_WWW_str
} FTPColumn;

// --------------------------------------------------------------------------------
EcoSystem::EcoSystem (Glade *glade)
  : Object ("EcoSystem")
{
  _glade = glade;

  g_signal_connect (_glade->GetWidget ("SSID_entry"),
                    "changed",
                    G_CALLBACK (EcoSystem::RefreshScannerCode), this);
  g_signal_connect (_glade->GetWidget ("security_combobox"),
                    "changed",
                    G_CALLBACK (EcoSystem::RefreshScannerCode), this);
  g_signal_connect (_glade->GetWidget ("passphrase_entry"),
                    "changed",
                    G_CALLBACK (EcoSystem::RefreshScannerCode), this);

  g_signal_connect (_glade->GetWidget ("ftp_comboboxentry"),
                    "changed",
                    G_CALLBACK (EcoSystem::OnRemoteHostChanged), this);

  // Remote FTP
  {
    GtkListStore *model  = GTK_LIST_STORE (_glade->GetGObject ("FavoriteFTP"));
    gchar        *path   = g_build_filename (Global::_share_dir, "resources", "glade", "images", "escrime_info.jpg", NULL);
    GdkPixbuf    *pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    GtkTreeIter   iter;

    g_free (path);
    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
                        FTP_NAME_str,   "<b><big>Escrime Info  </big></b>",
                        FTP_PIXBUF_pix, pixbuf,
                        FTP_FTP_str,    "escrime-info.com",
                        FTP_USER_str,   "GREG",
                        FTP_PASSWD_str, "cul qui gratte au couché doigt qui sent au levé",
                        FTP_WWW_str,    "http://www.escrime-info.com",
                        -1);
    g_object_unref (pixbuf);
  }

  // WIFI credentials
  {
    _wifi_network    = new Net::WifiNetwork ();
    _admin_wifi_code = new WifiCode ("Administrator");

    gtk_entry_set_visibility (GTK_ENTRY (_glade->GetWidget ("passphrase_entry")),
                              FALSE);

    RefreshScannerCode (NULL, this);
  }
}

// --------------------------------------------------------------------------------
EcoSystem::~EcoSystem ()
{
  _admin_wifi_code->Release ();
  _wifi_network->Release ();
}

// --------------------------------------------------------------------------------
void EcoSystem::RefreshScannerCode (GtkEditable *widget,
                                    EcoSystem   *ecosystem)
{
  GtkEntry *ssid_w       = GTK_ENTRY (ecosystem->_glade->GetWidget ("SSID_entry"));
  GtkEntry *passphrase_w = GTK_ENTRY (ecosystem->_glade->GetWidget ("passphrase_entry"));

  ecosystem->_wifi_network->SetSSID       ((gchar *) gtk_entry_get_text (ssid_w));
  ecosystem->_wifi_network->SetPassphrase ((gchar *) gtk_entry_get_text (passphrase_w));
  ecosystem->_wifi_network->SetEncryption ("WPA");

  ecosystem->_admin_wifi_code->SetWifiNetwork (ecosystem->_wifi_network);

  {
    GdkPixbuf *pixbuf = ecosystem->_admin_wifi_code->GetPixbuf ();

    gtk_image_set_from_pixbuf (GTK_IMAGE (ecosystem->_glade->GetWidget ("scanner_code_image")),
                               pixbuf);
    g_object_ref (pixbuf);
  }
}

// --------------------------------------------------------------------------------
WifiCode *EcoSystem::GetAdminCode ()
{
  return _admin_wifi_code;
}

// --------------------------------------------------------------------------------
Net::FileUploader *EcoSystem::GetUpLoader ()
{
  const gchar *url = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (_glade->GetWidget ("ftp_comboboxentry")))));

  if (url[0] == '\0')
  {
    return NULL;
  }
  else if (g_strrstr (url, "escrime-info.com"))
  {
    return new Net::GregUploader (url,
                                  gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("www"))));
  }
  else
  {
    return new Net::FileUploader (url,
                                  gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("user_entry"))),
                                  gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("passwd_entry"))),
                                  gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("www"))));
  }
}

// --------------------------------------------------------------------------------
void EcoSystem::OnRemoteHostChanged (GtkEditable *widget,
                                     EcoSystem   *ecosystem)
{
  GtkTreeIter iter;
  GtkComboBox *combo = GTK_COMBO_BOX (widget);

  if (gtk_combo_box_get_active_iter (combo,
                                     &iter))
  {
    gchar *ftp;
    gchar *user;
    gchar *passwd;
    gchar *www;

    gtk_tree_model_get (gtk_combo_box_get_model (combo),
                        &iter,
                        FTP_FTP_str,    &ftp,
                        FTP_USER_str,   &user,
                        FTP_PASSWD_str, &passwd,
                        FTP_WWW_str,    &www,
                        -1);

    gtk_entry_set_text (GTK_ENTRY (ecosystem->_glade->GetWidget ("user_entry")),
                        user);
    gtk_entry_set_text (GTK_ENTRY (ecosystem->_glade->GetWidget ("passwd_entry")),
                        passwd);
    gtk_entry_set_text (GTK_ENTRY (ecosystem->_glade->GetWidget ("www")),
                        www);

    g_free (ftp);
    g_free (user);
    g_free (passwd);
    g_free (www);
  }
}
