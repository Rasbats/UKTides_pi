///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DlgDef
///////////////////////////////////////////////////////////////////////////////
class DlgDef : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText9;
		wxChoice* m_choice3;
		wxStaticLine* m_staticline2;

		// Virtual event handlers, override them in your derived class
		virtual void OnDownload( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGetSavedTides( wxCommandEvent& event ) { event.Skip(); }
		virtual void DoRemovePortIcons( wxCommandEvent& event ) { event.Skip(); }
		virtual void DoRemoveAllPortIcons( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxStaticText* m_stTimeZone;
		wxButton* m_buttonDownload;
		wxStaticText* m_stUKDownloadInfo;
		wxButton* m_buttonSaved;
		wxButton* m_buttonSaved1;
		wxButton* m_buttonSaved11;

		DlgDef( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("UKTides"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DlgDef();

};

