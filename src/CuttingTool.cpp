// CuttingTool.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "CuttingTool.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;



void CCuttingToolParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_x_offset"), &m_x_offset, 0);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, (10 * m_diameter));
	config.Read(_T("m_orientation"), &m_orientation, 9);

	config.Read(_T("m_type"), (int *) &m_type, eDrill);
	config.Read(_T("m_flat_radius"), &m_flat_radius, 0);
	config.Read(_T("m_corner_radius"), &m_corner_radius, 0);
	config.Read(_T("m_cutting_edge_angle"), &m_cutting_edge_angle, 59);
}

void CCuttingToolParams::write_values_to_config()
{
	CNCConfig config;

	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);

	config.Write(_T("m_type"), m_type);
	config.Write(_T("m_flat_radius"), m_flat_radius);
	config.Write(_T("m_corner_radius"), m_corner_radius);
	config.Write(_T("m_cutting_edge_angle"), m_cutting_edge_angle);
}

static void on_set_diameter(double value, HeeksObj* object)
{
	((CCuttingTool*)object)->m_params.m_diameter = value;
	((CCuttingTool*)object)->m_params.m_tool_length_offset = 10 * value;

	std::wostringstream l_ossChange;

	l_ossChange << "Resetting tool length to " << ((CCuttingTool*)object)->m_params.m_tool_length_offset << " as a result of the diameter change\n";

	l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();

	wxMessageBox( wxString( l_ossChange.str().c_str() ).c_str() );
} // End on_set_diameter() routine

static void on_set_x_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_x_offset = value;}
static void on_set_tool_length_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_length_offset = value;}
static void on_set_orientation(int value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_orientation = value;}

