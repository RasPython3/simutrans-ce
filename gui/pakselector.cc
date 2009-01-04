
#include <string.h>
#include <stdio.h>

#include "../simdebug.h"
#include "pakselector.h"
#include "../dataobj/umgebung.h"


/**
 * what to do after loading
 */
void pakselector_t::action(const char *filename)
{
	umgebung_t::objfilename = (cstring_t)filename + "/";
}


void pakselector_t::del_action(const char *filename)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = (cstring_t)filename + "/";
}

const char *pakselector_t::get_info(const char *)
{
	return "";
}


void pakselector_t::zeichnen(koord p, koord gr)
{
	gui_frame_t::zeichnen( p, gr );
	display_multiline_text( p.x+10, p.y+10,
		"You have multiple pak sets to choose from.\n", COL_BLACK );

	display_multiline_text( p.x+10, p.y+gr.y-4-(LINESPACE*3),
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line", COL_BLACK );
}


bool pakselector_t::check_file( const char *filename, const char * )
{
	char buf[1024];
	sprintf( buf, "%s/ground.Outside.pak", filename );
	FILE *f=fopen( buf, "r" );
	if(f) {
		fclose(f);
	}
	// found only one?
	if(f!=NULL) {
		if(entries.count()==0) {
			umgebung_t::objfilename = (cstring_t)filename + "/";
		}
		else if(  !umgebung_t::objfilename.empty()  ) {
			umgebung_t::objfilename = "";
		}
	}
	return f!=NULL;
}


pakselector_t::pakselector_t() : savegame_frame_t( NULL, umgebung_t::program_dir )
{
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	remove_komponente( &divider1 );
	fnlabel.set_text( "Choose one graphics set for playing:" );
}
