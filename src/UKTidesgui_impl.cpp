/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  UKTides Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2019 by Mike Rossiter                                   *
 *   $EMAIL$                                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include "UKTidesgui_impl.h"
#include <wx/wx.h>
#include "wx/dir.h"
#include "UKTides_pi.h"

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/textfile.h>
#include <wx/url.h>

#include <wx/glcanvas.h>
#include <wx/graphics.h>
#include "qtstylesheet.h"

#ifdef __OCPN__ANDROID__
wxWindow *g_Window;
#endif

#ifdef __WXOSX__
# include <OpenGL/OpenGL.h>
# include <OpenGL/gl3.h>
#endif

#ifdef __OCPN__ANDROID__
#include <qopengl.h>
#include "gl_private.h"
#endif

#ifdef USE_GLES2
#include "GLES2/gl2.h"
#endif


class Position;
struct myPort;

static int texture_format;
static bool glQueried = false;

static GLboolean QueryExtension( const char *extName )
{
    /*
     ** Search for extName in the extensions string. Use of strstr()
     ** is not sufficient because extension names can be prefixes of
     ** other extension names. Could use strtok() but the constant
     ** string returned by glGetString might be in read-only memory.
     */
    char *p;
    char *end;
    int extNameLen;

    extNameLen = strlen( extName );

    p = (char *) glGetString( GL_EXTENSIONS );
    if( NULL == p ) {
        return GL_FALSE;
    }

    end = p + strlen( p );

    while( p < end ) {
        int n = strcspn( p, " " );
        if( ( extNameLen == n ) && ( strncmp( extName, p, n ) == 0 ) ) {
            return GL_TRUE;
        }
        p += ( n + 1 );
    }
    return GL_FALSE;
}

#if defined(__WXMSW__)
#define systemGetProcAddress(ADDR) wglGetProcAddress(ADDR)
#elif defined(__WXOSX__)
#include <dlfcn.h>
#define systemGetProcAddress(ADDR) dlsym( RTLD_DEFAULT, ADDR)
#elif defined(__OCPN__ANDROID__)
#define systemGetProcAddress(ADDR) eglGetProcAddress(ADDR)
#else
#define systemGetProcAddress(ADDR) glXGetProcAddress((const GLubyte*)ADDR)
#endif

Dlg::Dlg(UKTides_pi &_UKTides_pi, wxWindow* parent)
	: DlgDef(parent),
	m_UKTides_pi(_UKTides_pi)
{

	this->Fit();
    	dbg=false; //for debug output set to true

#ifdef __OCPN__ANDROID__
    g_Window = this;
    GetHandle()->setStyleSheet( qtStyleSheet);
    Connect( wxEVT_MOTION, wxMouseEventHandler( Dlg::OnMouseEvent ) );
#endif

	LoadTidalEventsFromXml();
	RemoveOldDownloads();

	b_clearAllIcons = true;
	b_clearSavedIcons = true;
}

Dlg::~Dlg()
{
}

#ifdef __OCPN__ANDROID__ 
wxPoint g_startPos;
wxPoint g_startMouse;
wxPoint g_mouse_pos_screen;

void Dlg::OnMouseEvent( wxMouseEvent& event )
{
    g_mouse_pos_screen = ClientToScreen( event.GetPosition() );
    
    if(event.Dragging()){
        int x = wxMax(0, g_startPos.x + (g_mouse_pos_screen.x - g_startMouse.x));
        int y = wxMax(0, g_startPos.y + (g_mouse_pos_screen.y - g_startMouse.y));
        int xmax = ::wxGetDisplaySize().x - GetSize().x;
        x = wxMin(x, xmax);
        int ymax = ::wxGetDisplaySize().y - (GetSize().y * 2);          // Some fluff at the bottom
        y = wxMin(y, ymax);
        
        g_Window->Move(x, y);
    }
}
#endif

void Dlg::OnInformation(wxCommandEvent& event)
{

/*
	wxFileName fn;
	wxString tmp_path;

	tmp_path = GetPluginDataDir("UKTides_pi");
	fn.SetPath(tmp_path);
	fn.AppendDir("data");
	fn.AppendDir("pictures");
	fn.SetFullName("UKTides.html");
	wxString infolocation = fn.GetFullPath();

	wxLaunchDefaultBrowser("file:///" + infolocation);*/
}

void Dlg::SetViewPort(PlugIn_ViewPort *vp)
{
	if (m_vp == vp)  return;
	delete m_vp;
	m_vp = new PlugIn_ViewPort(*vp);
}


