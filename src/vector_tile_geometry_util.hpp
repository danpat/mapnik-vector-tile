#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_UTIL_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_UTIL_H__

namespace mapnik { namespace vector_tile_impl {

inline void apply_clipper(ClipperLib::Clipper & clipper,
                          mapnik::geometry::multi_polygon<double> & geom,
                          double clipper_multiplier)
{
    ClipperLib::PolyTree polygons;
    clipper.Execute(ClipperLib::ctIntersection, polygons, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    clipper.Clear();
    ClipperLib::PolyNode* polynode = polygons.GetFirst();
    geom.emplace_back();
    bool first = true;
    while (polynode)
    {
        if (!polynode->IsHole())
        {
            if (first) first = false;
            else geom.emplace_back(); // start new polygon
            for (auto const& pt : polynode->Contour)
            {
                geom.back().exterior_ring.add_coord(pt.x/clipper_multiplier, pt.y/clipper_multiplier);
            }
            // children of exterior ring are always interior rings
            for (auto const* ring : polynode->Childs)
            {
                mapnik::geometry::linear_ring<double> hole;
                for (auto const& pt : ring->Contour)
                {
                    hole.add_coord(pt.x/clipper_multiplier, pt.y/clipper_multiplier);
                }
                geom.back().add_hole(std::move(hole));
            }
        }
        polynode = polynode->GetNext();
    }
    // assert here instead of fix
    mapnik::geometry::correct(geom);
}

inline bool add_clipbox_to_clipper(ClipperLib::Clipper & clipper,
                           mapnik::box2d<double> const& bbox,
                           double clipper_multiplier)
{
    mapnik::geometry::line_string<int64_t> clip_box;
    clip_box.emplace_back(static_cast<ClipperLib::cInt>(bbox.minx()*clipper_multiplier),
          static_cast<ClipperLib::cInt>(bbox.miny()*clipper_multiplier));
    clip_box.emplace_back(static_cast<ClipperLib::cInt>(bbox.maxx()*clipper_multiplier),
          static_cast<ClipperLib::cInt>(bbox.miny()*clipper_multiplier));
    clip_box.emplace_back(static_cast<ClipperLib::cInt>(bbox.maxx()*clipper_multiplier),
          static_cast<ClipperLib::cInt>(bbox.maxy()*clipper_multiplier));
    clip_box.emplace_back(static_cast<ClipperLib::cInt>(bbox.minx()*clipper_multiplier),
          static_cast<ClipperLib::cInt>(bbox.maxy()*clipper_multiplier));
    clip_box.emplace_back(static_cast<ClipperLib::cInt>(bbox.minx()*clipper_multiplier),
          static_cast<ClipperLib::cInt>(bbox.miny()*clipper_multiplier));
    if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
    {
        //std::clog << "ptClip failed!\n";
        return false;
    }
    return true;
}

inline void add_geom_to_clipper(ClipperLib::Clipper & clipper,
                                mapnik::geometry::polygon<double> const& poly,
                                mapnik::box2d<double> const& bbox,
                                double clipper_multiplier)
{
    mapnik::box2d<double> extent = mapnik::geometry::envelope(poly);
    if (poly.exterior_ring.size() > 3 && bbox.intersects(extent))
    {
        mapnik::geometry::line_string<int64_t> path;
        path.reserve(poly.exterior_ring.size());
        for (auto const& pt : poly.exterior_ring)
        {
            double x = static_cast<ClipperLib::cInt>(pt.x*clipper_multiplier);
            double y = static_cast<ClipperLib::cInt>(pt.y*clipper_multiplier);
            path.emplace_back(x,y);
        }
        if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
        {
            //std::clog << "ptSubject ext failed, skipping encoding interior rings " << path.size() << "\n";
            return;
        }
        for (auto const& ring : poly.interior_rings)
        {
            if (ring.size() < 4)
            {
                //std::clog << "ring < 4 encountered when copying to clipper path, skipping\n";
                continue;
            }
            path.clear();
            path.reserve(ring.size());
            for (auto const& pt : ring)
            {
                double x = static_cast<ClipperLib::cInt>(pt.x*clipper_multiplier);
                double y = static_cast<ClipperLib::cInt>(pt.y*clipper_multiplier);
                path.emplace_back(x,y);
            }
            if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
            {
                //std::clog << "ptSubject hole failed! " << path.size() << " " << ring.size() << "\n";
            }
        }
    }
    else
    {
        //std::clog << "when adding to clipper, polygon with less than 4 points encountered\n";
    }
}

}} // end ns

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_UTIL_H__
