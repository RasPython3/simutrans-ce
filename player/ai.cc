/*
 * Copyright (c) 2008 Markus Pristovsek
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/* Helper routines for AIs */

#include "ai.h"

#include "../simhalt.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simware.h"

#include "../bauer/brueckenbauer.h"
#include "../bauer/hausbauer.h"
#include "../bauer/tunnelbauer.h"
#include "../bauer/vehikelbauer.h"
#include "../bauer/wegbauer.h"

#include "../besch/haus_besch.h"

#include "../dings/zeiger.h"

#include "../vehicle/simvehikel.h"


/* The flesh for the place with road for headqurter searcher ... */
bool ai_bauplatz_mit_strasse_sucher_t::strasse_bei(sint16 x, sint16 y) const {
	grund_t *bd = welt->lookup_kartenboden( koord(x,y) );
	return bd && bd->hat_weg(road_wt);
}

bool ai_bauplatz_mit_strasse_sucher_t::ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const {
	if(bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
		// check to not built on a road
		int i, j;
		for(j=pos.x; j<pos.x+b; j++) {
			for(i=pos.y; i<pos.y+h; i++) {
				if(strasse_bei(j,i)) {
					return false;
				}
			}
		}
		// now check for road connection
		for(i = pos.y; i < pos.y + h; i++) {
			if(strasse_bei(pos.x - 1, i) ||  strasse_bei(pos.x + b, i)) {
				return true;
			}
		}
		for(i = pos.x; i < pos.x + b; i++) {
			if(strasse_bei(i, pos.y - 1) ||  strasse_bei(i, pos.y + h)) {
				return true;
			}
		}
	}
	return false;
}


/************************** and now the "real" helper functions ***************/


/* return true, if my bahnhof is here
 * @author prissi
 */
bool ai_t::is_my_halt(koord pos) const
{
	const halthandle_t halt = haltestelle_t::gib_halt(welt, pos);
	return
		halt.is_bound() && check_owner(this,halt->gib_besitzer());
}



/* returns true,
 * if there is already a connection
 * @author prissi
 */
bool ai_t::is_connected( const koord start_pos, const koord dest_pos, const ware_besch_t *wtyp ) const
{
	// Dario: Check if there's a stop near destination
	const planquadrat_t* start_plan = welt->lookup(start_pos);
	const halthandle_t* start_list = start_plan->get_haltlist();

	// Dario: Check if there's a stop near destination
	const planquadrat_t* dest_plan = welt->lookup(dest_pos);
	const halthandle_t* dest_list = dest_plan->get_haltlist();

	// suitable end search
	unsigned dest_count = 0;
	for (uint16 h = 0; h<dest_plan->get_haltlist_count(); h++) {
		halthandle_t halt = dest_list[h];
		if (halt->is_enabled(wtyp)) {
			for (uint16 hh = 0; hh<start_plan->get_haltlist_count(); hh++) {
				if (halt == start_list[hh]) {
					// connected with the start (i.e. too close)
					return true;
				}
			}
			dest_count ++;
		}
	}

	if(dest_count==0) {
		return false;
	}

	// now try to find a route
	// ok, they are not in walking distance
	ware_t ware(wtyp);
	ware.setze_zielpos(dest_pos);
	ware.menge = 1;
	for (uint16 hh = 0; hh<start_plan->get_haltlist_count(); hh++) {
		start_list[hh]->suche_route(ware, NULL);
		if (ware.gib_ziel().is_bound()) {
			// ok, already connected
			return true;
		}
	}

	// no connection possible between those
	return false;
}



// prepares a general tool just like a player work do
bool ai_t::init_general_tool( int tool, const char *param )
{
	const char *old_param = werkzeug_t::general_tool[tool]->default_param;
	werkzeug_t::general_tool[tool]->default_param = param;
	bool ok = werkzeug_t::general_tool[tool]->init( welt, this );
	werkzeug_t::general_tool[tool]->default_param = old_param;
	return ok;
}



