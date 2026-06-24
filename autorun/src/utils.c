/*
 * Copyright 2026 WrapEarnPass
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <geanyplugin.h>

#include "autorun.h"
#include "utils.h"

/* Read a filedef or keyfile for Intercept Actions
 * @param action key (should end with CM)
 * @param key_file to get remaining values from
 * @return true if parsing worked
 * 				false is parsing failed
 * */
gboolean parse_intercept_actions(gchar* key, GKeyFile* key_file, AUTORUN_CMD* cmd) {
	// key should be a (filetype_)action_number_flag string
	if (!key || !cmd) {
		return FALSE;
	}
	// is this a key?
	if (!g_strstr_len(key, -1, "_")) {
		// no
		cmd->invalid = TRUE;
		return FALSE;
	}
	cmd->invalid = FALSE;
	// what is this key?
	gchar** tokens;
	tokens = g_strsplit(key, "_", 4);

	switch (g_strv_length(tokens)) {
		// strv of 3 is filedef, 4 is project
	case 3: {
		// the caller has to set filetype
		if (g_strcmp0(tokens[0], "BS") == 0) {
			cmd->interceptor = g_strdup(tokens[0]);
		} else if (g_strcmp0(tokens[0], "OS") == 0) {
			cmd->interceptor = g_strdup(tokens[0]);
		} else {
			cmd->invalid = TRUE;
		}

		gint offset = atoi(tokens[1]);
		if (offset < 0 || offset > 99) {
			cmd->invalid = TRUE;
		} else {
			cmd->order = offset;
		}
		if (g_strcmp0(tokens[2], "CM") == 0) {
			cmd->command = utils_get_setting_string(key_file, "autorun", key, "");
			if (strlen(cmd->command) < 2) {
				// GKeyFile doesnt actually delete keys, but sets them to ""
				cmd->invalid = TRUE;
				break;
			}
			gchar* next_key;
			next_key = g_strconcat(tokens[0], "_", tokens[1], "_", "WD", NULL);
			cmd->working_dir = utils_get_setting_string(key_file, "autorun", next_key, "");
			g_free(next_key);
		} else {
			cmd->invalid = TRUE;
		}
		break;
	}
	case 4: {
		cmd->file_type = filetypes_lookup_by_name(tokens[0]);

		if (g_strcmp0(tokens[1], "BS") == 0) {
			cmd->interceptor = g_strdup(tokens[1]);
		} else if (g_strcmp0(tokens[1], "OS") == 0) {
			cmd->interceptor = g_strdup(tokens[1]);
		} else {
			cmd->invalid = TRUE;
		}

		gint offset = atoi(tokens[2]);
		if (offset < 0 || offset > 99) {
			cmd->invalid = TRUE;
		} else {
			cmd->order = offset;
		}

		if (g_strcmp0(tokens[3], "CM") == 0) {
			cmd->command = utils_get_setting_string(key_file, "autorun", key, "");
			gchar* next_key;
			next_key = g_strconcat(tokens[0], "_", tokens[1], "_", tokens[2], "_", "WD", NULL);
			cmd->working_dir = utils_get_setting_string(key_file, "autorun", next_key, "");
			g_free(next_key);
		} else {
			cmd->invalid = TRUE;
		}
		break;
	}
	default: {
		cmd->invalid = TRUE;
	}
	}

	g_strfreev(tokens);

	return cmd->invalid;
}
