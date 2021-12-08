/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  UKTides Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2019 by Mike Rossiter                                *
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "UKTides_pi.h"
#include "UKTidesgui_impl.h"
#include "UKTidesgui.h"

#include <wx/stdpaths.h>



class UKTides_pi;

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new UKTides_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//   UKTides PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#define UKTIDES_TOOL_POSITION    -1 
#include "icons.h"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

UKTides_pi::UKTides_pi(void *ppimgr)
      :opencpn_plugin_116 (ppimgr)
{
		// Create the PlugIn icons
	initialize_images();

	wxFileName fn;

	wxString path = GetPluginDataDir("UKTides_pi");
	fn.SetPath(path);
	fn.AppendDir("data");
	fn.SetFullName("uktides_panel_icon.png");

	path = fn.GetFullPath();
    
    wxInitAllImageHandlers();

	wxLogDebug(wxString("Using icon path: ") + path);
	if (!wxImage::CanRead(path)) {
		wxLogDebug("Initiating image handlers.");
		wxInitAllImageHandlers();
	}

	wxImage panelIcon(path);

	if (panelIcon.IsOk())
		m_panelBitmap = wxBitmap(panelIcon);
	else
		wxLogMessage(_("    UKTides panel icon has NOT been loaded"));



	m_bShowUKTides = false;
}

UKTides_pi::~UKTides_pi(void)
{
     delete _img_uktides;
     
}

int UKTides_pi::Init(void)
{
      AddLocaleCatalog("opencpn-UKTides_pi");

      // Set some default private member parameters
      m_route_dialog_x = 0;
      m_route_dialog_y = 0;
      ::wxDisplaySize(&m_display_width, &m_display_height);

      //    Get a pointer to the opencpn display canvas, to use as a parent for the POI Manager dialog
      m_parent_window = GetOCPNCanvasWindow();

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();

      //    And load the configuration items
      LoadConfig();

      //    This PlugIn needs a toolbar icon, so request its insertion
	  if (m_bUKTidesShowIcon) {

#ifdef PLUGIN_USE_SVG
		  m_leftclick_tool_id = InsertPlugInToolSVG("UKTides", _svg_uktides, _svg_uktides, _svg_uktides_toggled,
			  wxITEM_CHECK, _("UKTides"), "", NULL, UKTIDES_TOOL_POSITION, 0, this);
#else
		  m_leftclick_tool_id = InsertPlugInTool("", _img_uktides, _img_uktides, wxITEM_CHECK,
			  _("UKTides"), "", NULL,
			  UKTIDES_TOOL_POSITION, 0, this);
#endif
	  }
	wxMenu dummy_menu;
	m_position_menu_id = AddCanvasContextMenuItem

	(new wxMenuItem(&dummy_menu, -1, _("Select UK Tidal Station")), this);
	SetCanvasContextMenuItemViz(m_position_menu_id, false);

     m_pDialog = NULL;	 
	
	

      return (WANTS_OVERLAY_CALLBACK |
              WANTS_OPENGL_OVERLAY_CALLBACK |		      
		      WANTS_CURSOR_LATLON      |
              WANTS_TOOLBAR_CALLBACK    |
              INSTALLS_TOOLBAR_TOOL     |
              WANTS_CONFIG            
           );
}

bool UKTides_pi::DeInit(void)
{
      //    Record the dialog position
      if (NULL != m_pDialog)
      {
            //Capture dialog position
            wxPoint p = m_pDialog->GetPosition();
            SetCalculatorDialogX(p.x);
            SetCalculatorDialogY(p.y);
            m_pDialog->Close();
            delete m_pDialog;
            m_pDialog = NULL;

			m_bShowUKTides = false;
			SetToolbarItemState( m_leftclick_tool_id, m_bShowUKTides );

      }	
    
    SaveConfig();
    
    return true;
}

int UKTides_pi::GetAPIVersionMajor()
{
      return atoi(API_VERSION);
}

int UKTides_pi::GetAPIVersionMinor()
{
    std::string v(API_VERSION);
    size_t dotpos = v.find('.');
    return atoi(v.substr(dotpos + 1).c_str());
}

int UKTides_pi::GetPlugInVersionMajor()
{
    return PLUGIN_VERSION_MAJOR;
}

