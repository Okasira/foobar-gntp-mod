#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#define PLUGIN_NAME		"Foobar GNTP"
#define PLUGIN_AUTHOR	"Daniel Dimovski <daniel.k.dimovski@gmail.com>"
#define PLUGIN_DESC		"Plugin sends foobar notifications to Growl."
#define ICON_PATH 		"C:/Program Files (x86)/foobar2000/components/foobar2000.png"
#define VERSION			"0.1"
#define SERVER_IP 		"127.0.0.1:23053"

char* notifications[] = {
	"Track Change"
};

#include "gntp-send.h"

using namespace pfc;

void playback_stopped()
{
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

	gntp_register(NULL);
	gntp_notify("Track Change", ICON_PATH, "Track Change", message, NULL);
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
		playback_stopped();
	}
	virtual void on_playback_seek(double p_time) {}
	virtual void on_playback_pause(bool p_state)
	{
		if (p_state)
			playback_stopped();
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
	virtual void on_volume_change(float p_new_val) {};
	
	virtual unsigned get_flags() 
	{
		return flag_on_playback_new_track | flag_on_playback_pause | flag_on_playback_stop;
	}

};

static play_callback_static_factory_t<play_callback_gntp> msn_callback_factory;

DECLARE_COMPONENT_VERSION(PLUGIN_NAME,VERSION,0)