/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Object which shows the label that indicates that the ground is owned by somebody
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../simplay.h"

#include "../besch/grund_besch.h"

#include "../dataobj/umgebung.h"

#include "label.h"

label_t::label_t(karte_t *welt, loadsave_t *file) :
	ding_t(welt)
{
	rdwr(file);
	welt->add_label(gib_pos().gib_2d());
}


label_t::label_t(karte_t *welt, koord3d pos, spieler_t *sp, const char *text) :
	ding_t(welt, pos)
{
	setze_besitzer( sp );
	welt->add_label(pos.gib_2d());
	grund_t *gr=welt->lookup_kartenboden(pos.gib_2d());
	if(gr) {
		ding_t *d=gr->obj_bei(0);
		if(d  &&  d->gib_besitzer()==NULL) {
			d->setze_besitzer(sp);
		}
		if (text) {
			gr->setze_text(text);
		}
		spieler_t::accounting(sp, umgebung_t::cst_buy_land, pos.gib_2d(), COST_CONSTRUCTION);
	}
}



label_t::~label_t()
{
	koord k = gib_pos().gib_2d();
	welt->remove_label(k);
	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {
		gr->setze_text(NULL);
	}
}



image_id label_t::gib_bild() const
{
	grund_t *gr=welt->lookup(gib_pos());
	return (gr  &&  gr->obj_bei(0)==this) ? skinverwaltung_t::belegtzeiger->gib_bild_nr(0) : IMG_LEER;
}
