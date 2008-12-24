/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "mud/color.h"
#include "mud/settings.h"
#include "net/telnet.h"

/* BEGIN COMMAND
 *
 * name: set color
 * usage: set color <type> <color>
 *
 * format: set color :0% :1% (60)
 *
 * END COMMAND */

void command_set_color (Player* player, std::string argv[])
{
	// must have a telnet connection
	if (player->get_conn() == NULL) {
		*player << "Must have an active telnet connection.\n";
		return;
	}

	// get type/color
	int ctype, cvalue;
	for (ctype = 0; !color_type_names[ctype].empty(); ++ctype)
		if (str_eq(color_type_names[ctype], argv[0]))
			break;
	for (cvalue = 0; !color_value_names[cvalue].empty(); ++cvalue)
		if (str_eq(color_value_names[cvalue], argv[1]))
			break;

	// invalid color type
	if (color_type_names[ctype].empty()) {
		// determine column count
		int max_col = (player->get_width() - 3) / 20; // figure max size
		if (max_col <= 0)
			max_col = 3;

		// print list
		*player << "Invalid type '" << argv[0] << "'.  Possible options are:\n";
		int col = 0;
		for (uint i = 0; !color_type_names[i].empty(); ++i) {
			// display
			player->set_indent(col * 20 + 2);
			*player << color_type_names[i];

			// move column
			if (++col >= max_col) {
				*player << "\n";
				col = 0;
			}
		}
		if (col)
			*player << "\n";
		player->set_indent(0);

	// invalid color
	} else if (color_value_names[cvalue].empty()) {
		// determine column count
		int max_col = (player->get_width() - 3) / 20; // figure max size
		if (max_col <= 0)
			max_col = 3;

		// print list
		*player << "Invalid color '" << argv[1] << "'.  Possible options are:\n";
		int col = 0;
		for (uint i = 0; !color_value_names[i].empty(); ++i) {
			// display
			player->set_indent(col * 20 + 2);
			*player << color_value_names[i] << " (" << color_values[i] << "ABC" << CNORMAL << ")";

			// move column
			if (++col >= max_col) {
				*player << "\n";
				col = 0;
			}
		}
		if (col)
			*player << "\n";
		player->set_indent(0);

	// set color
	} else {
		player->get_conn()->pconn_set_color (ctype, cvalue);
		*player << "Color '" << color_type_names [ctype]<< "' set to " << color_values[cvalue] << color_value_names[cvalue] << CNORMAL "\n";
	}
}
