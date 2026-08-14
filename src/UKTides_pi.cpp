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
      :opencpn_plugin_118 (ppimgr)

		// Create the PlugIn icons
{	


          // Create the PlugIn icons
          initialize_images();
          auto icon_path = GetPluginIcon("uktides_panel_icon", PKG_NAME);
          if (icon_path.type == IconPath::Type::Svg)
            m_panel_bitmap = LoadSvgIcon(icon_path.path.c_str());
          else if (icon_path.type == IconPath::Type::Png)
            m_panel_bitmap = LoadPngIcon(icon_path.path.c_str());
          else  // icon_path.type == NotFound
            wxLogWarning("Cannot find icon for basename: %s",
                         "shipdriver_panel_icon");
          if (m_panel_bitmap.IsOk())
            wxLogDebug("UKTidesPi::, bitmap OK");
          else
            wxLogDebug("UKTidesPi::, bitmap fail");
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

	auto icon = GetPluginIcon("UKTides_pi", PKG_NAME);
      auto toggled_icon = GetPluginIcon("UKTides_pi_toggled", PKG_NAME);
      //    This PlugIn needs a toolbar icon, so request its insertion
      if (m_bShowUKTides) {
        if (icon.type == IconPath::Type::Svg)
          m_leftclick_tool_id = InsertPlugInToolSVG(
              "UKTides", icon.path, icon.path, toggled_icon.path, wxITEM_CHECK,
              "UKTides", "", nullptr, UKTIDES_TOOL_POSITION ,
              0, this);
        else if (icon.type == IconPath::Type::Png) {
          auto bitmap = LoadPngIcon(icon.path.c_str());
          m_leftclick_tool_id =
              InsertPlugInTool("", &bitmap, &bitmap, wxITEM_CHECK, "UKTides",
                               "", nullptr, UKTIDES_TOOL_POSITION , 0, this);
        }
      }
  
	wxMenu dummy_menu;
	m_position_menu_id = AddCanvasContextMenuItem

	(new wxMenuItem(&dummy_menu, -1, _("Select UK Tidal Station")), this);
	SetCanvasContextMenuItemViz(m_position_menu_id, false);

     m_pDialog = nullptr;
	
	

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

int GetPlugInVersionPatch() { return PLUGIN_VERSION_PATCH; }
int GetPlugInVersionPost() { return PLUGIN_VERSION_TWEAK; }
const char *GetPlugInVersionPre() { return PKG_PRERELEASE; }
const char *GetPlugInVersionBuild() { return PKG_BUILD_INFO; }

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
    
	if (!m_pDialog) {
    m_pDialog = new Dlg(m_parent_window);
    m_pDialog->plugin = this;
    m_pDialog->Move(wxPoint(m_route_dialog_x, m_route_dialog_y));

    // Toggle
    m_bShowUKTides = !m_bShowUKTides;

    //    Toggle dialog?
    if (m_bShowUKTides) {
      m_pDialog->Show();
      m_pDialog->b_clearAllIcons = false;
      m_pDialog->b_clearSavedIcons = false;

    } else {
      m_pDialog->Hide();
      m_pDialog->b_clearAllIcons = true;
      m_pDialog->b_clearSavedIcons = true;
    }
    // Toggle is handled by the toolbar but we must keep plugin manager b_toggle
    // updated to actual status to ensure correct status upon toolbar rebuild
    SetToolbarItemState(m_leftclick_tool_id, m_bShowUKTides);

    RequestRefresh(m_parent_window);  // refresh main window
  }
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