int UKTides_pi::GetPlugInVersionMinor()
{
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *UKTides_pi::GetPlugInBitmap()
{
      return &m_panelBitmap;
}

wxString UKTides_pi::GetCommonName()
{
      return _("UKTides");
}


wxString UKTides_pi::GetShortDescription()
{
      return _("UKTides");
}

wxString UKTides_pi::GetLongDescription()
{
      return _("Downloads UKHO Tidal Data for UK ports");
}

int UKTides_pi::GetToolbarToolCount(void)
{
      return 1;
}

void UKTides_pi::SetColorScheme(PI_ColorScheme cs)
{
      if (NULL == m_pDialog)
            return;

      DimeWindow(m_pDialog);
}

void UKTides_pi::OnToolbarToolCallback(int id)
{
    
	if(NULL == m_pDialog)
      {
           
		    m_pDialog = new Dlg(*this, m_parent_window);
            m_pDialog->plugin = this;
            m_pDialog->Move(wxPoint(m_route_dialog_x, m_route_dialog_y));

			wxFileName fn;
			wxString path;

			path = GetPluginDataDir("UKTides_pi");
			fn.SetPath(path);
			fn.AppendDir(_T("data"));
			fn.SetFullName("station_icon.png");

			path = fn.GetFullPath();

			wxLogDebug(wxString("Using station icon path: ") + path);
			if (!wxImage::CanRead(path)) {
				wxLogDebug("Initiating image handlers.");
				wxInitAllImageHandlers();
			}
			
			wxImage stationIcon(path);

			if (stationIcon.IsOk())
				m_pDialog->m_stationBitmap = wxBitmap(stationIcon);
			else
				wxLogMessage(_("UKTides:: station bitmap has NOT been loaded"));	

			m_pDialog->b_clearAllIcons = false;
			m_pDialog->b_clearSavedIcons = false;
						
      }

	  m_pDialog->Fit();
	  //Toggle 
	  m_bShowUKTides = !m_bShowUKTides;	  

      //    Toggle dialog? 
      if(m_bShowUKTides) {		  
          m_pDialog->Show();
		  m_pDialog->b_clearAllIcons = false;
		  m_pDialog->b_clearSavedIcons = false;

	  }
	  else {		 
		  m_pDialog->Hide();
		  m_pDialog->b_clearAllIcons = true;
		  m_pDialog->b_clearSavedIcons = true;
	  }
      // Toggle is handled by the toolbar but we must keep plugin manager b_toggle updated
      // to actual status to ensure correct status upon toolbar rebuild
      SetToolbarItemState( m_leftclick_tool_id, m_bShowUKTides );

      RequestRefresh(m_parent_window); // refresh main window
}

void UKTides_pi::OnUKTidesDialogClose()
{
	m_pDialog->b_clearSavedIcons = true;
	m_pDialog->b_clearAllIcons = true;
	m_bShowUKTides = false;
    SetToolbarItemState( m_leftclick_tool_id, m_bShowUKTides );
    m_pDialog->Hide();
    SaveConfig();

    RequestRefresh(m_parent_window); // refresh main window

}


bool UKTides_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T( "/Settings/UKTides_pi" ) );
			 pConf->Read ( _T( "ShowUKTidesIcon" ), &m_bUKTidesShowIcon, 1 );
           
            m_route_dialog_x =  pConf->Read ( _T ( "DialogPosX" ), 20L );
            m_route_dialog_y =  pConf->Read ( _T ( "DialogPosY" ), 20L );
         
            if((m_route_dialog_x < 0) || (m_route_dialog_x > m_display_width))
                  m_route_dialog_x = 5;
            if((m_route_dialog_y < 0) || (m_route_dialog_y > m_display_height))
                  m_route_dialog_y = 5;
            return true;
      }
      else
            return false;
}

bool UKTides_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath ( _T ( "/Settings/UKTides_pi" ) );
			pConf->Write ( _T ( "ShowUKTidesIcon" ), m_bUKTidesShowIcon );
          
            pConf->Write ( _T ( "DialogPosX" ),   m_route_dialog_x );
            pConf->Write ( _T ( "DialogPosY" ),   m_route_dialog_y );
            
            return true;
      }
      else
            return false;
}

bool UKTides_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
	if (!m_pDialog)
		return false;

	piDC pidc(dc);
	m_pDialog->RenderOverlay(pidc, *vp);
	return true;
}

bool UKTides_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
	if (!m_pDialog) 
		return false;

	//m_pDialog->SetViewPort(vp);
	piDC piDC;
    glEnable( GL_BLEND );
    piDC.SetVP(vp);

	m_pDialog->RenderOverlay(piDC, *vp);
	return true;
}


void UKTides_pi::OnContextMenuItemCallback(int id)
{
	if (!m_pDialog)
		return;
	
	if (id == m_position_menu_id) {
		m_cursor_lat = GetCursorLat();
		m_cursor_lon = GetCursorLon();
		m_pDialog->getPort(m_cursor_lat, m_cursor_lon);
	}	
}

void UKTides_pi::SetCursorLatLon(double lat, double lon)
{
	m_cursor_lat = lat;
	m_cursor_lon = lon;
}