bool Dlg::RenderOverlay(piDC &dc, PlugIn_ViewPort &vp)
{
	m_dc = &dc;	

	if (!dc.GetDC()) {
		if (!glQueried) {
			
			glQueried = true;
		}
#ifndef USE_GLSL
		glPushAttrib(GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT); //Save state

		//      Enable anti-aliased lines, at best quality
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
		glEnable(GL_BLEND);
	}

	wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	m_dc->SetFont(font);
	
	if (!b_clearAllIcons) {
		if (myports.size() != 0) {
			DrawAllStationIcons(&vp, false, false, false);
		}
	}

	if (!b_clearSavedIcons) {
		if (mySavedPorts.size() != 0) {
			DrawAllSavedStationIcons(&vp, false, false, false);
		}
	}
	
    return true;
}

void Dlg::DrawAllStationIcons(PlugIn_ViewPort *BBox, bool bRebuildSelList,
	bool bforce_redraw_icons, bool bdraw_mono_for_mask)
{	
	
	if (myports.size() == 0) return;

	wxColour text_color;
    GetGlobalColor( _T ("UINFD" ), &text_color );
    if (text_color != m_text_color) {
       // color changed, invalid cache
       m_text_color = text_color;
       m_labelCacheText.clear();
    }

	wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	m_dc->SetFont(font);

	 if( m_dc ) {
		wxColour color = wxColour("RED");
	    wxPen pen( color, 2 );

        m_dc->SetPen( pen );
        
    }
	
	double plat = 0.0;
	double plon = 0.0;
	myPort outPort;
	
	for (std::list<myPort>::iterator it = myports.begin(); it != myports.end(); it++) {
		
		plat = (*it).coordLat;
		plon = (*it).coordLon;
		outPort.Name = (*it).Name;
		int pixxc, pixyc;
		wxPoint cpoint;

		GetCanvasPixLL(BBox, &cpoint, plat, plon);
                pixxc = cpoint.x;
                pixyc = cpoint.y;

		wxRect myRect = BBox->rv_rect;
        if( myRect.Contains(cpoint.x, cpoint.y)){
			

#ifdef __OCPN__ANDROID__

			int x = pixxc;
			int y = pixyc;
			int w = 20;
			int h = 20;	

			if (m_dc) {
				wxColour myColour = wxColour("YELLOW");
				DrawLine(pixxc, pixyc, pixxc + 20, pixyc + 20, myColour, 4);
					
				// draw bounding rectangle //
				DrawLine(x, y, x + w, y, myColour, 2);
				DrawLine(x + w, y, x + w, y + h, myColour, 2);
				DrawLine(x + w, y + h, x, y + h, myColour, 2);
				DrawLine(x, y + h, x, y, myColour, 2);
			}
#else
			m_dc->DrawBitmap(m_stationBitmap, pixxc, pixyc, false);
#endif
			int textShift = -15;

			if (!m_dc) {

				//DrawGLLabels(this, m_pdc, BBox,
					//DrawGLTextString(outPort.Name), plat, plon, textShift);
			}
			else {			
				wxString name = outPort.Name;
				m_dc->DrawText(name, pixxc, pixyc + textShift);
			}
		}
	}
}