// calls a general tool just like a player work do
bool ai_t::call_general_tool( int tool, koord k, const char *param )
{
	grund_t *gr = welt->lookup_kartenboden(k);
	koord3d pos = gr ? gr->gib_pos() : koord3d::invalid;
	const char *old_param = werkzeug_t::general_tool[tool]->default_param;
	werkzeug_t::general_tool[tool]->default_param = param;
	const char * err = werkzeug_t::general_tool[tool]->work( welt, this, pos );
	if(err) {
		if(*err) {
			dbg->message("ai_t::call_general_tool()","failed for tool %i at (%s) because of \"%s\"", tool, pos.gib_str(), err );
		}
		else {
			dbg->message("ai_t::call_general_tool()","not succesful for tool %i at (%s)", tool, pos.gib_str() );
		}
	}
	werkzeug_t::general_tool[tool]->default_param = old_param;
	return err==0;
}



/* returns ok, of there is a suitable space found
 * only check into two directions, the ones given by dir
 */
bool ai_t::suche_platz(koord pos, koord &size, koord *dirs) const
{
	sint16 length = abs( size.x + size.y );

	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr==NULL) {
		return false;
	}

	sint16 start_z = gr->gib_hoehe();
	int max_dir = length==0 ? 1 : 2;
	// two rotations
	for(  int dir=0;  dir<max_dir;  dir++  ) {
		for( sint16 i=0;  i<=length;  i++  ) {
			grund_t *gr = welt->lookup_kartenboden(  pos + (dirs[dir]*i)  );
			if(gr==NULL  ||  gr->gib_halt().is_bound()  ||  !welt->can_ebne_planquadrat(pos,start_z)  ||  !gr->ist_natur()  ||  gr->kann_alle_obj_entfernen(this)!=NULL  ||  gr->gib_hoehe()<welt->gib_grundwasser()) {
				return false;
			}
		}
		// aparently we can built this rotation here
		size = dirs[dir]*length;
		return true;
	}
	return false;
}



/* needed renovation due to different sized factories
 * also try "nicest" place first
 * @author HJ & prissi
 */
bool ai_t::suche_platz(koord &start, koord &size, koord target, koord off)
{
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord	platz;
	int cov = welt->gib_einstellungen()->gib_station_coverage();
	int xpos = start.x;
	int ypos = start.y;

	koord dir[2];
	if(  abs(start.x-target.x)<abs(start.y-target.y)  ) {
		dir[0] = koord( 0, sgn(target.y-start.y) );
		dir[1] = koord( sgn(target.x-start.x), 0 );
	}
	else {
		dir[0] = koord( sgn(target.x-start.x), 0 );
		dir[1] = koord( 0, sgn(target.y-start.y) );
	}

	DBG_MESSAGE("ai_t::suche_platz()","at (%i,%i) for size (%i,%i)",xpos,ypos,off.x,off.y);
	int maxy = min( welt->gib_groesse_y(), ypos + off.y + cov );
	int maxx = min( welt->gib_groesse_x(), xpos + off.x + cov );
	for (int y = max(0,ypos-cov);  y < maxy;  y++) {
		for (int x = max(0,xpos-cov);  x < maxx;  x++) {
			platz = koord(x,y);
			// no water tiles
			if(  welt->lookup_kartenboden(platz)->gib_hoehe() <= welt->gib_grundwasser()  ) {
				continue;
			}
			// thus now check them
			int current_dist = abs_distance(platz,target);
			if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
				// we will take the shortest route found
				start = platz;
				dist = abs_distance(platz,target);
			}
			else {
				koord test(x,y);
				if(is_my_halt(test)) {
DBG_MESSAGE("ai_t::suche_platz()","Search around stop at (%i,%i)",x,y);

					// we are on a station that belongs to us
					int xneu=x-1, yneu=y-1;
					platz = koord(xneu,y);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					// search on the other side of the station
					xneu = x+1;
					yneu = y+1;
					platz = koord(xneu,y);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}

					platz = koord(x,yneu);
					current_dist = abs_distance(platz,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = platz;
						dist = current_dist;
					}
				}
			}
		}
	}
	// of, success of short than maximum
	if(  dist<0x7FFFFFFF  ) {
		return true;
	}
	return false;
}



