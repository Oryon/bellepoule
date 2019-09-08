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

#include "util/user_config.hpp"

#include "global.hpp"

UserConfig *Global::_user_config = nullptr;
gchar      *Global::_share_dir   = nullptr;
gchar      *Global::_binary_dir  = nullptr;
gchar      *Global::_www         = nullptr;
#ifdef G_OS_WIN32
const gchar *Global::_www_port   = "";
#else
const gchar *Global::_www_port   = ":8080";
#endif
