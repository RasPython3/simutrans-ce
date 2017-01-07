/*
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef HAUSBAUER_H
#define HAUSBAUER_H

#include "../besch/haus_besch.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

class gebaeude_t;
class karte_ptr_t;
class player_t;
class tool_selector_t;

/**
 * This class deals with building single- and multi-tile buildings. It knows the descriptions
 * of (nearly) all buildings as regards type, height, size, images and animations.
 * To be able to build a new house the house description must be registered by register_desc().
 *
 * To ensure all monuments are only present once per map, there is a list
 * which monuments have been built and which not.
 * There's no need to construct an instance since everything is static here.
 */
class hausbauer_t
{
private:
	static vector_tpl<const building_desc_t*> sehenswuerdigkeiten_land;  ///< Sights outside of cities
	static vector_tpl<const building_desc_t*> sehenswuerdigkeiten_city;  ///< Sights within cities
	static vector_tpl<const building_desc_t*> rathaeuser;                ///< Town halls
	static vector_tpl<const building_desc_t*> denkmaeler;                ///< All monuments
	static vector_tpl<const building_desc_t*> ungebaute_denkmaeler;      ///< All unbuilt monuments
	static vector_tpl<const building_desc_t*> headquarter;               ///< Company headquarters
	static vector_tpl<const building_desc_t*> station_building;          ///< All station buildings

	/// @returns a random entry from @p liste
	static const building_desc_t* waehle_aus_liste(vector_tpl<const building_desc_t*>& liste, uint16 time, bool ignore_retire, climate cl);

	/// our game world
	static karte_ptr_t welt;

public:
	/// description for elevated monorail (mandatory description)
	static const building_desc_t* elevated_foundation_desc;

	/**
	 * Finds a station building enabling pax/mail/goods for the AI.
	 * If @p time == 0 the timeline will be ignored.
	 * @param enables station enabled flags (see haltestelle_t::station_flags)
	 * @returns a random station that can be built above ground.
	 */
	static const building_desc_t* get_random_station(const building_desc_t::btype utype, const waytype_t wt, const uint16 time, const uint8 enables);

	/// Finds and returns the tile at position @p idx
	static const building_tile_desc_t* find_tile(const char* name, int idx);

	/// @returns the house description with name @p name
	static const building_desc_t* get_desc(const char *name);

	/**
	 * Registers the house description so the house can be built in-game.
	 * @returns true
	 */
	static bool register_desc(building_desc_t *desc);

	/// Sorts all house descriptions into their respective lists.
	static bool successfully_loaded();

	/**
	 * Fills menu with icons of buildings of a given waytype.
	 * This is needed for station extensions and headquarters.
 	 */
	static void fill_menu(tool_selector_t* tool_selector, building_desc_t::btype, waytype_t wt, sint16 sound_ok);

	/// @returns a random commercial building matching the requirements.
	static const building_desc_t* get_commercial(int level, uint16 time, climate c, uint32 clusters = 0l);

	/// @returns a random industrial building matching the requirements.
	static const building_desc_t* get_industrial(int level, uint16 time, climate cl, uint32 clusters = 0);

	/// @returns a random residential building matching the requirements.
	static const building_desc_t* get_residential(int level, uint16 time, climate cl, uint32 clusters = 0);

	/// @returns headquarters with level @p level (takes the first matching one)
	static const building_desc_t* get_headquarter(int level, uint16 time);

	/// @returns a random tourist attraction matching the requirements.
	static const building_desc_t* waehle_sehenswuerdigkeit(uint16 time, bool ignore_retire, climate cl)
	{
		return waehle_aus_liste(sehenswuerdigkeiten_land, time, ignore_retire, cl);
	}

	/// @returns a random unbuilt monument.
	static const building_desc_t* waehle_monument(uint16 time = 0)
	{
		return waehle_aus_liste(ungebaute_denkmaeler, time, false, MAX_CLIMATES);
	}

	/**
	 * Tells the house builder a new map is being loaded or generated.
	 * In this case the list of unbuilt monuments must be refilled
	 * to ensure each monument is only present once per map.
	 */
	static void new_world();

	/// @returns true if this monument has not yet been built.
	static bool is_valid_monument(const building_desc_t* desc) { return ungebaute_denkmaeler.is_contained(desc); }

	/// Tells the house builder a monument has been built.
	static void monument_gebaut(const building_desc_t* desc) { ungebaute_denkmaeler.remove(desc); }

	/// Called for a city attraction or a town hall with a certain number of inhabitants (bev).
	static const building_desc_t* get_special(uint32 bev, building_desc_t::btype utype, uint16 time, bool ignore_retire, climate cl);

	/**
	 * Removes an arbitrary building.
	 * It will also take care of factories and foundations.
	 * @param sp the player wanting to remove the building.
	 */
	static void remove(player_t *player, gebaeude_t *gb);

	/**
	 * Main function to build all non-traffic buildings, including factories.
	 * Building size can be larger than 1x1.
	 * Also the underlying ground will be changed to foundation.
	 * @param param if building a factory, pointer to the factory,
	 * 				if building a stop, pointer to the halt handle.
	 *
	 * @return The first built part of the building. Usually at @p pos, if this
	 *         building tile is not empty.
	 */
	static gebaeude_t* build(player_t* player, koord3d pos, int layout, const building_desc_t* desc, void* param = NULL);

	/**
	 * Build all kind of stops and depots. The building size must be 1x1.
	 * Stations with layout>4 may change the layout of neighbouring buildings. (->end of rail platforms)
	 * @param param if building a stop, pointer to the halt handle
	 */
	static gebaeude_t* neues_gebaeude(player_t* player, koord3d pos, int layout, const building_desc_t* desc, void* param = NULL);

	/// @returns house list of type @p typ
	static const vector_tpl<const building_desc_t *> *get_list(building_desc_t::btype typ);

	/// @returns city building list of type @p typ (res/com/ind)
	static const vector_tpl<const building_desc_t *> *get_citybuilding_list(building_desc_t::btype  typ);
};

#endif