void ai_t::clean_marker( koord place, koord size )
{
	koord pos;
	if(size.y<0) {
		place.y += size.y;
		size.y = -size.y;
	}
	if(size.x<0) {
		place.x += size.x;
		size.x = -size.x;
	}
	for(  pos.y=place.y;  pos.y<=place.y+size.y;  pos.y++  ) {
		for(  pos.x=place.x;  pos.x<=place.x+size.x;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			zeiger_t *z = gr->find<zeiger_t>();
			if(z) {
				delete z;
			}
		}
	}
}



void ai_t::set_marker( koord place, koord size )
{
	koord pos;
	if(size.y<0) {
		place.y += size.y;
		size.y = -size.y;
	}
	if(size.x<0) {
		place.x += size.x;
		size.x = -size.x;
	}
	for(  pos.y=place.y;  pos.y<=place.y+size.y;  pos.y++  ) {
		for(  pos.x=place.x;  pos.x<=place.x+size.x;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			zeiger_t *z = new zeiger_t(welt, gr->gib_pos(), this);
			z->setze_bild( skinverwaltung_t::belegtzeiger->gib_bild_nr(0) );
			gr->obj_add( z );
		}
	}
}



/* builts a headquarter or updates one */
bool ai_t::built_update_headquarter()
{
	// find next level
	const haus_besch_t* besch = NULL;
	for(  vector_tpl<const haus_besch_t *>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end();  iter != end;  ++iter  ) {
		if ((*iter)->gib_extra() == get_headquarter_level()) {
			besch = (*iter);
			break;
		}
	}
	// is the a suitable one?
	if(besch!=NULL) {
		// cost is negative!
		sint64 cost = welt->gib_einstellungen()->cst_multiply_headquarter*besch->gib_level()*besch->gib_b()*besch->gib_h();
		if(  konto+cost > welt->gib_einstellungen()->gib_starting_money()  ) {
			// and enough money left ...
			koord place = get_headquarter_pos();
			if(place!=koord::invalid) {
				// remove old hq
				grund_t *gr = welt->lookup_kartenboden(place);
				gebaeude_t *prev_hq = gr->find<gebaeude_t>();
				// other size?
				if(  besch->gib_groesse()!=prev_hq->gib_tile()->gib_besch()->gib_groesse()  ) {
//					hausbauer_t::remove( welt, this, prev_hq );
					// needs new place
					place = koord::invalid;
				}
			}
			// needs new place?
			if(place==koord::invalid  &&  !halt_list.empty()) {
				stadt_t *st = welt->suche_naechste_stadt(halt_list.front()->gib_basis_pos());
				if(st) {
					bool is_rotate=besch->gib_all_layouts()>1;
					place = ai_bauplatz_mit_strasse_sucher_t(welt).suche_platz(st->gib_pos(), besch->gib_b(), besch->gib_h(), besch->get_allowed_climate_bits(), &is_rotate);
				}
			}
			const char *err=NULL;
			if(  place!=koord::invalid  &&  (err=werkzeug_t::general_tool[WKZ_HEADQUARTER]->work( welt, this, welt->lookup_kartenboden(place)->gib_pos() ))!=NULL  ) {
				// tell the player
				char buf[256];
				sprintf(buf, translator::translate("%s s\nheadquarter now\nat (%i,%i)."), gib_name(), place.x, place.y );
				welt->get_message()->add_message(buf, place, message_t::ai, PLAYER_FLAG|player_nr, welt->lookup_kartenboden(place)->find<gebaeude_t>()->gib_tile()->gib_hintergrund(0,0,0) );
			}
			else {
				if(  place==koord::invalid  ) {
					add_headquarter( 0, koord::invalid );
				}
				dbg->warning( "ai_t::built_update_headquarter()", "HQ failed with : %s", translator::translate(err) );
			}
			return place != koord::invalid;
		}

	}
	return false;
}



/**
 * Find the last water tile using line algorithm von Hajo
 * start MUST be on land!
 **/
koord ai_t::find_shore(koord start, koord end) const
{
	int x = start.x;
	int y = start.y;
	int xx = end.x;
	int yy = end.y;

	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	koord last = start;
	for (i = 0; i <= steps; i++) {
		koord next(xp>>16,yp>>16);
		if(next!=last) {
			if(!welt->lookup_kartenboden(next)->ist_wasser()) {
				last = next;
			}
		}
		xp += xs;
		yp += ys;
	}
	// should always find something, since it ends in water ...
	return last;
}



