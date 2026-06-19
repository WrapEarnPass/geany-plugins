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

/* Handler to read any Project declared Auto-run configs 
 * For Auto-run, this the same as a project-save event */
static void on_project_open(G_GNUC_UNUSED GObject* obj, GKeyFile* config, G_GNUC_UNUSED gpointer user_data){
	gsize key_len;
	GError * gerr=NULL;
	gchar** handlers = g_key_file_get_keys(config,"autorun",&key_len, &gerr);
	//if the keyfile has an [autorun] section
	if( gerr==NULL ){
		//load any handlers
			//over any existing [autorun] handlers
	}else{
		g_free(gerr);
	}
	//The caller of g_key_file_get_keys takes ownership of the returned data, and is responsible for freeing it.
	g_free(handlers);
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
	if(!doc->changed)
	{
		return;
	}
}

/* Handler to run any applicable Auto-run configs before a write*/
void on_doc_before_save(G_GNUC_UNUSED GObject *obj, GeanyDocument *doc, G_GNUC_UNUSED gpointer user_data){
	//if there are any [autorun] BS handlers that exist for this FT
		//iterate through them and run them in Terminal
	if(!doc->changed)
	{
		return;
	}
}

PluginCallback plugin_callbacks[] = { 
	{ "project-open", (GCallback)&on_project_open, TRUE, NULL },
	{ "project-save", (GCallback)&on_project_open, TRUE, NULL },
	{ "project-close", (GCallback)&on_project_close, TRUE, NULL },
	{ "document-save", (GCallback)&on_doc_save, TRUE, NULL},
	{ "document-before-save", (GCallback)&on_doc_before_save, TRUE, NULL},
	{ NULL, NULL, FALSE, NULL }
};


/* Bring up plugin and load filetypes.FILE that exist for Auto-run */
static gboolean autorun_init(GeanyPlugin* plugin, gpointer pdata) {
	if (!autorun_globals || autorun_globals == NULL) {
		autorun_globals_init();
		autorun_globals->geany_plugin = plugin;
		GeanyData *	geany_data = plugin->geany_data;
		// if there are any filetypes.FILE autorun sections
		gchar * filedef_path = g_build_path(G_DIR_SEPARATOR_S, geany->app->configdir, "filedefs", NULL); 
		guint filedef_len;
		GSList * file_list = utils_get_file_list(filedef_path, &filedef_len, NULL);
		if( filedef_len > 0){
			// stash filetypes.FILE to fallback on-project-close
			GSList* node;
			foreach_slist(node,file_list){
				
				if( g_str_has_prefix(node->data, "filetypes.") && g_strcmp0(node->data, "filetypes.README")!=0 )
				{
					//we have files to process
					g_message("found filedef at %s",(gchar*)node->data);
					gsize key_len;
					gchar * filedef_file = g_build_filename(G_DIR_SEPARATOR_S, filedef_path, (gchar*)node->data, NULL);
					GKeyFile* config=g_key_file_new ();
					g_key_file_load_from_file(config,filedef_file ,G_KEY_FILE_NONE, NULL);
					GError * gerr=NULL;
					gchar** handlers = g_key_file_get_keys(config, "autorun", &key_len, &gerr);
					//if the keyfile has an [autorun] section
					if( gerr == NULL ){
						g_message("found autorun filedef at %s of %li",(gchar*)node->data,key_len);
						
						
					}else
					{
						g_free(gerr);
						g_message("no autorun filedef at %s",(gchar*)node->data);
					}
					g_free(filedef_file);
					g_free(handlers);
				}
			}
			
		}
		//cleanup
		g_slist_foreach(file_list, (GFunc) g_free, NULL);
		g_slist_free(file_list); 
		g_free(filedef_path);
		
		
		if(geany_data && geany_data->app && geany_data->app->project) {
			//if initialized while a project is already open, manually ingest the keyfile	
			// force a GKeyFile
			GKeyFile* config=g_key_file_new ();
			gchar* configfile;
			configfile = g_build_filename(geany_data->app->project->base_path, geany_data->app->project->file_name, NULL);
			msgwin_status_add("%s", configfile);
			g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);
			on_project_open(NULL, config, NULL);
			g_free(configfile);
			g_free(config);	
		}
		
		
	}
	return TRUE;
}

/* ensure destruction of any Auto-run objects */
static void autorun_cleanup(GeanyPlugin* plugin, gpointer pdata) {
	autorun_globals_cleanup();
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
