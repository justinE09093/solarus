/*
 * Copyright (C) 2006-2012 Christopho, Solarus - http://www.solarus-games.org
 * 
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SOLARUS_CRYSTAL_BLOCK_H
#define SOLARUS_CRYSTAL_BLOCK_H

#include "Common.h"
#include "entities/Detector.h"

/**
 * @brief A block in the ground that can be raised or lowered with the help of a crystal.
 *
 * When the block is raised, the hero cannot traverse it. However, if the hero is
 * on raised blocks, he can walk on them until he jumps back to the normal ground.
 */
class CrystalBlock: public Detector {

  public:

    /**
     * Subtypes of raised blocks.
     */
    enum Subtype {
      ORANGE = 0, /**< an orange block */
      BLUE   = 1  /**< a blue block */
    };

  private:

    Subtype subtype;                   /**< indicates whether this is an orange or a blue block */
    bool orange_raised;                /**< true if the orange blocks are raised,
					* false if the blue blocks are raised */

    bool try_jump(Hero &hero, const Rectangle &collision_box,
	int jump_direction, int jump_length);

  public:

    CrystalBlock(Game &game, Layer layer, int x, int y, int width, int height, Subtype subtype);
    ~CrystalBlock();

    EntityType get_type();

    bool is_obstacle_for(MapEntity &other);
    void notify_collision(MapEntity &entity_overlapping, CollisionMode collision_mode);

    void update();
    void draw_on_map();

    bool is_raised();
};

#endif