bool ai_t::find_harbour(koord &start, koord &size, koord target)
{
	koord shore = find_shore(target,start);
	// distance of last found point
	int dist=0x7FFFFFFF;
	koord k;
	// now find a nice shore next to here
	for(  k.y=max(1,shore.y-5);  k.y<shore.y+6  &&  k.y<welt->gib_groesse_y()-2; k.y++  ) {
		for(  k.x=max(1,shore.x-5);  k.x<shore.x+6  &&  k.y<welt->gib_groesse_x()-2; k.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(k);
			if(gr  &&  gr->gib_grund_hang()!=0  &&  hang_t::ist_wegbar(gr->gib_grund_hang())  &&  gr->ist_natur()  &&  gr->gib_hoehe()==welt->gib_grundwasser()  &&  !gr->is_halt()) {
				koord zv = koord(gr->gib_grund_hang());
				if(welt->lookup_kartenboden(k-zv)->gib_weg_ribi(water_wt)) {
					// next place is also water
					koord dir[2] = { zv, koord(zv.y,zv.x) };
					koord platz = k+zv;
					int current_dist = abs_distance(k,target);
					if(  current_dist<dist  &&  suche_platz(platz,size,dir)  ){
						// we will take the shortest route found
						start = k;
						dist = current_dist;
					}
				}
			}
		}
	}
	return (dist<0x7FFFFFFF);
}



bool ai_t::create_simple_road_transport(koord platz1, koord size1, koord platz2, koord size2, const weg_besch_t *road_weg )
{
	// sanity check here
	if(road_weg==NULL) {
		DBG_MESSAGE("ai_t::create_simple_road_transport()","called without valid way.");
		return false;
	}

	// remove pointer
	clean_marker(platz1,size1);
	clean_marker(platz2,size2);

	if(!(welt->ebne_planquadrat( this, platz1, welt->lookup_kartenboden(platz1)->gib_hoehe() )  &&  welt->ebne_planquadrat( this, platz2, welt->lookup_kartenboden(platz2)->gib_hoehe() ))  ) {
		// no flat land here?!?
		return false;
	}

	INT_CHECK( "simplay 1742" );

	// is there already a connection?
	// get a default vehikel
	vehikel_besch_t test_besch(road_wt, 25, vehikel_besch_t::diesel );
	vehikel_t* test_driver = vehikelbauer_t::baue(welt->lookup_kartenboden(platz1)->gib_pos(), this, NULL, &test_besch);
	route_t verbindung;
	if (verbindung.calc_route(welt, welt->lookup_kartenboden(platz1)->gib_pos(), welt->lookup_kartenboden(platz2)->gib_pos(), test_driver, 0)  &&
		verbindung.gib_max_n()<2u*abs_distance(platz1,platz2))  {
DBG_MESSAGE("ai_passenger_t::create_simple_road_transport()","Already connection between %d,%d to %d,%d is only %i",platz1.x, platz1.y, platz2.x, platz2.y, verbindung.gib_max_n() );
		// found something with the nearly same lenght
		delete test_driver;
		return true;
	}
	delete test_driver;

	// no connection => built one!
	wegbauer_t bauigel(welt, this);
	bauigel.route_fuer( wegbauer_t::strasse, road_weg, tunnelbauer_t::find_tunnel(road_wt,road_weg->gib_topspeed(),welt->get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,road_weg->gib_topspeed(),welt->get_timeline_year_month()) );

	// we won't destroy cities (and save the money)
	bauigel.set_keep_existing_faster_ways(true);
	bauigel.set_keep_city_roads(true);
	bauigel.set_maximum(10000);

	INT_CHECK("simplay 846");

	bauigel.calc_route(welt->lookup_kartenboden(platz1)->gib_pos(),welt->lookup_kartenboden(platz2)->gib_pos());
	if(bauigel.max_n > 1) {
DBG_MESSAGE("ai_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d",platz1.x, platz1.y, platz2.x, platz2.y);
		bauigel.baue();
		return true;
	}
	// beware: The stop position might have changes!
DBG_MESSAGE("ai_t::create_simple_road_transport()","building simple road from %d,%d to %d,%d failed",platz1.x, platz1.y, platz2.x, platz2.y);
	return false;
}



