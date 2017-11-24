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

#include <libxml/xmlwriter.h>

#include "util/object.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"

#include "pool_proxy.hpp"
#include "criteria.hpp"

#include "fencer_proxy.hpp"

namespace NeoSwapper
{
  // --------------------------------------------------------------------------------
  FencerProxy::FencerProxy (Player    *player,
                            guint      rank,
                            PoolProxy *original_pool,
                            guint      criteria_count)
    : Object ("SmartSwapper::FencerProxy")
  {
    _player        = player;
    _rank          = rank;
    _original_pool = original_pool;
    _new_pool      = original_pool;
    _criterias     = g_new0 (Criteria *, criteria_count);
  }

  // --------------------------------------------------------------------------------
  FencerProxy::~FencerProxy ()
  {
    g_free (_criterias);
  }

  // --------------------------------------------------------------------------------
  gboolean FencerProxy::HasQuark (guint  at_depth,
                                  GQuark quark)
  {
    if (_criterias[at_depth])
    {
    return _criterias[at_depth]->HasQuark (quark);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void FencerProxy::SetCriteria (guint     at_depth,
                                 Criteria *criteria)
  {
    _criterias[at_depth] = criteria;
  }

  // --------------------------------------------------------------------------------
  Criteria *FencerProxy::GetCriteria (guint at_depth)
  {
    return _criterias[at_depth];
  }

  // --------------------------------------------------------------------------------
  gint FencerProxy::CompareRank (FencerProxy *a,
                                 FencerProxy *b)
  {
    return a->_rank - b->_rank;
  }
}
