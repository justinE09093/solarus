/*
 * Copyright (C) 2009 Christopho, Zelda Solarus - http://www.zelda-solarus.com
 * 
 * Zelda: Mystery of Solarus DX is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Zelda: Mystery of Solarus DX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ZSDX_FAIRY_MOVEMENT_H
#define ZSDX_FAIRY_MOVEMENT_H

#include "Common.h"
#include "movements/Movement.h"

/**
 * Defines the movement of a pickable fairy.
 */
class FairyMovement: public Movement {

 private:
  
  /**
   * Date of the next direction change
   */
  uint32_t next_direction_change;

  void set_random_direction(void);

 public:

  FairyMovement(void);
  ~FairyMovement(void);

  void update(void);
};

#endif