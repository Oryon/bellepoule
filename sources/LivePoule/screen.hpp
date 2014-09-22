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

#ifndef screen_hpp
#define screen_hpp

#include <gtk/gtk.h>

#include "util/module.hpp"
#include "network/http_server.hpp"
#include "network/wifi_network.hpp"
#include "util/wifi_code.hpp"
#include "timer.hpp"
#include "gpio.hpp"
#include "light.hpp"
#include "scoring_machine.hpp"

class Screen : public Module, Net::HttpServer::Client
{
  public:
    Screen ();

    void ManageScoringMachine (ScoringMachine *machine);

    gboolean OnKeyPressed (GdkEventKey *event);

  private:
    static Screen   *_singleton;
    Net::HttpServer *_http_server;
    WifiCode        *_wifi_code;
    Timer           *_timer;
    GData           *_lights;
    guint            _strip_id;
    GList           *_scoring_machines;
    Gpio            *_qr_code_pin;
    Gpio            *_strip_plus_pin;
    Gpio            *_strip_minus_pin;

    virtual ~Screen ();

    void ResetDisplay ();

    void RefreshStripId ();

    void Rescale (gdouble factor);

    void ChangeNumber (gint step);

    void SetCompetition (GKeyFile *key_file);

    void SetTimer (GKeyFile *key_file);

    void SetScore (const gchar *light_color,
                   GKeyFile    *key_file);

    void SetColor (const gchar *color);

    void SetTitle (const gchar *title);

    void SetFencer (const gchar *light_color,
                    const gchar *data,
                    const gchar *name);

    gboolean OnHttpPost (const gchar *data);

    gchar *GetSecretKey (const gchar *authentication_scheme);

    void Unfullscreen ();

    void ToggleWifiCode ();

    void ChangeStripId (gint step);

  private:
    static gboolean HttpPostCbk (Net::HttpServer::Client *client,
                                 const gchar             *data);

    static void OnLightEvent ();

    static gboolean OnLightDefferedEvent ();

    static void OnQrCodeButton ();

    static void OnStripPlusPin ();

    static void OnStripMinusPin ();
};

#endif
