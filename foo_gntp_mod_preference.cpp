#include "stdafx.h"
#include "resource.h"
#include "foo_gntp_mod_preference.h"

#define	DEFAULT_IP_ADDRESS	"127.0.0.1"
#define DEFAULT_PORT_NUMBER	"23053"

// {53ADD5E5-4845-43C9-B91B-FCCD04CB763A}
static const GUID guid_cfg_ip_address = { 0x53add5e5, 0x4845, 0x43c9, { 0xb9, 0x1b, 0xfc, 0xcd, 0x4, 0xcb, 0x76, 0x3a } };
// {301E47B7-300A-467D-BBDE-0E981C9FE91B}
static const GUID guid_cfg_port_number = { 0x301e47b7, 0x300a, 0x467d, { 0xbb, 0xde, 0xe, 0x98, 0x1c, 0x9f, 0xe9, 0x1b } };

cfg_string cfg_ip_address( guid_cfg_ip_address, DEFAULT_IP_ADDRESS );
cfg_string cfg_port_number( guid_cfg_port_number, DEFAULT_PORT_NUMBER );


bool string8ToWChar( pfc::string8 in, wchar_t *u16buff, t_size buffsize )
{
	try{
		t_size u16buffsize;

		// �o�b�t�@�T�C�Y�擾
		u16buffsize = MultiByteToWideChar( CP_UTF8, 0, in.get_ptr(), -1, NULL, 0 );
		if( ( u16buffsize != 0 ) &&
			( u16buffsize < buffsize ) )
		{
			// �ϊ����s�I
			if( MultiByteToWideChar( CP_UTF8, 0, in.get_ptr(), -1, u16buff, u16buffsize ) )
			{
				return( true );
			}
		}
	}
	catch( std::exception ex )
	{
	}
	return( false );
}

bool WCharToString8( wchar_t *in, pfc::string8 *out )
{
	try{
		t_size u8buffsize;
		char *u8buff;

		// �o�b�t�@�T�C�Y�擾
		u8buffsize = WideCharToMultiByte( CP_UTF8, 0, in, -1, NULL, 0, NULL, NULL );
		if( u8buffsize != 0 )
		{
			u8buffsize++;
			u8buff = new char[u8buffsize];

			// �ϊ����s�I
			if( WideCharToMultiByte( CP_UTF8, 0, in, -1, u8buff, u8buffsize, NULL, NULL ) )
			{
				*out = u8buff;
				delete[] u8buff;

				return( true );
			}
			delete[] u8buff;
		}
	}
	catch( std::exception ex )
	{
	}
	return( false );
}

class CGntpPreference : public CDialogImpl<CGntpPreference>, public preferences_page_instance
{
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CGntpPreference(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//dialog resource ID
	enum {IDD = IDD_DIALOG1};

	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CGntpPreference)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_EDIT1, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_EDIT2, EN_CHANGE, OnEditChange)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CGntpPreference::OnInitDialog(CWindow, LPARAM)
{
	const int buffsize = 1024;
	wchar_t buff[buffsize];

	if( string8ToWChar( cfg_ip_address, buff, buffsize ) )
	{
		SetDlgItemText( IDC_EDIT1, buff );
	}

	if( string8ToWChar( cfg_port_number, buff, buffsize ) )
	{
		SetDlgItemText( IDC_EDIT2, buff );
	}
	return FALSE;
}

void CGntpPreference::OnEditChange( UINT, int, CWindow )
{
	// not much to do here
	OnChanged();
}

t_uint32 CGntpPreference::get_state()
{
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return( state );
}


void CGntpPreference::reset()
{
	const int buffsize = 1024;
	wchar_t buff[buffsize];

	if( string8ToWChar( cfg_ip_address, buff, buffsize ) )
	{
		SetDlgItemText( IDC_EDIT1, buff );
	}

	if( string8ToWChar( cfg_port_number, buff, buffsize ) )
	{
		SetDlgItemText( IDC_EDIT2, buff );
	}

	OnChanged();
}

void CGntpPreference::apply()
{
	const int buffsize = 1024;
	wchar_t buff[buffsize];

	GetDlgItemText( IDC_EDIT1, buff, buffsize );
	WCharToString8( buff, &cfg_ip_address );
	GetDlgItemText( IDC_EDIT2, buff, buffsize );
	WCharToString8( buff, &cfg_port_number );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}


bool CGntpPreference::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	const int buffsize = 1024;
	wchar_t buff[buffsize];
	pfc::string8 str8buff;
	bool ret = false;

	GetDlgItemText( IDC_EDIT1, buff, buffsize );
	WCharToString8( buff, &str8buff );
	ret= ret || ( str8buff != cfg_ip_address );

	GetDlgItemText( IDC_EDIT2, buff, buffsize );
	WCharToString8( buff, &str8buff );
	ret= ret || ( str8buff != cfg_port_number );

	return ret;
}

void CGntpPreference::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
	ResetGrowl();
}

class preferences_page_gntp : public preferences_page_impl<CGntpPreference> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name()
	{
		return( "GNTP Setting" );
	}

	GUID get_guid()
	{
		// {513D52FF-7B60-49EF-AA62-B1C6A5FF48C5}
		static const GUID guid = { 0x513d52ff, 0x7b60, 0x49ef, { 0xaa, 0x62, 0xb1, 0xc6, 0xa5, 0xff, 0x48, 0xc5 } };
		return( guid );
	}
	GUID get_parent_guid()
	{
		return( guid_tools );
	}
};

static preferences_page_factory_t<preferences_page_gntp> g_preferences_page_myimpl_factory;