static void on_set_type(int value, HeeksObj* object)
{
	std::wostringstream l_ossChange;

	switch(value)
	{
		case CCuttingToolParams::eDrill:
				((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);

				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 59) l_ossChange << "Changing cutting edge angle to 59 degrees (for normal 118 degree cutting face)\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 59;

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eEndmill:
				((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);

				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != ( ((CCuttingTool*)object)->m_params.m_diameter / 2) )
				{
					l_ossChange << "Changing flat radius to " << ((CCuttingTool*)object)->m_params.m_diameter / 2 << "\n";
					((CCuttingTool*)object)->m_params.m_flat_radius = ((CCuttingTool*)object)->m_params.m_diameter / 2;
				} // End if - then

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eSlotCutter:
				((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);

				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != (((CCuttingTool*)object)->m_params.m_diameter / 2)) 
				{
					l_ossChange << "Changing flat radius to " << ((CCuttingTool*)object)->m_params.m_diameter / 2 << "\n";
					((CCuttingTool*)object)->m_params.m_flat_radius = ((CCuttingTool*)object)->m_params.m_diameter / 2;
				} // End if- then

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eBallEndMill:
				((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);

				if (((CCuttingTool*)object)->m_params.m_corner_radius != (((CCuttingTool*)object)->m_params.m_diameter / 2)) 
				{
					l_ossChange << "Changing corner radius to " << (((CCuttingTool*)object)->m_params.m_diameter / 2) << "\n";
					((CCuttingTool*)object)->m_params.m_corner_radius = (((CCuttingTool*)object)->m_params.m_diameter / 2);
				} // End if - then

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eChamfer:
				((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);

				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero (this may need to be reset)\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 45) l_ossChange << "Changing cutting edge angle to 45 degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 45;

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		default:
				wxMessageBox(_T("That is not a valid cutting tool type. Aborting value change."));
				return;
	} // End switch

	if (l_ossChange.str().size() > 0)
	{
		wxMessageBox( wxString( l_ossChange.str().c_str() ).c_str() );
	} // End if - then
} // End on_set_type() method

static void on_set_corner_radius(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_corner_radius = value;}
static void on_set_flat_radius(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_flat_radius = value;}
static void on_set_cutting_edge_angle(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_cutting_edge_angle = value;}


void CCuttingToolParams::GetProperties(CCuttingTool* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
	list->push_back(new PropertyLength(_("x_offset"), m_x_offset, parent, on_set_x_offset));
	list->push_back(new PropertyLength(_("tool_length_offset"), m_tool_length_offset, parent, on_set_tool_length_offset));
	list->push_back(new PropertyInt(_("orientation"), m_orientation, parent, on_set_orientation));

	{
                std::list< wxString > choices;
                choices.push_back(_("Drill bit"));
                choices.push_back(_("End Mill"));
                choices.push_back(_("Slot Cutter"));
                choices.push_back(_("Ball End Mill"));
                choices.push_back(_("Chamfering bit"));
                int choice = int(m_type);
                list->push_back(new PropertyChoice(_("type"), choices, choice, parent, on_set_type));
        }

	list->push_back(new PropertyLength(_("flat_radius"), m_flat_radius, parent, on_set_flat_radius));
	list->push_back(new PropertyLength(_("corner_radius"), m_corner_radius, parent, on_set_corner_radius));
	list->push_back(new PropertyDouble(_("cutting_edge_angle"), m_cutting_edge_angle, parent, on_set_cutting_edge_angle));
}

void CCuttingToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  


	element->SetDoubleAttribute("diameter", m_diameter);
	element->SetDoubleAttribute("x_offset", m_x_offset);
	element->SetDoubleAttribute("tool_length_offset", m_tool_length_offset);
	
	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_orientation;
	element->SetAttribute("orientation", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << int(m_type);
	element->SetAttribute("type", l_ossValue.str().c_str() );

	element->SetDoubleAttribute("corner_radius", m_corner_radius);
	element->SetDoubleAttribute("flat_radius", m_flat_radius);
	element->SetDoubleAttribute("cutting_edge_angle", m_cutting_edge_angle);
}

void CCuttingToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
	if (pElem->Attribute("x_offset")) m_x_offset = atof(pElem->Attribute("x_offset"));
	if (pElem->Attribute("tool_length_offset")) m_tool_length_offset = atof(pElem->Attribute("tool_length_offset"));
	if (pElem->Attribute("orientation")) m_orientation = atoi(pElem->Attribute("orientation"));
	if (pElem->Attribute("type")) m_type = CCuttingToolParams::eCuttingToolType(atoi(pElem->Attribute("type")));
	if (pElem->Attribute("corner_radius")) m_corner_radius = atof(pElem->Attribute("corner_radius"));
	if (pElem->Attribute("flat_radius")) m_flat_radius = atof(pElem->Attribute("flat_radius"));
	if (pElem->Attribute("cutting_edge_angle")) m_cutting_edge_angle = atof(pElem->Attribute("cutting_edge_angle"));
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CCuttingTool::AppendTextToProgram()
{

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	// The G10 command can be used (within EMC2) to add a tool to the tool
        // table from within a program.
        // G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]
	//
	// The radius value must be expressed in MACHINE CONFIGURATION UNITS.  This may be different
	// to this model's drawing units.  The value is interpreted, at lease for EMC2, in terms
	// of the units setup for the machine's configuration (someting.ini in EMC2 parlence).  At
	// the moment we don't have a MACHINE CONFIGURATION UNITS parameter so we've got a 50%
	// chance of getting it right.

	if (m_title.size() > 0)
	{
		ss << "#(" << m_title.c_str() << ")\n";
	} // End if - then

	ss << "tool_defn( id=" << m_tool_number << ", ";

	if (m_title.size() > 0)
	{
		ss << "name='" << m_title.c_str() << "\', ";
	} // End if - then
	else
	{
		ss << "name=None, ";
	} // End if - else

	if (m_params.m_diameter > 0)
	{
		ss << "radius=" << m_params.m_diameter / 2 /theApp.m_program->m_units << ", ";
	} // End if - then
	else
	{
		ss << "radius=None, ";
	} // End if - else

	if (m_params.m_tool_length_offset > 0)
	{
		ss << "length=" << m_params.m_tool_length_offset /theApp.m_program->m_units;
	} // End if - then
	else
	{
		ss << "length=None";
	} // End if - else
	
	ss << ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


static void on_set_tool_number(const int value, HeeksObj* object){((CCuttingTool*)object)->m_tool_number = value;}

/**
	NOTE: The m_title member is a special case.  The HeeksObj code looks for a 'GetShortDesc()' method.  If found, it
	adds a Property called 'Object Title'.  If the value is changed, it tries to call the 'OnEditString()' method.
	That's why the m_title value is not defined here
 */
void CCuttingTool::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("tool_number"), m_tool_number, this, on_set_tool_number));

	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}


HeeksObj *CCuttingTool::MakeACopy(void)const
{
	return new CCuttingTool(*this);
}

void CCuttingTool::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCuttingTool*)object));
}

bool CCuttingTool::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == ToolsType;
}

void CCuttingTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CuttingTool" );
	root->LinkEndChild( element );  
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_tool_number;
	element->SetAttribute("tool_number", l_ossValue.str().c_str() );

	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingTool::ReadFromXMLElement(TiXmlElement* element)
{

	int tool_number = 0;
	if (element->Attribute("tool_number")) tool_number = atoi(element->Attribute("tool_number"));

	wxString title(Ctt(element->Attribute("title")));
	CCuttingTool* new_object = new CCuttingTool( title.c_str(), tool_number);

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


void CCuttingTool::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->WasModified(this);
}

/**
 * Find the CuttingTool object whose tool number matches that passed in.
 */
