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

#ifndef swapper_hpp
#define swapper_hpp

namespace Pool
{
  class Swapper
  {
    public:
      virtual void Delete () = 0;

      virtual void Init (GSList *zones,
                         guint   fencer_count) = 0;

      virtual void Swap (GSList *criteria_list,
                         GSList *fencer_list) = 0;

      virtual guint HasErrors () = 0;

      virtual guint GetMoved () = 0;

    protected:
      Swapper () {};

      virtual ~Swapper () {};
  };
}

#endif
