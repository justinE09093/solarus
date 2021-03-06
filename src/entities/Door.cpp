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
#include "entities/MapEntities.h"
#include "entities/Door.h"
#include "entities/Hero.h"
#include "entities/DynamicTile.h"
#include "lua/LuaContext.h"
#include "lowlevel/FileTools.h"
#include "lowlevel/Debug.h"
#include "lowlevel/StringConcat.h"
#include "lowlevel/Sound.h"
#include "lowlevel/System.h"
#include "lowlevel/Geometry.h"
#include "Sprite.h"
#include "Game.h"
#include "DialogBox.h"
#include "Equipment.h"
#include "KeysEffect.h"
#include "Savegame.h"
#include "Map.h"
#include <list>
#include <sstream>

const std::string Door::animations[] = {
  "closed", "small_key", "small_key_block", "big_key", "boss_key", "weak", "very_weak", "", "weak_block"
};

const std::string Door::key_required_dialog_ids[] = {
  "", "_small_key_required", "_small_key_required", "_big_key_required", "_boss_key_required", "", "", "", ""
};

/**
 * @brief Creates a door.
 * @param game The game.
 * @param name name identifying this entity
 * @param layer layer of the entity to create
 * @param x x coordinate of the entity to create
 * @param y y coordinate of the entity to create
 * @param direction direction of the door
 * @param subtype the subtype of door
 * @param savegame_variable variable where the door's state is saved
 * (can be -1 for the subtype CLOSED)
 */
Door::Door(Game& game, const std::string& name, Layer layer, int x, int y,
	     int direction, Subtype subtype, const std::string& savegame_variable):
  Detector(COLLISION_FACING_POINT | COLLISION_SPRITE, name, layer, x, y, 16, 16),
  subtype(subtype),
  savegame_variable(savegame_variable),
  door_open(true),
  changing(false),
  initialized(false),
  next_hint_sound_date(0) {

  if (subtype == SMALL_KEY_BLOCK || subtype == WEAK_BLOCK) {
    set_size(16, 16);
  }
  else if (direction % 2 == 0) {
    set_size(16, 32);
  }
  else {
    set_size(32, 16);
  }

  Sprite& sprite = create_sprite("entities/door", true);
  sprite.set_ignore_suspend(true); // allow the animation while the camera is moving
  set_direction(direction);

  if (is_saved()) {
    set_open(game.get_savegame().get_boolean(savegame_variable));
  }
  else {
    set_open(false);
  }
  sprite.set_current_direction(direction);
}

/**
 * @brief Destructor.
 */
Door::~Door() {

}

/**
 * @brief Returns the type of entity.
 * @return the type of entity
 */
EntityType Door::get_type() {
  return DOOR;
}

/**
 * @brief Returns whether this entity is an obstacle for another one.
 * @param other another entity
 * @return true
 */
bool Door::is_obstacle_for(MapEntity &other) {
  return !is_open() || changing;
}

/**
 * @brief Returns whether this door is open.
 * @return true if this door is open
 */
bool Door::is_open() {
  return door_open;
}

/**
 * @brief Makes the door opened or closed.
 * @param door_open true to make it opened, false to make it closed
 */
void Door::set_open(bool door_open) {
  
  this->door_open = door_open;

  if (door_open) {
    set_collision_modes(COLLISION_NONE); // to avoid being the hero's facing entity
  }
  else {
    get_sprite().set_current_animation(animations[subtype]);
    set_collision_modes(COLLISION_FACING_POINT | COLLISION_SPRITE);

    // ensure that we are not closing the door on the hero
    if (is_on_map() && overlaps(get_hero())) {
      get_hero().avoid_collision(*this, (get_direction() + 2) % 4);
    }
  }

  if (is_on_map()) {
    update_dynamic_tiles();

    if (is_saved()) {
      get_savegame().set_boolean(savegame_variable, door_open);
    }

    if (door_open) {
      get_lua_context().door_on_open(*this);
    }
    else {
      get_lua_context().door_on_closed(*this);
    }
  }
}

/**
 * @brief Enables or disables the dynamic tiles related to this door.
 *
 * The dynamic tiles impacted by this function are the ones whose prefix is the door's name
 * followed by "_closed" or "_open", depending on the door state.
 */
