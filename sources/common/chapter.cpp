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

#include "chapter.hpp"

// --------------------------------------------------------------------------------
Chapter::Chapter (Module           *module,
                  Stage::StageView  stage_view,
                  guint             first_page,
                  guint             last_page)
: Object ("Chapter")
{
  _module     = module;
  _stage_view = stage_view;
  _first_page = first_page;
  _last_page  = last_page;
}

// --------------------------------------------------------------------------------
Chapter::~Chapter ()
{
}

// --------------------------------------------------------------------------------
guint Chapter::GetFirstPage ()
{
  return _first_page;
}

// --------------------------------------------------------------------------------
guint Chapter::GetLastPage ()
{
  return _last_page;
}

// --------------------------------------------------------------------------------
Module *Chapter::GetModule ()
{
  return _module;
}

// --------------------------------------------------------------------------------
Stage::StageView Chapter::GetStageView ()
{
  return _stage_view;
}
