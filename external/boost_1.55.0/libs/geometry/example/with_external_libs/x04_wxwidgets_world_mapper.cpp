// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// wxWidgets World Mapper example


// #define EXAMPLE_WX_USE_GRAPHICS_CONTEXT 1

#include <fstream>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/extensions/algorithms/selected.hpp>


// wxWidgets, if these headers are NOT found, adapt include path (and lib path)

#include "wx/wx.h"
#include "wx/math.h"
#include "wx/stockitem.h"


#ifdef EXAMPLE_WX_USE_GRAPHICS_CONTEXT
#include "wx/graphics.h"
#include "wx/dcgraph.h"
#endif

typedef boost::geometry::model::d2::point_xy<double> point_2d;
typedef boost::geometry::model::multi_polygon
    <
        boost::geometry::model::polygon<point_2d>
    > country_type;

// Adapt wxWidgets points to Boost.Geometry points such that they can be used
// in e.g. transformations (see below)
BOOST_GEOMETRY_REGISTER_POINT_2D(wxPoint, int, cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_POINT_2D(wxRealPoint, double, cs::cartesian, x, y)


// wxWidgets draws using wxPoint*, so we HAVE to use that.
// Therefore have to make a wxPoint* array
// 1) compatible with Boost.Geometry
// 2) compatible with Boost.Range (required by Boost.Geometry)
// 3) compatible with std::back_inserter (required by Boost.Geometry)

// For compatible 2):
typedef std::pair<wxPoint*,wxPoint*> wxPointPointerPair;

// For compatible 1):
BOOST_GEOMETRY_REGISTER_RING(wxPointPointerPair);


// For compatible 3):
// Specialize back_insert_iterator for the wxPointPointerPair
// (has to be done within "namespace std")
namespace std
{

template <>
class back_insert_iterator<wxPointPointerPair>
    : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:

    typedef wxPointPointerPair container_type;

    explicit back_insert_iterator(wxPointPointerPair& x)
        : current(boost::begin(x))
        , end(boost::end(x))
    {}

    inline back_insert_iterator<wxPointPointerPair>&
                operator=(wxPoint const& value)
    {
        // Check if not passed beyond
        if (current != end)
        {
            *current++ = value;
        }
        return *this;
    }

    // Boiler-plate
    inline back_insert_iterator<wxPointPointerPair>& operator*()     { return *this; }
    inline back_insert_iterator<wxPointPointerPair>& operator++()    { return *this; }
    inline back_insert_iterator<wxPointPointerPair>& operator++(int) { return *this; }

private:
    boost::range_iterator<wxPointPointerPair>::type current, end;
};

} // namespace std


// ----------------------------------------------------------------------------
// Read an ASCII file containing WKT's
// ----------------------------------------------------------------------------
template <typename Geometry, typename Box>
inline void read_wkt(std::string const& filename, std::vector<Geometry>& geometries, Box& box)
{
    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            if (! line.empty())
            {
                Geometry geometry;
                boost::geometry::read_wkt(line, geometry);
                geometries.push_back(geometry);
                boost::geometry::expand(box, boost::geometry::return_envelope<Box>(geometry));
            }
        }
    }
}


// ----------------------------------------------------------------------------
class HelloWorldFrame: public wxFrame
{
public:
    HelloWorldFrame(wxFrame *frame, wxString const& title, wxPoint const& pos, wxSize const& size);

    void OnCloseWindow(wxCloseEvent& );
    void OnExit(wxCommandEvent& );

    DECLARE_EVENT_TABLE()
};


// ----------------------------------------------------------------------------
class HelloWorldCanvas: public wxWindow
{
public:
    HelloWorldCanvas(wxFrame *frame);

private:
    void DrawCountries(wxDC& dc);
    void DrawCountry(wxDC& dc, country_type const& country);

    void OnPaint(wxPaintEvent& );
    void OnMouseMove(wxMouseEvent&);

    typedef boost::geometry::strategy::transform::map_transformer
        <
            double, 2, 2,
            true, true
        > map_transformer_type;

    typedef boost::geometry::strategy::transform::inverse_transformer
        <
            double, 2, 2
        > inverse_transformer_type;

    boost::shared_ptr<map_transformer_type> m_map_transformer;
    boost::shared_ptr<inverse_transformer_type> m_inverse_transformer;

    boost::geometry::model::box<point_2d> m_box;
    std::vector<country_type> m_countries;
    int m_focus;

    wxBrush m_orange;
    wxFrame* m_owner;

DECLARE_EVENT_TABLE()
};



// ----------------------------------------------------------------------------
class HelloWorldApp: public wxApp
{
public:
    bool OnInit()
    {
        // Create the main frame window
        HelloWorldFrame *frame = new HelloWorldFrame(NULL, _T("Boost.Geometry for wxWidgets - Hello World!"), wxDefaultPosition, wxSize(640, 480));

        wxMenu *file_menu = new wxMenu;
        file_menu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT));
        wxMenuBar* menuBar = new wxMenuBar;
        menuBar->Append(file_menu, _T("&File"));
        frame->SetMenuBar(menuBar);

        int width, height;
        frame->GetClientSize(&width, &height);

        (void) new HelloWorldCanvas(frame);

        // Show the frame
        frame->Show(true);

        return true;
    }
};