void Door::update_dynamic_tiles() {

  std::list<MapEntity*> tiles = get_entities().get_entities_with_prefix(DYNAMIC_TILE, get_name() + "_closed");
  std::list<MapEntity*>::iterator it;
  for (it = tiles.begin(); it != tiles.end(); it++) {
    DynamicTile *tile = (DynamicTile*) *it;
    tile->set_enabled(!door_open);
  }

  tiles = get_entities().get_entities_with_prefix(DYNAMIC_TILE, get_name() + "_open");
  for (it = tiles.begin(); it != tiles.end(); it++) {
    DynamicTile *tile = (DynamicTile*) *it;
    tile->set_enabled(door_open);
  }
}

/**
 * @brief Notifies this detector that a collision was just detected with another entity.
 *
 * This function is called by the engine when there is a collision with another entity.
 *
 * @param entity_overlapping the entity overlapping the detector
 * @param collision_mode the collision mode that detected the collision
 */
void Door::notify_collision(MapEntity &entity_overlapping, CollisionMode collision_mode) {

  if (!is_open()
      && entity_overlapping.is_hero()
      && requires_key()
      && !is_changing()) {

    Hero &hero = (Hero&) entity_overlapping;

    if (get_keys_effect().get_action_key_effect() == KeysEffect::ACTION_KEY_NONE
	&& hero.is_free()) {

      // we show the action icon
      get_keys_effect().set_action_key_effect(can_open() ? KeysEffect::ACTION_KEY_OPEN : KeysEffect::ACTION_KEY_LOOK);
    }
  }
}

/**
 * @brief Notifies this detector that a pixel-perfect collision was just detected with another sprite.
 *
 * This function is called by check_collision(MapEntity*, Sprite*) when another entity's
 * sprite overlaps a sprite of this detector.
 *
 * @param other_entity the entity overlapping this detector
 * @param other_sprite the sprite of other_entity that is overlapping this detector
 * @param this_sprite the sprite of this detector that is overlapping the other entity's sprite
 */
void Door::notify_collision(MapEntity &other_entity, Sprite &other_sprite, Sprite &this_sprite) {

  if (other_entity.get_type() == EXPLOSION) {
    notify_collision_with_explosion((Explosion&) other_entity, other_sprite);
  }
}

/**
 * @brief This function is called when an explosion's sprite
 * detects a pixel-perfect collision with a sprite of this entity.
 * @param explosion the explosion
 * @param sprite_overlapping the sprite of the current entity that collides with the explosion
 */
void Door::notify_collision_with_explosion(Explosion &explosion, Sprite &sprite_overlapping) {

  if (requires_explosion() && !is_open() && !changing) {
    set_opening();
  }
}

/**
 * @brief Returns whether the state of this door is saved.
 * @return true if this door is saved.
 */
bool Door::is_saved() {
  return !savegame_variable.empty();
}

/**
 * @brief Returns whether this door requires a key to be open.
 * @return true if this door requires a key to be open
 */
bool Door::requires_key() {
  return requires_small_key() || subtype == BIG_KEY || subtype == BOSS_KEY;
}

/**
 * @brief Returns whether this door must be open with a small key.
 * @return true if this door must be open with a small key
 */
bool Door::requires_small_key() {
  return subtype == SMALL_KEY || subtype == SMALL_KEY_BLOCK;
}

/**
 * @brief Returns whether this door must be open with an explosion.
 * @return true if this door must be open with an explosion
 */
bool Door::requires_explosion() {
  return subtype == WEAK || subtype == VERY_WEAK || subtype == WEAK_BLOCK;
}

/**
 * @brief Returns whether the player has the right key to open this door.
 *
 * If the door cannot be open with a key, false is returned.
 *
 * @return true if the player has the key corresponding to this door
 */
bool Door::can_open() {

  // TODO dungeons and small keys are no longer hardcoded: reimplement doors with general properties
  return true;
}

/**
 * @brief Suspends or resumes the entity.
 * @param suspended true to suspend the entity
 */
void Door::set_suspended(bool suspended) {

  Detector::set_suspended(suspended);

  if (!suspended && next_hint_sound_date > 0) {
    next_hint_sound_date += System::now() - when_suspended;
  }
}

/**
 * @brief Updates the entity.
 */
