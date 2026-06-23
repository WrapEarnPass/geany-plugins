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
#include <gtk/gtk.h>

#include "autorun.h"
#include "menu.h"
#include "spawn.h"

/* Handler to read any Project declared Auto-run configs */
static void on_project_open(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data) {
	if (!autorun_globals->project_commands) {
		load_projectdefs(config);
	}
}

// this handler is currently disconnected due to geany/geany#4603
/* Handler to read any Project declared Auto-run configs */
// static void on_project_save(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data){
//	load_projectdefs(config);
// }

/* Handler to clear any old Project declared Auto-run configs */
static void on_project_close(G_GNUC_UNUSED GObject* obj, G_GNUC_UNUSED gpointer user_data) {
	// unload project handlers
	autorun_cmd_list_free(autorun_globals->project_commands);
	autorun_globals->project_commands = NULL;
}

/* Handler to run any applicable Auto-run configs after a write*/
static void on_doc_save(G_GNUC_UNUSED GObject* obj, GeanyDocument* doc, G_GNUC_UNUSED gpointer user_data) {
	/* disabled because geany/geany#4604 never sets doc->changed
	if(!doc->changed)
	{
		return;
	}
	*/
	if (sci_get_length(doc->editor->sci) < 1) {
		// no point in processing a file so short
		return;
	}
	ui_progress_bar_start(NULL);
	/*
	 * dont reset the msgwin here, as document-save
	 * is the second stage of document-before-save
	 * and the messages will all be logically related
	 * to the user selecting File>Save
	 */
	dispatch_run("OS", doc);
	ui_progress_bar_stop();
}

/* Handler to run any applicable Auto-run configs before a write*/
static void on_doc_before_save(G_GNUC_UNUSED GObject* obj, GeanyDocument* doc, G_GNUC_UNUSED gpointer user_data) {
	if (!doc->changed) {
		return;
	}
	if (sci_get_length(doc->editor->sci) < 1) {
		// no point in processing a file so short
		return;
	}
	ui_progress_bar_start(NULL);
	// status is used for return codes, so dont clear that one
	// messages is used for stdout
	msgwin_clear_tab(MSG_MESSAGE);
	// compiler is used for stderr
	msgwin_clear_tab(MSG_COMPILER);
	dispatch_run("BS", doc);
	ui_progress_bar_stop();
}

// clang-format off
PluginCallback plugin_callbacks[] = {
	{ "project-open", (GCallback)&on_project_open, TRUE, NULL },
	//{ "project-save", (GCallback)&on_project_save, TRUE, NULL },// geany/geany#4603
	{ "project-close", (GCallback)&on_project_close, TRUE, NULL },
	{ "document-save", (GCallback)&on_doc_save, TRUE, NULL },
	{ "document-before-save", (GCallback)&on_doc_before_save, TRUE, NULL },
	{ NULL, NULL, FALSE, NULL }
};
// clang-format on

/* Bring up plugin and load filetypes.FILE that exist for Auto-run */
static gboolean autorun_init(GeanyPlugin* plugin, gpointer pdata) {
	if (!autorun_globals) {
		autorun_globals_init(plugin);
		load_filedefs();

		// if initialized while a project is already open, manually ingest the project
		if (autorun_globals->data->app && autorun_globals->data->app->project) {
			// force a GKeyFile
			GKeyFile* config = g_key_file_new();
			g_key_file_load_from_file(config, autorun_globals->data->app->project->file_name, G_KEY_FILE_NONE, NULL);
			load_projectdefs(config);
			g_free(config);
		}
	}
	menu_init();
	return TRUE;
}

/* ensure destruction of any Auto-run objects */
static void autorun_cleanup(GeanyPlugin* plugin, gpointer pdata) {
	if (autorun_globals) {
		autorun_globals_free();
	}
	menu_cleanup();
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin* plugin) {
	// who am we?
	plugin->info->name = "Auto-run";
	plugin->info->description = _("Geany action interceptor plugin");
	plugin->info->version = "0.1";
	plugin->info->author = "WrapEarnPass";

	// english do you speak?
	main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);

	// what am we do?
	plugin->funcs->init = autorun_init;
	plugin->funcs->cleanup = autorun_cleanup;
	plugin->funcs->callbacks = plugin_callbacks;

	// go forth and come fifth.
	GEANY_PLUGIN_REGISTER(plugin, 225);
}
