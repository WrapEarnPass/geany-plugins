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
#include "spawn.h"

/* Handler to read any Project declared Auto-run configs */
static void on_project_open(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data){
	g_message("on_project_open start");
	//TODO make this more smarter
	if(autorun_globals->project_commands==NULL)
	{
		load_projectdefs(config);
	}
	g_message("on_project_open end");

}


// this handler is currently disconnected due to geany/geany#4603
/* Handler to read any Project declared Auto-run configs */
//static void on_project_save(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data){
//	g_message("on_project_save");
//	load_projectdefs(config);
//}


/* Handler to clear any old Project declared Auto-run configs */
static void on_project_close(G_GNUC_UNUSED GObject* obj, G_GNUC_UNUSED gpointer user_data) {
g_message("project close start");
	//unload project handlers
	autorun_cmd_list_free(autorun_globals->project_commands);
	autorun_globals->project_commands=NULL;
g_message("project close end");
}

/* Handler to run any applicable Auto-run configs after a write*/
void on_doc_save(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc, G_GNUC_UNUSED gpointer user_data){
g_message("on save start");
	/* disabled because geany/geany#4604 never sets doc->changed*/
	if(!doc->changed)
	{
		g_message("doc not changed?");
		return;
	}
	/**/
	dispatch_run("OS", doc);
g_message("on save end");
}


/* Handler to run any applicable Auto-run configs before a write*/
void on_doc_before_save(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc, G_GNUC_UNUSED gpointer user_data){
g_message("before save start");
	if(!doc->changed)
	{
		g_message("doc not changed?");
		return;
	}
	dispatch_run("BS", doc);
g_message("before save end");
}

PluginCallback plugin_callbacks[] = {
	{ "project-open", (GCallback)&on_project_open, TRUE, NULL },
	//{ "project-save", (GCallback)&on_project_save, TRUE, NULL },// geany/geany#4603
	{ "project-close", (GCallback)&on_project_close, TRUE, NULL },
	{ "document-save", (GCallback)&on_doc_save, TRUE, NULL},
	{ "document-before-save", (GCallback)&on_doc_before_save, TRUE, NULL},
	{ NULL, NULL, FALSE, NULL }
};


/* Bring up plugin and load filetypes.FILE that exist for Auto-run */
static gboolean autorun_init(GeanyPlugin* plugin, gpointer pdata) {
	g_message("autorun_init start");
	if (!autorun_globals || autorun_globals == NULL) {
		autorun_globals_init(plugin);
		load_filedefs();
		g_message ("filedef command list is %i" ,g_slist_length(autorun_globals->filedef_commands));

		//if initialized while a project is already open, manually ingest the project
		if(autorun_globals->data->app && autorun_globals->data->app->project) {
			// force a GKeyFile
			GKeyFile* config=g_key_file_new ();
			g_key_file_load_from_file(config,  autorun_globals->data->app->project->file_name, G_KEY_FILE_NONE, NULL);
			load_projectdefs(config);
			g_message ("proj command list is %i" ,g_slist_length(autorun_globals->project_commands));
			g_free(config);
		}
	}
	g_message("autorun_init end");
	return TRUE;
}

/* ensure destruction of any Auto-run objects */
static void autorun_cleanup(GeanyPlugin* plugin, gpointer pdata) {
	g_message("autorun_cleanup start");
	if ( autorun_globals != NULL) {
		autorun_globals_free();
	}
	g_message("autorun_cleanup end");
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin* plugin) {
g_message("load_module start");
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
g_message("load_module end");

}