void Dlg::DrawAllSavedStationIcons(PlugIn_ViewPort *BBox, bool bRebuildSelList,
	bool bforce_redraw_icons, bool bdraw_mono_for_mask)
{
	if (mySavedPorts.size() == 0) return;
	
	double plat = 0.0;
	double plon = 0.0;
	myPort outPort;

	for (list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {

		plat = (*it).coordLat;
		plon = (*it).coordLon;
		outPort.Name = (*it).Name;

		int pixxc, pixyc;
		wxPoint cpoint;
		
		GetCanvasPixLL(BBox, &cpoint, plat, plon);
		pixxc = cpoint.x;
		pixyc = cpoint.y;
		
		
		wxRect myRect = BBox->rv_rect;
        if( myRect.Contains(cpoint.x, cpoint.y)){


#ifdef __OCPN__ANDROID__

			int x = pixxc;
			int y = pixyc;
			int w = 20;
			int h = 20;		

			if (m_dc) {
				wxColour myColour = wxColour("YELLOW");
				DrawLine(pixxc, pixyc, pixxc + 20, pixyc + 20, myColour, 4);
					
				// draw bounding rectangle //
				DrawLine(x, y, x + w, y, myColour, 2);
				DrawLine(x + w, y, x + w, y + h, myColour, 2);
				DrawLine(x + w, y + h, x, y + h, myColour, 2);
				DrawLine(x, y + h, x, y, myColour, 2);
			}
#else
			m_dc->DrawBitmap(m_stationBitmap, pixxc, pixyc, true);
#endif

			int textShift = -15;

			if (!m_dc) {

				//DrawGLLabels(this, m_pdc, BBox,
					//DrawGLTextString(outPort.Name), plat, plon, textShift);
			}
			else {
				m_dc->DrawText(outPort.Name, pixxc, pixyc + textShift);
			}
		}
	}	
}

void Dlg::DrawLine(double x1, double y1, double x2, double y2,
	const wxColour &color, double width)
{
	//m_dc->ConfigurePen();
	m_dc->SetPen(wxPen(color, width));
	//m_dc->ConfigureBrush();
	m_dc->SetBrush(*wxTRANSPARENT_BRUSH);
	m_dc->DrawLine(x1, y1, x2, y2, false);

}

void Dlg::Addpoint(TiXmlElement* Route, wxString ptlat, wxString ptlon, wxString ptname, wxString ptsym, wxString pttype){
//add point
	TiXmlElement * RoutePoint = new TiXmlElement( "rtept" );
    RoutePoint->SetAttribute("lat", ptlat.mb_str());
    RoutePoint->SetAttribute("lon", ptlon.mb_str());


    TiXmlElement * Name = new TiXmlElement( "name" );
    TiXmlText * text = new TiXmlText( ptname.mb_str() );
    RoutePoint->LinkEndChild( Name );
    Name->LinkEndChild( text );

    TiXmlElement * Symbol = new TiXmlElement( "sym" );
    TiXmlText * text1 = new TiXmlText( ptsym.mb_str() );
    RoutePoint->LinkEndChild( Symbol );
    Symbol->LinkEndChild( text1 );

    TiXmlElement * Type = new TiXmlElement( "type" );
    TiXmlText * text2 = new TiXmlText( pttype.mb_str() );
    RoutePoint->LinkEndChild( Type );
    Type->LinkEndChild( text2 );
    Route->LinkEndChild( RoutePoint );
//done adding point
}

void Dlg::OnDownload(wxCommandEvent& event) {

	b_clearSavedIcons = false;
	b_clearAllIcons = false;

	myports.clear();
	myPort outPort;

	wxString s_lat, s_lon;

	wxString urlString = "https://admiraltyapi.azure-api.net/uktidalapi/api/V1/Stations?key=29f375e044ff41b39340da20d50bc6a7";
	wxURI url(urlString);

	wxString tmp_file = wxFileName::CreateTempFileName("");

	_OCPN_DLStatus ret = OCPN_downloadFile(url.BuildURI(), tmp_file,
		"UKTides", "", wxNullBitmap, this,
		OCPN_DLDS_ELAPSED_TIME | OCPN_DLDS_ESTIMATED_TIME | OCPN_DLDS_REMAINING_TIME | OCPN_DLDS_SPEED | OCPN_DLDS_SIZE | OCPN_DLDS_CAN_PAUSE | OCPN_DLDS_CAN_ABORT | OCPN_DLDS_AUTO_CLOSE,
		10);

	if (ret == OCPN_DL_ABORTED) {

		m_stUKDownloadInfo->SetLabel(_("Aborted"));
		return;
	} else

	if (ret == OCPN_DL_FAILED) {
		wxMessageBox(_("Download failed.\n\nAre you connected to the Internet?"));

		m_stUKDownloadInfo->SetLabel(_("Failed"));
		return;
	}

	else {
		m_stUKDownloadInfo->SetLabel(_("Success"));
	}

    wxString message_body;
	wxFFile fileData;
	fileData.Open(tmp_file, wxT("r"));
	fileData.ReadAll(&message_body);

	Json::CharReaderBuilder builder;
	Json::CharReader* reader = builder.newCharReader();

	wxString message_id;
	Json::Value value;
	string errors;

	bool parsingSuccessful = reader->parse(message_body.c_str(),
		message_body.c_str() + message_body.size(), &value, &errors);
	delete reader;
	
	wxString error = _("No tidal stations found");

	if (!parsingSuccessful) {
		wxLogMessage(error);
		return;
	}

	if (!value.isMember("features")) {
		// Originator
		wxLogMessage(_("No features found in message"));
		return;
	}

	int i = value["features"].size();

	for (int j = 0; j < i; j++) {

		Json::Value  features = value["features"][j];

		if (!features.isMember("properties")) {
			// Originator
			wxLogMessage(_("No properties found in message"));
		}

		string name = features["properties"]["Name"].asString();
		wxString myname(name.c_str(), wxConvUTF8);
		outPort.Name = myname;
		
		string id = features["properties"]["Id"].asString();
		wxString myId(id.c_str(), wxConvUTF8);
		outPort.Id = myId;

		string lon = features["geometry"]["coordinates"][0].asString();
		s_lon = lon.c_str(), wxConvUTF8;
		string lat = features["geometry"]["coordinates"][1].asString();
		s_lat = lat.c_str(), wxConvUTF8;

		double myLat, myLon;
		s_lat.ToDouble(&myLat);
		s_lon.ToDouble(&myLon);

		outPort.coordLat = myLat;
		outPort.coordLon = myLon;

		myports.push_back(outPort);
	}

	SetCanvasContextMenuItemViz(plugin->m_position_menu_id, true);
	fileData.Close();

	b_clearSavedIcons = true;
	b_clearAllIcons = false;

	RequestRefresh(m_parent);
	value.clear();

}

void Dlg::OnGetSavedTides(wxCommandEvent& event) {

	wxString portName;
	wxString sId;

	double myLat, myLon;
	myLat = 0;
	myLon = 0;

	if (mySavedPorts.size() != 0) {
		mySavedPorts.clear();
	}
	
	LoadTidalEventsFromXml();

	if (mySavedPorts.size() == 0) {
		wxMessageBox(_("No locations are available, please download and select a tidal station"));
		return;
	}

	RequestRefresh(m_parent);  //put the saved port icons back

	b_usingSavedPorts = true;


	GetTidalEventDialog* GetPortDialog = new GetTidalEventDialog(this, -1, _("Select the Location"), wxPoint(200, 200), wxSize(300, 200), 		wxRESIZE_BORDER);

	GetPortDialog->dialogText->InsertColumn(0, "", 0, wxLIST_AUTOSIZE);
	GetPortDialog->dialogText->SetColumnWidth(0, 290);
	GetPortDialog->dialogText->InsertColumn(1, "", 0, wxLIST_AUTOSIZE);
	GetPortDialog->dialogText->SetColumnWidth(1, 0);
	GetPortDialog->dialogText->DeleteAllItems();

	int in = 0;
	wxString routeName = "";
	for (list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {

		portName = (*it).Name;

		sId = (*it).Id;
		myLat = (*it).coordLat;
		myLon = (*it).coordLon;
		
		GetPortDialog->dialogText->InsertItem(in, "", -1);
		GetPortDialog->dialogText->SetItem(in, 0, portName);
		in++;
	}
	this->Fit();
	this->Refresh();

	long si = -1;
	long itemIndex = -1;


	wxString portId;

	wxListItem     row_info;
	wxString       cell_contents_string = wxEmptyString;
	bool foundPort = false;

	
	b_clearSavedIcons = false;
	b_clearAllIcons = true;	

	GetParent()->Refresh();

	if (GetPortDialog->ShowModal() != wxID_OK) {
		// Do nothing
	}
	else {

		for (;;) {
			itemIndex = GetPortDialog->dialogText->GetNextItem(itemIndex,
				wxLIST_NEXT_ALL,
				wxLIST_STATE_SELECTED);

			if (itemIndex == -1) break;

			// Got the selected item index
			if (GetPortDialog->dialogText->IsSelected(itemIndex)) {
				si = itemIndex;
				foundPort = true;
				break;
			}
		}

		if (foundPort) {

			// Set what row it is (m_itemId is a member of the regular wxListCtrl class)
			row_info.m_itemId = si;
			// Set what column of that row we want to query for information.
			row_info.m_col = 0;
			// Set text mask
			row_info.m_mask = wxLIST_MASK_TEXT;

			// Get the info and store it in row_info variable.
			GetPortDialog->dialogText->GetItem(row_info);
			// Extract the text out that cell
			cell_contents_string = row_info.m_text;

			for (list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {
				wxString portName = (*it).Name;
				portId = (*it).Id;
				if (portName == cell_contents_string) {
					OnShowSavedPortTides(portId);
				}
			}
		}
	}

	GetParent()->Refresh();

}

void Dlg::DoRemovePortIcons(wxCommandEvent& event) {
	
	b_clearSavedIcons = false;
	b_clearAllIcons = true;
	RequestRefresh(m_parent);

}


void Dlg::DoRemoveAllPortIcons(wxCommandEvent& event) {
		
	b_clearSavedIcons = true;
	b_clearAllIcons = true;
	RequestRefresh(m_parent);
	
}

void Dlg::getHWLW(string id)
{

	myevents.clear();
	TidalEvent outTidalEvent;

	int daysAhead = m_choice3->GetSelection();
	wxString choiceDays = m_choice3->GetString(daysAhead);

	string duration = "?duration=";
	string urlDays = choiceDays.ToStdString();

	string key = "&key=29f375e044ff41b39340da20d50bc6a7";
	string tidalevents = "/TidalEvents";


	wxString urlString = "https://admiraltyapi.azure-api.net/uktidalapi/api/V1/Stations/" + id + tidalevents + duration + urlDays + key;
	wxURI url(urlString);

	wxString tmp_file = wxFileName::CreateTempFileName("");

	_OCPN_DLStatus ret = OCPN_downloadFile(url.BuildURI(), tmp_file,
		"", "", wxNullBitmap, this, OCPN_DLDS_AUTO_CLOSE,
		10);

	wxString myjson;
	wxFFile fileData;
	fileData.Open(tmp_file, wxT("r"));
	fileData.ReadAll(&myjson);

	Json::CharReaderBuilder builder;
	Json::CharReader* reader = builder.newCharReader();

	// construct the JSON root object
	Json::Value  root2;
	// construct a JSON parser
	Json::Reader reader2;
	wxString error = "Unable to parse json";

	if (!reader2.parse((std::string)myjson, root2)) {
		wxLogMessage(error);
		return;
	}

	if (!root2.isArray()) {
		wxLogMessage(error);
		return;
	}
	else {

		int i = root2.size();

		for (int j = 0; j < i; j++) {

			string type = root2[j]["EventType"].asString();
			if (type == "HighWater") type = _("HW");
			else if (type == "LowWater") type = _("LW");
			wxString mytype(type.c_str(), wxConvUTF8);
			outTidalEvent.EventType = mytype;

			Json::Value  jdt = root2[j];

			if (jdt.isMember("DateTime")) {
				string datetime = root2[j]["DateTime"].asString();
				wxString mydatetime(datetime.c_str(), wxConvUTF8);
				outTidalEvent.DateTime = ProcessDate(mydatetime);
			}
			else {
				outTidalEvent.DateTime = "N/A";
			}

			if (jdt.isMember("Height")) {
				double height = root2[j]["Height"].asDouble();
				wxString myheight(wxString::Format("%4.2f", height));
				outTidalEvent.Height = myheight;
			}
			else {
				outTidalEvent.Height = "N/A";
			}

			myevents.push_back(outTidalEvent);

		}
	}

	root2.clear();

	for (std::list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end();) {

		if ((*it).Id == id) {
			mySavedPorts.erase(it);
		}
		else {
			++it;
		}
	}

	mySavedPort = SavePortTidalEvents(myevents, id);
	mySavedPorts.push_back(mySavedPort);

	SaveTidalEventsToXml(mySavedPorts);
	b_HideButtons = true;
	OnShow();
}

void Dlg::OnTest(wxString thePort)
{
	//wxMessageBox("OnTest");
	RemoveSavedPort(thePort);
}

void Dlg::OnShow(void)
{
		tidetable = new TideTable(this, 7000, _("Tides"), wxPoint(200, 200), wxSize(-1, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
		wxString label = m_titlePortName + _("      (Times are UTC)  ") + _(" (Height in metres)");
		tidetable->itemStaticBoxSizer14Static->SetLabel(label);

		if (b_HideButtons) {
			tidetable->m_bDelete->Hide();
			tidetable->m_bDeleteAll->Hide();
		}

		//tidetable->theDialog = this;

		wxString Event;
		wxString EventDT;
		wxString EventHeight;


		if (myevents.empty()) {
			wxMessageBox(_("No tidal data found. Please use right click to select the UK tidal station"));
			return;
		}

		int in = 0;

		for (std::list<TidalEvent>::iterator it = myevents.begin();
			it != myevents.end(); it++) {

			Event = (*it).EventType;
			EventDT = (*it).DateTime;
			EventHeight = (*it).Height;

			tidetable->m_wpList->InsertItem(in, "", -1);
			tidetable->m_wpList->SetItem(in, 0, EventDT);
			tidetable->m_wpList->SetItem(in, 1, Event);
			tidetable->m_wpList->SetItem(in, 2, EventHeight);

			in++;

		}

		AutoSizeHeader(tidetable->m_wpList);
		tidetable->Fit();
		tidetable->Layout();
		tidetable->Show();

		tidetable->theDialog = this;
		
		

}

void Dlg::OnShowSavedPortTides(wxString thisPortId) {

	if (mySavedPorts.empty()) {
		wxMessageBox(_("Please download the locations. \nUse right click to select the UK tidal station"));
		return;
	}

	tidetable = new TideTable(this, 7000, _("Locations Saved"), wxPoint(200, 200), wxSize(-1, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	wxString Event;
	wxString EventDT;
	wxString EventHeight;

	

	for (std::list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {

		if ((*it).Id == thisPortId) {

			wxString m_titlePortTides;
			m_titlePortTides = (*it).Name;

			tidetable->portName = m_titlePortTides;

			wxString label = m_titlePortTides + _("      (Times are UTC)  ") + _(" (Height in metres)");
			tidetable->itemStaticBoxSizer14Static->SetLabel(label);

			list<TidalEvent> savedevents = (*it).tidalevents;

			int in = 0;

			for (list<TidalEvent>::iterator itt = savedevents.begin(); itt != savedevents.end(); itt++) {

				Event = (*itt).EventType;
				EventDT = (*itt).DateTime;
				EventHeight = (*itt).Height;

				tidetable->m_wpList->InsertItem(in, "", -1);
				tidetable->m_wpList->SetItem(in, 0, EventDT);
				tidetable->m_wpList->SetItem(in, 1, Event);
				tidetable->m_wpList->SetItem(in, 2, EventHeight);

				in++;

			}
		}
	}

	AutoSizeHeader(tidetable->m_wpList);
	tidetable->Fit();
	tidetable->Layout();
	tidetable->Show();
	
	tidetable->theDialog = this;

	
}


void Dlg::AutoSizeHeader(wxListCtrl *const list_ctrl)
{
	if (list_ctrl)
	{
		for (int i = 0; i < list_ctrl->GetColumnCount(); ++i)
		{
			list_ctrl->SetColumnWidth(i, wxLIST_AUTOSIZE);
			const int a_width = list_ctrl->GetColumnWidth(i);
			list_ctrl->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
			const int h_width = list_ctrl->GetColumnWidth(i);
			list_ctrl->SetColumnWidth(i, (std::max)(a_width, h_width));
		}
	}
}

void Dlg::getPort(double m_lat, double m_lon) {	
	wxString m_portId;

	if (myports.empty()) {
		wxMessageBox(_("No active tidal stations found. Please download the locations"));
	}

	m_portId = getPortId(m_lat, m_lon);

	if (m_portId.IsEmpty()) {
		wxMessageBox(_("Please try again"));
		return;
	}
		
	if (mySavedPorts.size() != 0) {
		wxString portName, portId;
		
		for (list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {
			portName = (*it).Name;
			portId = (*it).Id;
		
			if (m_portId == portId) {
				
				int dialog_return_value = wxNO;
				mdlg = new wxMessageDialog(this, _("In the saved list \n\nOK: Shows the data for this station.\n\n     Updates the data if online"),
					_("Saved Port"), wxOK_DEFAULT | wxCANCEL | wxICON_WARNING);
				dialog_return_value = mdlg->ShowModal();
				switch(dialog_return_value){
					case wxID_OK :
						b_HideButtons = true;
						getHWLW(m_portId.ToStdString());
						return;
					case wxID_CANCEL :						
						return;
				};
			}			
		}
	} 			
	getHWLW(m_portId.ToStdString());
}

wxString Dlg::getPortId(double m_lat, double m_lon) {

	bool foundPort = false;
	double radius = 0.1;
	double myDist, myBrng;
	double plat;
	double plon;
	wxString m_portId;

	while (!foundPort) {
		for (std::list<myPort>::iterator it = myports.begin();	it != myports.end(); it++) {
				{
					plat = (*it).coordLat;
					plon = (*it).coordLon;

					DistanceBearingMercator(plat, plon, m_lat, m_lon, &myDist, &myBrng);

					if (myDist < radius) {
						m_portId = (*it).Id;
						m_titlePortName = (*it).Name;
						foundPort = true;
						return m_portId;
					}
				}
		}
		radius += 0.1;
	}
	return _("Port not found");
}

wxString Dlg::getSavedPortId(double m_lat, double m_lon) {

	bool foundPort = false;
	double radius = 0.1;
	double myDist, myBrng;
	double plat;
	double plon;
	wxString m_portId;

	if (mySavedPorts.empty()) {
		wxMessageBox(_("No tidal stations found. Please download locations when online"));
		return wxEmptyString;
	}

	while (!foundPort) {
		for (std::list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end(); it++) {
			{
				plat = (*it).coordLat;
				plon = (*it).coordLon;

				DistanceBearingMercator(plat, plon, m_lat, m_lon, &myDist, &myBrng);

				if (myDist < radius) {
					m_portId = (*it).Id;
					m_titlePortName = (*it).Name;
					foundPort = true;
					return m_portId;
				}
			}
		}
		radius += 0.1;
	}
	return _("Port not found");
}


wxString Dlg::ProcessDate(wxString myLongDate) {

	wxDateTime myDateTime;
	myDateTime.ParseISOCombined(myLongDate);
	return myDateTime.Format(" %a %d-%b-%Y   %H:%M");

}

void Dlg::OnClose(wxCloseEvent& event)
{
	plugin->OnUKTidesDialogClose();
}


wxString Dlg::StandardPath()
{
    wxString s = wxFileName::GetPathSeparator();
    wxString stdPath  = *GetpPrivateApplicationDataLocation();

    stdPath += s + _T("plugins") + s + _T("UKTides_pi") + s + "data";
    if (!wxDirExists(stdPath))
      wxMkdir(stdPath);

    return stdPath;
}


myPort Dlg::SavePortTidalEvents(list<TidalEvent>myEvents, string portId)
{
	myPort thisPort;

	double plat, plon;
	plat = 0;
	plon = 0;

	wxString portName;

	for (std::list<myPort>::iterator it = myports.begin(); it != myports.end(); it++) {
		{
			if ((*it).Id == portId) {
				plat = (*it).coordLat;
				plon = (*it).coordLon;
				portName = (*it).Name;
			}
		}
	}

	wxString dtNow = GetDateStringNow();

	thisPort.Name = portName;
	thisPort.DownloadDate = dtNow;
	thisPort.Id = portId;
	thisPort.coordLat = plat;
	thisPort.coordLon = plon;
	thisPort.tidalevents = myevents;

	return thisPort;

}

void Dlg::SaveTidalEventsToXml(list<myPort>myPorts)
{

	wxString tidal_events_path;

	tidal_events_path = StandardPath();

	/* ensure the directory exists */
	wxFileName fn;

	if (!wxDirExists(tidal_events_path)) {
		fn.Mkdir(tidal_events_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	wxString filename = tidal_events_path + "/tidalevents.xml";	

	if (myPorts.size() == 0) {		
		wxTextFile myXML(filename);
		if(!myXML.Exists())
		   return;
		myXML.Open();
		myXML.Clear();
		myXML.Write();
		return;             		
	}
	
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	doc.LinkEndChild(decl);

	TiXmlElement * root = new TiXmlElement("TidalEventDataSet");
	doc.LinkEndChild(root);

	for (list<myPort>::iterator it = myPorts.begin(); it != myPorts.end(); it++) {

		TiXmlElement *Port = new TiXmlElement("Port");
		Port->SetAttribute("Name", (*it).Name);
		Port->SetAttribute("DownloadDate", (*it).DownloadDate);
		Port->SetAttribute("Id", (*it).Id);
		Port->SetAttribute("Latitude", wxString::Format("%.5f", (*it).coordLat));
		Port->SetAttribute("Longitude", wxString::Format("%.5f", (*it).coordLon));

		root->LinkEndChild(Port);

		myevents = (*it).tidalevents;

		for (list<TidalEvent>::iterator it = myevents.begin(); it != myevents.end(); it++) {
			TiXmlElement *t = new TiXmlElement("TidalEvent");

			t->SetAttribute("Event", (*it).EventType.mb_str());
			t->SetAttribute("DateTime", (*it).DateTime.mb_str());
			t->SetAttribute("Height", (*it).Height.mb_str());

			Port->LinkEndChild(t);
		}
	}
	
	
	if (!doc.SaveFile(filename))
		wxLogMessage(_("UKTides") + wxString(": ") + _("Failed to save xml file: ") + filename);
}

list<myPort>Dlg::LoadTidalEventsFromXml()
{
	
	list<myPort>myEmptyPorts;

	myPort thisPort;
	TidalEvent thisEvent;

	TiXmlDocument doc;

	wxString tidal_events_path;

	tidal_events_path = StandardPath();

    wxString filename = tidal_events_path + "/tidalevents.xml";

	
	/* ensure the directory exists */
	wxFileName fn;
	if (!wxDirExists(tidal_events_path)) {
		fn.Mkdir(tidal_events_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
		return myEmptyPorts;
	}
	
	if (!wxFileExists(filename)) {		
		return myEmptyPorts;
	}

	list<TidalEvent> listEvents;

	SetTitle(_("Tidal Events"));

	if (!doc.LoadFile(filename.mb_str())) {
		wxMessageBox(_("No UK tide locations available"));
		return myEmptyPorts;
	}
	else {
		TiXmlHandle root(doc.RootElement());

		if (strcmp(root.Element()->Value(), "TidalEventDataSet"))
			wxMessageBox(_("Invalid xml file"));

		int count = 0;
		for (TiXmlElement* e = root.FirstChild().Element(); e; e = e->NextSiblingElement())
			count++;

		int i = 0;
		for (TiXmlElement* e = root.FirstChild().Element(); e; e = e->NextSiblingElement(), i++) {

			if (!strcmp(e->Value(), "Port")) {
				thisPort.Name = e->Attribute("Name");
				thisPort.DownloadDate = e->Attribute("DownloadDate");
				thisPort.Id = e->Attribute("Id");
				thisPort.coordLat = AttributeDouble(e, "Latitude", NAN);
				thisPort.coordLon = AttributeDouble(e, "Longitude", NAN);

				listEvents.clear();

				for (TiXmlElement* f = e->FirstChildElement(); f; f = f->NextSiblingElement()) {
					if (!strcmp(f->Value(), "TidalEvent")) {
						thisEvent.EventType = f->Attribute("Event");
						thisEvent.DateTime = f->Attribute("DateTime");
						thisEvent.Height = f->Attribute("Height");
					}
					listEvents.push_back(thisEvent);
				}

				thisPort.tidalevents = listEvents;
				mySavedPorts.push_back(thisPort);
			}
		}
	}

	return mySavedPorts;

}

double Dlg::AttributeDouble(TiXmlElement *e, const char *name, double def)
{
	const char *attr = e->Attribute(name);
	if (!attr)
		return def;
	char *end;
	double d = strtod(attr, &end);
	if (end == attr)
		return def;
	return d;
}

wxString Dlg::GetDateStringNow() {

	m_dtNow = wxDateTime::Now();
	wxString downloadDate = m_dtNow.Format("%Y-%m-%d  %H:%M");
	return downloadDate;

}

void Dlg::RemoveOldDownloads( ) {
	
	if (mySavedPorts.size() == 0) {				
		return;
	}
	
	wxDateTime dtn, ddt;
	wxString sdt, sddt;
	wxTimeSpan DaySpan;
	DaySpan = wxTimeSpan::Days(7);
	bool tooOld;

	dtn = wxDateTime::Now().ToUTC();
	
	if (mySavedPorts.size() == 1) {	

		myPort thisPort = mySavedPorts.front();

		sddt = thisPort.DownloadDate;
		ddt.ParseDateTime(sddt);
		ddt.Add(DaySpan);
		ddt.ToUTC();

		if (dtn > ddt) {
			mySavedPorts.clear();			
		}

	}
	else {

		for (std::list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end();) {
			tooOld = false;
			sddt = (*it).DownloadDate;
			ddt.ParseDateTime(sddt);
			ddt.Add(DaySpan);
			ddt.ToUTC();
			if (dtn > ddt) tooOld = true;

			if (tooOld) {				
				it = mySavedPorts.erase(it);
			}
			else {
				it++;
			}
		}
	}
	
	SaveTidalEventsToXml(mySavedPorts);
	GetParent()->Refresh();

}

void Dlg::RemoveSavedPort(wxString myStation) {

	if (mySavedPorts.empty()) {
		wxMessageBox(_("No saved tidal stations. Please load"));
		return;
	}

	if (mySavedPorts.size() == 1) {
		mySavedPorts.clear();
	}
	else {
		for (std::list<myPort>::iterator it = mySavedPorts.begin(); it != mySavedPorts.end();) {
			if ((*it).Name == myStation) {				
				mySavedPorts.erase(it);
				break;
			}
			else {
				it++;
			}
		}
	}
	SaveTidalEventsToXml(mySavedPorts);
	GetParent()->Refresh();
}

void Dlg::RemoveAllSavedPorts() {

	if (mySavedPorts.empty()) {
		wxMessageBox(_("No saved tidal stations. Please load"));
		return;
	}

	mySavedPorts.clear();	
	SaveTidalEventsToXml(mySavedPorts);

	GetParent()->Refresh();
}


GetTidalEventDialog::GetTidalEventDialog(wxWindow * parent, wxWindowID id, const wxString & title,
	const wxPoint & position, const wxSize & size, long style)
	: wxDialog(parent, id, title, position, size, style)
{
	wxBoxSizer* itemBoxSizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(itemBoxSizer1);

	itemStaticBoxSizer14Static = new wxStaticBox(this, wxID_ANY, "Locations");
	m_pListSizer = new wxStaticBoxSizer(itemStaticBoxSizer14Static, wxVERTICAL);
	itemBoxSizer1->Add(m_pListSizer, 2, wxEXPAND | wxALL, 1);

	wxBoxSizer* itemBoxSizerBottom = new wxBoxSizer(wxHORIZONTAL);
	itemBoxSizer1->Add(itemBoxSizerBottom, 0, wxALIGN_LEFT | wxALL | wxEXPAND, 5);

	wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
	itemBoxSizerBottom->Add(itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 3);

	m_OKButton = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition,
		wxDefaultSize, 0);
	itemBoxSizer16->Add(m_OKButton, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1);
	m_OKButton->SetDefault();


	wxString dimensions = wxT(""), s;
	wxPoint p;
	wxSize  sz;

	sz.SetWidth(size.GetWidth() - 20);
	sz.SetHeight(size.GetHeight() - 70);

	p.x = 6; p.y = 2;

	dialogText = new wxListView(this, wxID_ANY, p, sz, wxLC_NO_HEADER | wxLC_REPORT | wxLC_SINGLE_SEL, wxDefaultValidator, wxT(""));
	m_pListSizer->Add(dialogText, 1, wxEXPAND | wxALL, 6);

	wxFont *pVLFont = wxTheFontList->FindOrCreateFont(12, wxFONTFAMILY_SWISS, wxNORMAL, wxFONTWEIGHT_NORMAL,
		FALSE, wxString("Arial"));
	dialogText->SetFont(*pVLFont);

}