// ----------------------------------------------------------------------------
HelloWorldFrame::HelloWorldFrame(wxFrame *frame, wxString const& title, wxPoint const& pos, wxSize const& size)
    : wxFrame(frame, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE )
{
    CreateStatusBar(2);
}


void HelloWorldFrame::OnExit(wxCommandEvent& )
{
    this->Destroy();
}

void HelloWorldFrame::OnCloseWindow(wxCloseEvent& )
{
    static bool destroyed = false;
    if (! destroyed)
    {
        this->Destroy();
        destroyed = true;
    }
}


// ----------------------------------------------------------------------------
HelloWorldCanvas::HelloWorldCanvas(wxFrame *frame)
    : wxWindow(frame, wxID_ANY)
    , m_owner(frame)
    , m_focus(-1)
{
    boost::geometry::assign_inverse(m_box);
    read_wkt("../data/world.wkt", m_countries, m_box);
    m_orange = wxBrush(wxColour(255, 128, 0), wxSOLID);
}



void HelloWorldCanvas::OnMouseMove(wxMouseEvent &event)
{
    namespace bg = boost::geometry;

    if (m_inverse_transformer)
    {
        // Boiler-plate wxWidgets code
        wxClientDC dc(this);
        PrepareDC(dc);
        m_owner->PrepareDC(dc);

        // Transform the point to Lon/Lat
        point_2d point;
        bg::transform(event.GetPosition(), point, *m_inverse_transformer);

        // Determine selected object
        int i = 0;
        int previous_focus = m_focus;
        m_focus = -1;
        BOOST_FOREACH(country_type const& country, m_countries)
        {
            if (bg::selected(country, point, 0))
            {
                m_focus = i;
            }
            i++;
        }

        // On change:
        if (m_focus != previous_focus)
        {
            // Undraw old focus
            if (previous_focus >= 0)
            {
                dc.SetBrush(*wxWHITE_BRUSH);
                DrawCountry(dc, m_countries[previous_focus]);
            }
            // Draw new focus
            if (m_focus >= 0)
            {
                dc.SetBrush(m_orange);
                DrawCountry(dc, m_countries[m_focus]);
            }
        }

        // Create a string and set it in the status text
        std::ostringstream out;
        out << "Position: " << point.x() << ", " << point.y();
        m_owner->SetStatusText(wxString(out.str().c_str(), wxConvUTF8));
    }
}



void HelloWorldCanvas::OnPaint(wxPaintEvent& )
{
#if defined(EXAMPLE_WX_USE_GRAPHICS_CONTEXT)
    wxPaintDC pdc(this);
    wxGCDC gdc(pdc);
    wxDC& dc = (wxDC&) gdc;
#else
    wxPaintDC dc(this);
#endif

    PrepareDC(dc);

    static bool running = false;
    if (! running)
    {
        running = true;

        // Update the transformers
        wxSize sz = dc.GetSize();
        m_map_transformer.reset(new map_transformer_type(m_box, sz.x, sz.y));
        m_inverse_transformer.reset(new inverse_transformer_type(*m_map_transformer));

        DrawCountries(dc);

        running = false;
    }
}


void HelloWorldCanvas::DrawCountries(wxDC& dc)
{
    namespace bg = boost::geometry;

    dc.SetBackground(*wxLIGHT_GREY_BRUSH);
    dc.Clear();

    BOOST_FOREACH(country_type const& country, m_countries)
    {
        DrawCountry(dc, country);
    }
    if (m_focus != -1)
    {
        dc.SetBrush(m_orange);
        DrawCountry(dc, m_countries[m_focus]);
    }
}


void HelloWorldCanvas::DrawCountry(wxDC& dc, country_type const& country)
{
    namespace bg = boost::geometry;

    BOOST_FOREACH(bg::model::polygon<point_2d> const& poly, country)
    {
        // Use only exterior ring, holes are (for the moment) ignored. This would need
        // a holey-polygon compatible wx object

        std::size_t n = boost::size(bg::exterior_ring(poly));

        boost::scoped_array<wxPoint> points(new wxPoint[n]);

        wxPointPointerPair pair = std::make_pair(points.get(), points.get() + n);
        bg::transform(bg::exterior_ring(poly), pair, *m_map_transformer);

        dc.DrawPolygon(n, points.get());
    }
}

// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(HelloWorldFrame, wxFrame)
    EVT_CLOSE(HelloWorldFrame::OnCloseWindow)
    EVT_MENU(wxID_EXIT, HelloWorldFrame::OnExit)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(HelloWorldCanvas, wxWindow)
    EVT_PAINT(HelloWorldCanvas::OnPaint)
    EVT_MOTION(HelloWorldCanvas::OnMouseMove)
END_EVENT_TABLE()


IMPLEMENT_APP(HelloWorldApp)