int CCuttingTool::FindCuttingTool( const int tool_number )
{
        /* Can't make this work.  Need to figure out why */
        for (HeeksObj *ob = heeksCAD->GetFirstObject(); ob != NULL; ob = heeksCAD->GetNextObject())
        {
                if (ob->GetType() != CuttingToolType) continue;

                if (((CCuttingTool *) ob)->m_tool_number == tool_number)
                {
                        return(ob->m_id);
                } // End if - then
        } // End for

        // This is a hack but it works.  As long as we don't get more than 100 tools in the holder.
        for (int id=1; id<100; id++)
        {
                HeeksObj *ob = heeksCAD->GetIDObject( CuttingToolType, id );
                if (! ob) continue;

                if (((CCuttingTool *) ob)->m_tool_number == tool_number)
                {
                        return(ob->m_id);
                } // End if - then
        } // End for

        return(-1);

} // End FindCuttingTool() method


wxString CCuttingTool::GenerateMeaningfulName() const
{
	std::wostringstream l_ossName;

	switch (m_params.m_type)
	{
		case CCuttingToolParams::eDrill:	l_ossName << m_params.m_diameter << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");
							l_ossName << "Drill Bit";
							break;

                case CCuttingToolParams::eEndmill:	l_ossName << m_params.m_diameter << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");
							l_ossName << "End Mill";
							break;

                case CCuttingToolParams::eSlotCutter:	l_ossName << m_params.m_diameter << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");
							l_ossName << "Slot Cutter";
							break;

                case CCuttingToolParams::eBallEndMill:	l_ossName << m_params.m_diameter << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");
							l_ossName << "Ball End Mill";
							break;

                case CCuttingToolParams::eChamfer:	l_ossName << m_params.m_cutting_edge_angle << " degreee ";
                					l_ossName << "Chamfering Bit";
		default:				break;
	} // End switch

	return( l_ossName.str().c_str() );
} // End GenerateMeaningfulName() method


/**
	Reset the m_title value with a meaningful name ONLY if it does not look like it was
	automatically generated in the first place.  If someone has reset it manually then leave it alone.
 */
wxString CCuttingTool::ResetTitle()
{
	std::wostringstream l_ossUnits;
	l_ossUnits << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");

	if ( (m_title == GetTypeString()) ||
	     ((m_title.Find( _T("Drill Bit") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("End Mill") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Slot Cutter") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Ball End Mill") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Chamfering Bit") ) != -1) && (m_title.Find(_T("degree")) != -1)) )
	{
		// It has the default title.  Give it a name that makes sense.
		m_title = GenerateMeaningfulName();
		heeksCAD->WasModified(this);

		std::wostringstream l_ossChange;
		l_ossChange << "Changing name to " << m_title.c_str() << "\n";
		return( l_ossChange.str().c_str() );
	} // End if - then

	// Nothing changed, nothing to report
	return(_T(""));
} // End ResetTitle() method



/**
        This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
        routines to paint the cutting tool in the graphics window.  The graphics is transient.

	We want to draw an outline of the cutting tool in 2 dimensions so that the operator
	gets a feel for what the various cutting tool parameter values mean.
 */
void CCuttingTool::glCommands(bool select, bool marked, bool no_color)
{
        if(marked && !no_color)
        {
                // Draw the outline of the bit.

		double ShaftLength = m_params.m_tool_length_offset;

		if (m_params.m_cutting_edge_angle > 0)
		{
			ShaftLength -= ((m_params.m_diameter / 2) * tan( 90 - m_params.m_cutting_edge_angle));
		} // End if - then

		glBegin(GL_LINE_STRIP);
		glVertex3d( -1 * ( m_params.m_diameter / 2), -1 * ((m_params.m_tool_length_offset / 2) - ShaftLength), 0 );
		glVertex3d( -1 * ( m_params.m_diameter / 2), +1 * (m_params.m_tool_length_offset / 2), 0 );
		glVertex3d( +1 * ( m_params.m_diameter / 2), +1 * (m_params.m_tool_length_offset / 2), 0 );
		glVertex3d( +1 * ( m_params.m_diameter / 2), -1 * ((m_params.m_tool_length_offset / 2) - ShaftLength), 0 );
		glEnd();

	/*
		// Draw the cutting edge.
		double x = 0;
		double y = -1 * (m_params.m_tool_length_offset / 2);

		if (m_params.m_flat_radius > 0)
		{
			glBegin(GL_LINE_STRIP)
			glVertex3d( -1 * m_params.m_flat_radius, y, 0 );
			glVertex3d( +1 * m_params.m_flat_radius, y, 0 );
			glEnd();

			x = m_params.m_flat_radius;
		} // End if - then

		if (m_params.m_cornder_radius)
		{
			// This is an arc from the current x position and up around 90 degrees
			// We'll cheat for a moment.

			glBegin(GL_LINE_STRIP)
			glVertex3d( -1 * x, y, 0 );
			glVertex3d( -1 * (m_params.m_diameter / 2)
			glEnd();
			
		} // End if - then

		double m_diameter;
        double m_x_offset;
        double m_tool_length_offset;
        int m_orientation;
        double m_corner_radius;
        double m_flat_radius;
        double m_cutting_edge_angle;

        eCuttingToolType        m_type;
	*/

	} // End if - then

} // End glCommands() method

