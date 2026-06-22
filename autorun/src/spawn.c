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
#include <glib/gstdio.h>

#include "autorun.h"
#include "spawn.h"
#include "utils.h"

void dispatch_run(const gchar* interceptor, GeanyDocument* doc) {
	g_message("dispatch_run %%f %s", doc->file_name);
	g_message("dispatch_run %%d %s", doc->real_path);

	GSList* command_list = NULL;
	g_message("target interceptor %s", interceptor);

	// build the commands backwads
	GSList* elem = NULL;
	foreach_slist(elem, autorun_globals->filedef_commands) {
		AUTORUN_CMD* cmd = ((AUTORUN_CMD*)elem->data);
		if (!cmd->invalid && g_strcmp0(cmd->interceptor, interceptor) == 0 && cmd->file_type == doc->file_type) {
			command_list = g_slist_prepend(command_list, cmd);
		}
	}

	foreach_slist(elem, autorun_globals->project_commands) {
		AUTORUN_CMD* cmd = ((AUTORUN_CMD*)elem->data);
		if (!cmd->invalid && g_strcmp0(cmd->interceptor, interceptor) == 0 && cmd->file_type == doc->file_type) {
			command_list = g_slist_prepend(command_list, cmd);
		}
	}
	// now flip it on them.
	command_list = g_slist_reverse(command_list);
	g_message("command list in %s %i", interceptor, g_slist_length(command_list));
	if (g_slist_length(command_list) > 0) {
		// now if there are any cmds in scope, honor order
		GSList* meh[g_slist_length(command_list)] = {};
		foreach_slist(elem, command_list) {
			AUTORUN_CMD* cmd = ((AUTORUN_CMD*)elem->data);
			g_message("Staged %s in %s # %i", cmd->command, cmd->working_dir, cmd->order);
			meh[cmd->order] = elem;
		}

		for (uint i = 0; i < g_slist_length(command_list); i++) {
			if (meh[i] == NULL) {
				break; // stop on the first null;
			}
			AUTORUN_CMD* cmd = (AUTORUN_CMD*)meh[i]->data;

			// prep replacements.
			gchar* target_dir;
			if (doc->real_path != NULL) {
				target_dir = g_path_get_dirname(doc->real_path);
			} else {
				// where else but here?
				target_dir = g_strdup(".");
			}
			gchar* target_projdir;
			if (autorun_globals->data->app->project != NULL && autorun_globals->data->app->project->base_path != NULL) {
				target_projdir = utils_get_locale_from_utf8(autorun_globals->data->app->project->base_path);
			} else {
				// fold %p to %d if no project.
				target_projdir = g_strdup(target_dir);
			}
			gchar* target_file = NULL;
			GFile* tmpfile = NULL;
			GFileIOStream* iostream = NULL;
			gboolean success = FALSE;
			gchar* before_contents;
			// we need scintilla in a buffer regardless of %a
			gint con_len = sci_get_length(doc->editor->sci);
			g_message("SCI is %i long", con_len);
			before_contents = sci_get_contents(doc->editor->sci, con_len);

			if (g_strcmp0(interceptor, "BS") == 0 && (g_strrstr(cmd->command, "%a") || g_strrstr(cmd->working_dir, "%a"))) {
				//%a means something special
				tmpfile = g_file_new_tmp("ar.aXXXXXX", &iostream, NULL);
				if (tmpfile != NULL) {
					target_file = g_file_get_path(tmpfile);
					g_message("dumping SCI to %s", target_file);
					success = g_file_replace_contents(tmpfile, before_contents, con_len, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL, NULL);
				} else {
					success = FALSE;
				}
			} else {
				//%a just means file.
				success = TRUE;
				target_file = utils_get_locale_from_utf8(doc->file_name);
			}

			// do the replacement bits
			gchar* target_name = g_path_get_basename(target_file);
			// replace %f
			GString* command = g_string_new(cmd->command);
			g_string_replace(command, "%f", target_name, 0);
			GString* working_dir = g_string_new(cmd->working_dir);
			g_string_replace(working_dir, "%f", target_name, 0);

			// replace %a
			g_string_replace(command, "%a", target_file, 0);
			g_string_replace(working_dir, "%a", target_file, 0);

			// replace %d
			g_string_replace(command, "%d", target_dir, 0);
			g_string_replace(working_dir, "%d", target_dir, 0);

			// replace %p
			g_string_replace(command, "%p", target_projdir, 0);
			g_string_replace(working_dir, "%p", target_projdir, 0);

			// it looks like we're running;
			SpawnWriteData* stdin_data;
			GString* stdout_data = g_string_new(NULL);
			GString* stderr_data = g_string_new(NULL);
			if (success) {
				g_message("Run %s", command->str);
				g_message(" in %s # %i", working_dir->str, cmd->order);

				if (g_strcmp0(interceptor, "BS") == 0 && success && tmpfile == NULL) {
					// need to send stdin.
					g_message("Send to stdin");
					stdin_data = g_new0(SpawnWriteData, 1);
					stdin_data->ptr = before_contents;
					stdin_data->size = con_len;
				} else {
					g_message("Read from file");
					// no stdin
					stdin_data = NULL;
				}
				gchar** env;
				env = utils_copy_environment(NULL, "GEANY_FUNCNAME", __func__, NULL);
				GError* error = NULL;
				success = spawn_sync(working_dir->str, command->str, NULL, env, stdin_data, stdout_data, stderr_data, NULL, &error);
				g_strfreev(env);
				if (!success) {
					g_message("!success");
					// somewhat bad had occur
					msgwin_status_add("Command failed with %s", error->message);
					ui_set_statusbar(FALSE, "Auto-run %s interceptor failed.", interceptor);
					g_error_free(error);
				} else {
					g_message("success");
					// if we made a tmpfile, grab the results and del the file.
					if (tmpfile != NULL) {
						gchar* read = NULL;
						g_file_get_contents(target_file, &read, NULL, NULL);
						if (read != NULL) {
							g_message("read was goodish");
							if (g_strcmp0(read, before_contents) != 0) {
								// no point in stealing the document focus
								sci_set_text(doc->editor->sci, read);
							}
							g_free(read);
						} else {
							g_message("read was nullish");
						}
					} else if (g_strcmp0(interceptor, "BS") == 0) {
						// update scintilla from the spawn.
						sci_set_text(doc->editor->sci, stdout_data->str);
						msgwin_compiler_add_string(COLOR_BLACK, stderr_data->str);
					}
				}

				if (tmpfile != NULL) {
					g_unlink(target_file);
				}

				g_string_free(stdout_data, TRUE);
				g_string_free(stderr_data, TRUE);

				if (stdin_data != NULL) {
					g_free(stdin_data);
				}

				if (before_contents != NULL) {
					g_free(before_contents);
				}

				g_string_free(working_dir, TRUE);
				g_string_free(command, TRUE);
				g_free(target_name);
				g_free(target_dir);
				g_free(target_projdir);
				if (target_file != NULL) {
					g_free(target_file);
				}
			}
		}
	}
	g_slist_free(command_list);
}
