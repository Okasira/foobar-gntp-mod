#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include <stdio.h>

#include <direct.h> // for getcwd
#include <stdlib.h>// for MAX_PATH

#include <fstream>

using namespace pfc;

#define PLUGIN_NAME		"Foobar GNTP"
#define PLUGIN_AUTHOR	"Daniel Dimovski <daniel.k.dimovski@gmail.com>"
#define PLUGIN_DESC		"Plugin sends Foobar notifications to Growl."
#define VERSION			"0.2.2"
#define SERVER_IP 		"127.0.0.1:23053"

char CurrentPath[_MAX_PATH];
char AlbumArtPath[_MAX_PATH];

const char* notifications[] = {
	"Playback Started",
	"Playback Stopped",
	"Playback Paused"
};

typedef int (__cdecl* GROWL_REGISTER)(const char *const server , const char *const appname ,
	const char **const notifications , const int notifications_count , const char *const password, const char* icon_path);

typedef int (__cdecl* GROWL_NOTIFY)(const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password, const char* const url, const char* const icon);

GROWL_REGISTER growl_register;   // Function pointer
GROWL_NOTIFY growl_notify;    	// Function pointer

static bool connected = false;
static bool registered = false;

void connect_to_dll()
{
	if(connected)
		return;
	
	
	HMODULE hDLL;               // Handle to DLL
	hDLL = LoadLibraryA("growl.dll");
	if (hDLL != NULL)
	{
	   growl_register = (GROWL_REGISTER)GetProcAddress(hDLL, "growl_tcp_register");
	   growl_notify = (GROWL_NOTIFY)GetProcAddress(hDLL, "growl_tcp_notify");

	   if (!growl_register || !growl_notify)
	   {
		  popup_message::g_show("Failed to fetch functions from growl.dll","Foobar-GNTP");
	      FreeLibrary(hDLL);       
	   }
	   else
	   {	
		   connected = true;
	   }
	}
	else
	{
		  popup_message::g_show("Failed to load growl.dll","Foobar-GNTP");
	}
}

void growl(char* type, char* title, char* notice, bool hasAlbumArt)
{
	connect_to_dll();

	if(!registered)
	{
		_getcwd(CurrentPath, _MAX_PATH);
		strcat_s(CurrentPath, "/icons/foobar2000.png");

		if(growl_register)
		{
			growl_register(SERVER_IP, PLUGIN_NAME, static_cast<const char **const>(notifications), 3, 0, CurrentPath);
			registered = true;
		}
	}

	if(growl_notify)
	{
		if(!hasAlbumArt)
			growl_notify(SERVER_IP, PLUGIN_NAME, type, title, notice, NULL, NULL, CurrentPath);
		else
			growl_notify(SERVER_IP, PLUGIN_NAME, type, title, notice, NULL, NULL, &AlbumArtPath[7]);
	}
}

void playback_stopped(play_control::t_stop_reason p_reason)
{
	switch ( p_reason )
	{
		case play_control::stop_reason_user : 
			growl("Playback Stopped", "Playback Stopped", "Stopped by user", NULL);
			break;
		case play_control::stop_reason_eof : 
			growl("Playback Stopped", "Playback Stopped", "Reached end of playlist", NULL);
			break;
		case play_control::stop_reason_starting_another : 
			//growl("starting new one");
			break;
		case play_control::stop_reason_shutting_down : 
			growl("Playback Stopped", "Playback Stopped", "Foobar2000 shutting down", NULL);
			break;
		case 5 : 
			growl("Playback Paused", "Playback Paused", "Paused by user", NULL);
			break;
	}

}

void playback_new_track(metadb_handle_ptr track)
{
	if (track.is_empty())
		return;
		
	static_api_ptr_t<titleformat_compiler> compiler;
	
	string8 title;
	string8 artist;
	string8 album;

	service_ptr_t<titleformat_object> title_obj;
	
	compiler->compile_safe(title_obj, "%title%");
	track->format_title(NULL, title, title_obj, NULL);

	compiler->compile_safe(title_obj, "%artist%");
	track->format_title(NULL, artist, title_obj, NULL);

	compiler->compile_safe(title_obj, "%album%");
	track->format_title(NULL, album, title_obj, NULL);

	int len = strlen(title.toString()) + strlen(artist.toString()) + strlen(album.toString()) + 5;
	char *message = new char[len];

	strcpy_s(message, len, artist.toString());
	strcat_s (message, len, "\n\"");
	strcat_s (message, len, title.toString());
	strcat_s (message, len, "\"\n");
	strcat_s (message, len, album.toString());
	
	DeleteFileA(&AlbumArtPath[7]);	

	AlbumArtPath[0] = '\0';
	strcat_s (AlbumArtPath, core_api::get_profile_path());
	strcat_s (AlbumArtPath, "/");
	strcat_s (AlbumArtPath, title.toString());
	
	abort_callback_dummy *dummy = new abort_callback_dummy(); // never aborts
	album_art_manager_instance_ptr aamip = static_api_ptr_t<album_art_manager>()->instantiate();
	aamip->open(track->get_path(),*dummy);
	album_art_data_ptr art = NULL;

	const void *ptr;

	try
	{
		art = aamip->query(album_art_ids::cover_front, *dummy);
		ptr = art->get_ptr();

		std::fstream the_file (&AlbumArtPath[7], std::ios::out | std::ios::binary);
	    the_file.seekg (0);
		the_file.write( reinterpret_cast<const char *>(art->get_ptr()), art->get_size());
	    the_file.close();
	}
	catch(exception_album_art_not_found e)
	{
		ptr = NULL;
	}
	
	growl("Playback Started", "Playback Started", message, ptr ? true : false );
	delete[] message;
}

class play_callback_gntp : public play_callback_static
{
	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	virtual void on_playback_new_track(metadb_handle_ptr p_track)
	{
		playback_new_track(p_track);
	}
	virtual void on_playback_stop(play_control::t_stop_reason p_reason)
	{
		playback_stopped(p_reason);
	}
	virtual void on_playback_seek(double p_time) {}
	virtual void on_playback_pause(bool p_state)
	{
		if (p_state)
			playback_stopped((play_control::t_stop_reason)5);
		else
		{
			metadb_handle_ptr track;
			static_api_ptr_t<play_control>()->get_now_playing(track);
			if (track.is_valid())
			{
				playback_new_track(track);
				track.release();
			}
		}
	}
	virtual void on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void on_playback_dynamic_info(const file_info & info) {}
	virtual void on_playback_dynamic_info_track(const file_info & info) {}
	virtual void on_playback_time(double p_time) {}
	virtual void on_volume_change(float p_new_val) {}
	virtual unsigned get_flags() 
	{
		return flag_on_playback_new_track | flag_on_playback_pause | flag_on_playback_stop;
	}
};

static play_callback_static_factory_t<play_callback_gntp> gntp_callback_factory;

DECLARE_COMPONENT_VERSION(PLUGIN_NAME,VERSION,0)