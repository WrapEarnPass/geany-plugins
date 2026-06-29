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
//
// if (spawn_async(working_dir, locale_term_cmd, NULL, NULL, &(run_info[cmdindex].pid), &error)) {
// g_child_watch_add(run_info[cmdindex].pid, (GChildWatchFunc)run_exit_cb, (gpointer) & (run_info[cmdindex]));
// build_menu_update(doc);
//}

/*
 * Build the @interceptor from @autorun_globals, and return the parsed command to @command
 * If the parsed command creates a tempfile, return it in cmd->interceptor
 * */

typedef struct {
	GString* stdout;
	GString* stderr;
	GeanyDocument* doc;

} AUTORUN_ASYNC_DATA;
static void parse_command(const gchar* interceptor, GeanyDocument* doc, GSList** command) {
	GSList* command_list = NULL;

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
	if (g_slist_length(command_list) > 0) {
		// now if there are any cmds in scope, honor order
		GSList* command_order[g_slist_length(command_list)] = {};
		foreach_slist(elem, command_list) {
			AUTORUN_CMD* cmd = ((AUTORUN_CMD*)elem->data);
			command_order[cmd->order] = elem;
		}

		for (uint i = 0; i < g_slist_length(command_list); i++) {
			if (!command_order[i]) {
				break; // stop on the first null;
			}
			AUTORUN_CMD* cmd = (AUTORUN_CMD*)command_order[i]->data;

			// prep replacements.
			gchar* target_dir;
			if (doc->real_path) {
				target_dir = g_path_get_dirname(doc->real_path);
			} else {
				// where else but here?
				target_dir = g_strdup(".");
			}
			gchar* target_projdir;
			if (autorun_globals->data->app->project && autorun_globals->data->app->project->base_path) {
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

			if (g_strcmp0(interceptor, "BS") == 0 && g_strrstr(cmd->command, "%a")) {
				//%a means something special
				tmpfile = g_file_new_tmp("ar.aXXXXXX", &iostream, NULL);
				if (tmpfile) {
					// dont need the iostream, but gtk wont make a temp without it.
					g_io_stream_close((GIOStream*)iostream, NULL, NULL);
					target_file = g_file_get_path(tmpfile);
					// remove the trailing \0 on write
					gint con_len = sci_get_length(doc->editor->sci);
					before_contents = sci_get_contents(doc->editor->sci, con_len);
					success = g_file_set_contents_full(target_file, before_contents, con_len - 1, G_FILE_SET_CONTENTS_CONSISTENT, 0660, NULL);
					g_free(before_contents);

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

			GString* command_str = g_string_new(cmd->command);
			// replace %f
			g_string_replace(command_str, "%f", target_name, 0);
			// replace %a
			g_string_replace(command_str, "%a", target_file, 0);

			GString* working_dir_str = g_string_new(cmd->working_dir);
			// replace %d
			g_string_replace(command_str, "%d", target_dir, 0);
			g_string_replace(working_dir_str, "%d", target_dir, 0);

			// replace %p
			g_string_replace(command_str, "%p", target_projdir, 0);
			g_string_replace(working_dir_str, "%p", target_projdir, 0);

			if (success) {
				success = spawn_check_command(command_str->str, TRUE, NULL);
			}
			if (success) {
				// attach this command to the outgoing GSList
				AUTORUN_CMD* cmdout = autorun_cmd_new();
				cmdout->command = g_string_free_and_steal(command_str);
				cmdout->working_dir = g_string_free_and_steal(working_dir_str);
				if (tmpfile) {
					cmdout->interceptor = g_strdup(target_file);
				} else {
					cmdout->interceptor = NULL;
				}
				cmdout->invalid = FALSE;
				*command = g_slist_append(*command, cmdout);
			} else {
				// cant assemble this command, don't try again
				cmd->invalid = TRUE;
				msgwin_status_add(_("Cannot parse command '%s'"), command_str->str);
			}
			// cleanup
			g_free(target_dir);
			g_free(target_projdir);
			if (tmpfile) {
				g_object_unref(tmpfile);
			}
			g_free(target_file);
		}
		// cleanup
		// autorun_cmd_list_free(command_list);
		// we dont own the contents of command_list.
		g_slist_free(command_list);
	}
}

static void stdio_cb(GString* string, GIOCondition condition, gpointer data) {
	// dup the string to data.
	if (condition == G_IO_IN || condition == G_IO_PRI) {
		g_string_append((GString*)data, string->str);
	}
}

static void stdioend_cb(GPid pid, gint wait_status, gpointer user_data) {
	AUTORUN_ASYNC_DATA* exit_data = (AUTORUN_ASYNC_DATA*)user_data;
	parse_output(exit_data->stdout->str);
	g_string_free(exit_data->stdout, TRUE);
	parse_output(exit_data->stderr->str);
	g_string_free(exit_data->stderr, TRUE);
	if (--autorun_globals->children < 1) {
		ui_progress_bar_stop();
		ui_set_statusbar(FALSE, _("Auto-run finished %s"), exit_data->doc->file_name);
		autorun_globals->children = 0;
	}
	g_free(exit_data);
}

// prefer running commands async, so the editor plays nice
void dispatch_run_async(GeanyDocument* doc) {
	GSList* runnables = NULL;
	parse_command("OS", doc, &runnables);
	GSList* runnable;
	ui_progress_bar_start("Auto-run");
	foreach_slist(runnable, runnables) {
		// convenience
		AUTORUN_CMD* current_cmd = (AUTORUN_CMD*)runnable->data;

		gchar** env;
		env = utils_copy_environment(NULL, "GEANY_FUNCNAME", __func__, NULL);
		GError* error = NULL;

		gchar* status_msg = g_strdup_printf(_("Running %s (from %s)"), current_cmd->command, current_cmd->working_dir);
		msgwin_compiler_add_string(COLOR_BLUE, status_msg);
		g_free(status_msg);
		GString* stdout_data = g_string_new(NULL);
		GString* stderr_data = g_string_new(NULL);
		AUTORUN_ASYNC_DATA* end_data = g_new0(AUTORUN_ASYNC_DATA, 1);
		end_data->stdout = stdout_data;
		end_data->stderr = stderr_data;
		end_data->doc = doc;
		GPid* child_pid = NULL;
		gboolean success = FALSE;
		ui_set_statusbar(FALSE, _("Auto-run running %s"), current_cmd->command);

		++autorun_globals->children;
		success = spawn_with_callbacks(current_cmd->working_dir, current_cmd->command, NULL, env, SPAWN_ASYNC | SPAWN_LINE_BUFFERED, NULL, NULL, stdio_cb, stdout_data, 0,
																	 stdio_cb, stderr_data, 0, stdioend_cb, end_data, child_pid, &error);

		// cleanup anything we dont need in processing.
		g_strfreev(env);

		if (!success) {
			if (--autorun_globals->children < 1) {
				ui_progress_bar_stop();
				autorun_globals->children = 0;
			}
			//  refused to run
			msgwin_status_add(_("Command failed with %s"), error->message);
			g_error_free(error);
		}
	}
	autorun_cmd_list_free(runnables);
}

// this should only ever be for interceptor=BS, as it locks up the editor
void dispatch_run_sync(GeanyDocument* doc) {
	GSList* runnables = NULL;
	parse_command("BS", doc, &runnables);
	GSList* runnable;
	foreach_slist(runnable, runnables) {
		// convienience
		AUTORUN_CMD* current_cmd = (AUTORUN_CMD*)runnable->data;
		// pull the tempfile from the overloaded cmd->interceptor
		gboolean has_tmpfile = (current_cmd->interceptor) ? TRUE : FALSE;
		gchar* tmpfile_path = current_cmd->interceptor;

		gchar** env;
		env = utils_copy_environment(NULL, "GEANY_FUNCNAME", __func__, NULL);
		gint ret;
		GError* error = NULL;
		GString* stdout_data = g_string_new(NULL);
		GString* stderr_data = g_string_new(NULL);
		SpawnWriteData* stdin_data;

		gint con_len = sci_get_length(doc->editor->sci);
		gchar* before_contents = sci_get_contents(doc->editor->sci, con_len);
		if (!has_tmpfile) {
			// need to send stdin.
			stdin_data = g_new0(SpawnWriteData, 1);
			stdin_data->ptr = before_contents;
			stdin_data->size = con_len;
		} else {
			// no stdin
			stdin_data = NULL;
		}
		gchar* status_msg = g_strdup_printf(_("Running %s (from %s)"), current_cmd->command, current_cmd->working_dir);
		msgwin_compiler_add_string(COLOR_BLUE, status_msg);
		g_free(status_msg);
		// runnit
		gboolean success = spawn_sync(current_cmd->working_dir, current_cmd->command, NULL, env, stdin_data, stdout_data, stderr_data, &ret, &error);
		if (stdin_data) {
			g_free(stdin_data);
		}
		// cleanup anything we dont need in processing.
		g_strfreev(env);

		if (success && ret == 0) {
			// ran and didn't throw an error
			// do we need to update the editor?
			// maybe.
			// could be in tmpfile, stdout
			gchar* read_tmp = NULL;
			if (has_tmpfile) {
				g_file_get_contents(tmpfile_path, &read_tmp, NULL, NULL);
			}
			gint cursor_at = sci_get_current_position(doc->editor->sci);
			// check for the stupid case
			if (strlen(stdout_data->str) > 0 && has_tmpfile && g_strcmp0(read_tmp, before_contents) != 0 && g_strcmp0(stdout_data->str, read_tmp) != 0) {
				// if there are changes to the tempfile AND stdout
				// this command is stupid. there is no way to know
				// which output is the output to output.
				msgwin_status_add(_("Command failed with inconsistent output."));
			} else if (strlen(stdout_data->str) > 0) {
				sci_set_text(doc->editor->sci, stdout_data->str);
				// put ze cursor bek
				sci_set_current_position(doc->editor->sci, cursor_at, TRUE);
			} else if (has_tmpfile && g_strcmp0(read_tmp, before_contents) != 0) {
				sci_set_text(doc->editor->sci, read_tmp);
				// put ze cursor bek
				sci_set_current_position(doc->editor->sci, cursor_at, TRUE);
			}
			if (read_tmp) {
				g_free(read_tmp);
			}
			// dont update the editor if it ran with an error.
		} else if (!success) {
			// refused to run
			msgwin_status_add(_("Command failed with %s"), error->message);
			ui_set_statusbar(FALSE, _("Auto-run before-save interceptor failed."));
			g_error_free(error);
		}

		// GUI output formatting?
		// BS stdout goes to the scintilla editor
		// stderr
		if (strlen(stderr_data->str) > 0) {
			parse_output(stderr_data->str);
		}

		if (has_tmpfile) {
			g_unlink(tmpfile_path);
		}

		g_string_free(stdout_data, TRUE);
		g_string_free(stderr_data, TRUE);

		if (before_contents) {
			g_free(before_contents);
		}
	}
	autorun_cmd_list_free(runnables);
}
