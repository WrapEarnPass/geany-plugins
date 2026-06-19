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

/* Handler to read any Project declared Auto-run configs */
static void on_project_open(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data){
	//if the keyfile has an [autorun] section
		//load any handlers
			//over any existing [autorun] handlers
}

/* Handler to clear any old and read any new Project declared Auto-run configs */
static void on_project_save(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data) {
	//if the keyfile has an [autorun] section
		//load any handlers
			//over any existing [autorun] handlers
}

/* Handler to clear any old Project declared Auto-run configs */
static void on_project_close(G_GNUC_UNUSED GObject* obj, G_GNUC_UNUSED gpointer user_data) {
	//if we stashed a filetypes.FILE from plugin-init
		//unload handlers and fallback to any filetypes.FILE
}

/* Handler to run any applicable Auto-run configs after a write*/
void on_doc_save(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc, G_GNUC_UNUSED gpointer user_data){
	//if there are any [autorun] OS handlers that exist for this FT
		//iterate through them and run in Terminal
}

/* Handler to run any applicable Auto-run configs before a write*/
void on_doc_before_save(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc, G_GNUC_UNUSED gpointer user_data){
	//if there are any [autorun] BS handlers that exist for this FT
		//iterate through them and run them in Terminal
}

PluginCallback plugin_callbacks[] = { 
	{ "project-open", (GCallback)&on_project_open, TRUE, NULL },
	{ "project-close", (GCallback)&on_project_close, TRUE, NULL },
	{ "project-save", (GCallback)&on_project_save, TRUE, NULL },
	{ "document-save", (GCallback)&on_doc_save, TRUE, NULL},
	{ "document-before-save", (GCallback)&on_doc_before_save, TRUE, NULL},
	{ NULL, NULL, FALSE, NULL }
};


/* Bring up plugin and load filetypes.FILE that exist for Auto-run */
static gboolean autorun_init(GeanyPlugin* plugin, gpointer pdata) {
	// if there are any filetypes.FILE autorun sections
		// stash filetypes.FILE to fallback on-project-close
	return TRUE;
}

/* ensure destruction of any Auto-run objects */
static void autorun_cleanup(GeanyPlugin* plugin, gpointer pdata) {
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