void Door::update() {

  Detector::update();

  if (!initialized) {
    update_dynamic_tiles();
    initialized = true;
  }

  if (!is_open()
      && requires_explosion()
      && subtype != WEAK_BLOCK
      && get_equipment().has_ability("detect_weak_walls")
      && Geometry::get_distance(get_center_point(), get_hero().get_center_point()) < 40
      && !is_suspended()
      && System::now() >= next_hint_sound_date) {
    Sound::play("cane");
    next_hint_sound_date = System::now() + 500;
  }

  if (changing && get_sprite().is_animation_finished()) {
    changing = false;
    set_open(!is_open());
  }

  if (is_saved()) {
    bool open_in_savegame = get_savegame().get_boolean(savegame_variable);
    if (open_in_savegame && !is_open() && !changing) {
      set_opening();
    }
    else if (!open_in_savegame && is_open() && !changing) {
      set_closing();
    }
  }
}

/**
 * @brief Draws the entity on the map.
 */
void Door::draw_on_map() {

  if (has_sprite() && (!is_open() || changing)) {
    Detector::draw_on_map();
  }
}

/**
 * @brief Notifies this detector that the player is interacting with it by
 * pressing the action command.
 *
 * This function is called when the player presses the action command
 * while the hero is facing this detector, and the action command effect lets
 * him do this.
 * The hero opens the door if possible, otherwise a message is shown.
 */
void Door::notify_action_command_pressed() {

  if (get_hero().is_free() && requires_key() && !is_changing()) {
    if (can_open()) {
      Sound::play("door_unlocked");
      Sound::play("door_open");

      if (is_saved()) {
        get_savegame().set_boolean(savegame_variable, true);
      }

      if (subtype == SMALL_KEY_BLOCK || subtype == WEAK_BLOCK) {
        set_open(true);
      }
      else {
        set_opening();
      }

      /* TODO replace by if (requires_item() && item.has_counter()) ... 
      if (requires_small_key()) {
        get_equipment().remove_small_key();
      }
      */

      get_hero().check_position();
    }
    else {
      Sound::play("wrong");
      get_dialog_box().start_dialog(key_required_dialog_ids[subtype]);
    }
  }
}

/**
 * @brief This function is called when the player is tapping his sword against this detector.
 * @return the sound to play when tapping this detector with the sword
 */
std::string Door::get_sword_tapping_sound() {
  return requires_explosion() ? "sword_tapping_weak_wall" : "sword_tapping";
}

/**
 * @brief Starts opening the door and plays the corresponding animations.
 *
 * This function can be called only for a door with subtype CLOSED.
 * Nothing is done if the door is already in the process of being open.
 */
void Door::open() {

  Debug::check_assertion(subtype == CLOSED, "This kind of door cannot be open or closed directly");

  Debug::check_assertion(!is_open() || changing,
      StringConcat() << "Door '" << get_name() << "' is already open");

  if (changing) {
    if (is_open()) {
      // the door is being closed: mark it as closed so that we can open it
      door_open = false;
    }
    else {
      // the door is already being open: nothing to do
      return;
    }
  }

  set_opening();

  if (is_saved()) {
    get_savegame().set_boolean(savegame_variable, true);
  }
}

/**
 * @brief Makes the door being opened.
 */
void Door::set_opening() {

  std::string animation = "";
  if (requires_key()) {
    animation = "opening_key";
  }
  else if (!requires_explosion()) {
    animation = "opening";
  }
  // TODO add the animation of a weak wall destroyed by an explosion

  if (animation.size() > 0) {
    get_sprite().set_current_animation(animation);
    changing = true;
  }
  else {
    set_open(true);
  }
}

/**
 * @brief Starts closing the door and plays the corresponding animations.
 *
 * This function can be called only for a door with subtype CLOSED.
 * Nothing is done if the door is already in the process of being closed.
 */
void Door::close() {

  Debug::check_assertion(is_open() || changing,
      StringConcat() << "Door '" << get_name() << "' is already closed");

  if (changing) {
    if (!is_open()) {
      // the door is being open: mark it as open so that we can close it
      door_open = true;
    }
    else {
      // the door is already being closed: nothing to do
      return;
    }
  }

  set_closing();

  if (is_saved()) {
    get_savegame().set_boolean(savegame_variable, false);
  }
}

/**
 * @brief Makes the door being closed.
 */
void Door::set_closing() {

  get_sprite().set_current_animation("opening");
  changing = true;
}

/**
 * @brief Returns true if the door is currently being open or closed.
 * @return true if the door is currently being open or closed
 */
bool Door::is_changing() {
  return changing;
}

/**
 * @brief Returns the name identifying this type in Lua.
 * @return The name identifying this type in Lua.
 */
const std::string& Door::get_lua_type_name() const {
  return LuaContext::entity_door_module_name;
}

