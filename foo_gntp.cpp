#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include <stdio.h>

#include <direct.h> // for getcwd
#include <stdlib.h>// for MAX_PATH

#define PLUGIN_NAME		"Foobar GNTP"
#define PLUGIN_AUTHOR	"Daniel Dimovski <daniel.k.dimovski@gmail.com>"
#define PLUGIN_DESC		"Plugin sends foobar notifications to Growl."
#define VERSION			"0.1"
#define SERVER_IP 		"127.0.0.1:23053"

char CurrentPath[_MAX_PATH];

char* notifications[] = {
	"Playback Started",
	"Playback Stopped",
	"Playback Paused"
};

#include "gntp-send.h"

using namespace pfc;

void growl(char* type, char* title, char* notice)
{
	gntp_register(NULL);
	gntp_notify(type, CurrentPath, title, notice, NULL);
}

void playback_stopped(play_control::t_stop_reason p_reason)
{
	switch ( p_reason )
	{
		case play_control::t_stop_reason::stop_reason_user : 
			growl("Playback Stopped", "Playback Stopped", "Stopped by user");
			break;
		case play_control::t_stop_reason::stop_reason_eof : 
			growl("Playback Stopped", "Playback Stopped", "Reached end of playlist");
			break;
		case play_control::t_stop_reason::stop_reason_starting_another : 
			//growl("starting new one");
			break;
		case play_control::t_stop_reason::stop_reason_shutting_down : 
			growl("Playback Stopped", "Playback Stopped", "Foobar2000 shutting down");
			break;
		case 5 : 
			growl("Playback Paused", "Playback Paused", "Paused by user");
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

	int len = strlen(title.toString()) + strlen(artist.toString()) + strlen(album.toString()) + 3;
	char *message = new char[len];

	strcpy(message, artist.toString());
	strcat (message, "\n");
	strcat (message, album.toString());
	strcat (message, "\n");
	strcat (message, title.toString());
	
	growl("Playback Started", "Playback Started", message);
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