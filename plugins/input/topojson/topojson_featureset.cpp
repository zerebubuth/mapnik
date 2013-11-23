/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2013 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/json/topology.hpp>
// stl
#include <string>
#include <vector>
#include <fstream>
// boost
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/sliced.hpp>
// boost.geometry
#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/simplify.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/foreach.hpp>

#include "topojson_featureset.hpp"

BOOST_GEOMETRY_REGISTER_POINT_2D(mapnik::topojson::coordinate, double, boost::geometry::cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<mapnik::topojson::coordinate>)

namespace mapnik { namespace topojson {

struct attribute_value_visitor
    :  boost::static_visitor<mapnik::value>
{
public:
    attribute_value_visitor(mapnik::transcoder const& tr)
        : tr_(tr) {}

    mapnik::value operator()(std::string const& val) const
    {
        return mapnik::value(tr_.transcode(val.c_str()));
    }

    template <typename T>
    mapnik::value operator()(T const& val) const
    {
        return mapnik::value(val);
    }

    mapnik::transcoder const& tr_;
};

template <typename T>
void assign_properties(mapnik::feature_impl & feature, T const& geom, mapnik::transcoder const& tr)
{
    if ( geom.props)
    {
        BOOST_FOREACH (property const& p,  *geom.props)
        {
            feature.put_new(boost::get<0>(p), boost::apply_visitor(attribute_value_visitor(tr),boost::get<1>(p)));
        }
    }
}

template <typename Context>
struct feature_generator : public boost::static_visitor<mapnik::feature_ptr>
{
    feature_generator(Context & ctx,  mapnik::transcoder const& tr, topology const& topo, std::size_t feature_id)
        : ctx_(ctx),
          tr_(tr),
          topo_(topo),
          feature_id_(feature_id) {}

    feature_ptr operator() (point const& pt) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));
        std::auto_ptr<geometry_type> point_ptr(new geometry_type(Point));

        double x = pt.coord.x;
        double y = pt.coord.y;
        if (topo_.tr)
        {
            x =  x * (*topo_.tr).scale_x + (*topo_.tr).translate_x;
            y =  y * (*topo_.tr).scale_y + (*topo_.tr).translate_y;
        }

        point_ptr->move_to(x,y);
        feature->paths().push_back(point_ptr.release());
        assign_properties(*feature, pt, tr_);
        return feature;
    }

    feature_ptr operator() (multi_point const& multi_pt) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));

        BOOST_FOREACH (coordinate const& pt , multi_pt.points)
        {
            std::auto_ptr<geometry_type> point_ptr(new geometry_type(Point));

            double x = pt.x;
            double y = pt.y;
            if (topo_.tr)
            {
                x =  x * (*topo_.tr).scale_x + (*topo_.tr).translate_x;
                y =  y * (*topo_.tr).scale_y + (*topo_.tr).translate_y;
            }

            point_ptr->move_to(x,y);
            feature->paths().push_back(point_ptr.release());
        }
        assign_properties(*feature, multi_pt, tr_);
        return feature;
    }

    feature_ptr operator() (linestring const& line) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));
        std::auto_ptr<geometry_type> line_ptr(new geometry_type(LineString));

        double px = 0, py = 0;
        index_type arc_index = line.ring;
        bool first = true;
        BOOST_FOREACH (coordinate const& pt , topo_.arcs[arc_index].coordinates)
        {
            double x = pt.x;
            double y = pt.y;
            if (topo_.tr)
            {
                x =  (px += x) * (*topo_.tr).scale_x + (*topo_.tr).translate_x;
                y =  (py += y) * (*topo_.tr).scale_y + (*topo_.tr).translate_y;
            }
            if (first)
            {
                first = false;
                line_ptr->move_to(x,y);
            }
            else
            {
                line_ptr->line_to(x,y);
            }
        }

        feature->paths().push_back(line_ptr.release());
        assign_properties(*feature, line, tr_);
        return feature;
    }

    feature_ptr operator() (multi_linestring const& multi_line) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));

        BOOST_FOREACH (index_type const& index , multi_line.rings)
        {
            std::auto_ptr<geometry_type> line_ptr(new geometry_type(LineString));

            double px = 0, py = 0;
            bool first = true;
            bool reverse = index < 0;
            index_type arc_index = reverse ? std::abs(index) - 1 : index;
            BOOST_FOREACH (coordinate const& pt , topo_.arcs[arc_index].coordinates)
            {
                double x = pt.x;
                double y = pt.y;
                if (topo_.tr)
                {
                    x =  (px += x) * (*topo_.tr).scale_x + (*topo_.tr).translate_x;
                    y =  (py += y) * (*topo_.tr).scale_y + (*topo_.tr).translate_y;
                }
                if (first)
                {
                    first = false;
                    line_ptr->move_to(x,y);
                }
                else
                {
                    line_ptr->line_to(x,y);
                }
            }
            feature->paths().push_back(line_ptr.release());
        }
        assign_properties(*feature, multi_line, tr_);
        return feature;
    }

    feature_ptr operator() (polygon const& poly) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));
        std::auto_ptr<geometry_type> poly_ptr(new geometry_type(Polygon));
        std::vector<mapnik::topojson::coordinate> processed_coords;

        BOOST_FOREACH (std::vector<index_type> const& ring , poly.rings)
        {
            bool first = true;
            BOOST_FOREACH (index_type
             const& index , ring)
            {
                double px = 0, py = 0;
                bool reverse = index < 0;
                index_type arc_index = reverse ? std::abs(index) - 1 : index;
                std::list<coordinate> const& coords = topo_.arcs[arc_index].coordinates;
                processed_coords.clear();
                processed_coords.reserve(coords.size());
                BOOST_FOREACH (coordinate const& pt , coords )
                {
                    double x = pt.x;
                    double y = pt.y;

                    if (topo_.tr)
                    {
                        transform const& tr = *topo_.tr;
                        x =  (px += x) * tr.scale_x + tr.translate_x;
                        y =  (py += y) * tr.scale_y + tr.translate_y;
                    }
                    coordinate coord;
                    coord.x = x;
                    coord.y = y;
                    processed_coords.push_back(coord);
                }


                // simplify
                std::vector<mapnik::topojson::coordinate> simplified(processed_coords.size());
                boost::geometry::simplify(processed_coords, simplified, 2);

                using namespace boost::adaptors;

                if (reverse)
                {
                    BOOST_FOREACH (coordinate const& c , simplified | reversed | sliced(0, simplified.size()-1))
                    {
                        if (first)
                        {
                            first = false;
                            poly_ptr->move_to(c.x,c.y);
                        }
                        else poly_ptr->line_to(c.x,c.y);
                    }
                }
                else
                {
                    BOOST_FOREACH (coordinate const& c , simplified | sliced(0, simplified.size()-1))
                    {
                        if (first)
                        {
                            first = false;
                            poly_ptr->move_to(c.x,c.y);
                        }
                        else poly_ptr->line_to(c.x,c.y);
                    }
                }
            }
            poly_ptr->close_path();
        }

        feature->paths().push_back(poly_ptr.release());
        assign_properties(*feature, poly, tr_);
        return feature;
    }

    feature_ptr operator() (multi_polygon const& multi_poly) const
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx_,feature_id_));
        std::vector<mapnik::topojson::coordinate> processed_coords;
        BOOST_FOREACH (std::vector<std::vector<index_type> > const& poly , multi_poly.polygons)
        {
            std::auto_ptr<geometry_type> poly_ptr(new geometry_type(Polygon));
            BOOST_FOREACH (std::vector<index_type> const& ring , poly)
            {
                bool first = true;
                BOOST_FOREACH (index_type const& index , ring)
                {
                    double px = 0, py = 0;
                    bool reverse = index < 0;
                    index_type arc_index = reverse ? std::abs(index) - 1 : index;
                    std::list<coordinate> const& coords = topo_.arcs[arc_index].coordinates;
                    processed_coords.clear();
                    processed_coords.reserve(coords.size());
                    BOOST_FOREACH (coordinate const& pt , coords )
                    {
                        double x = pt.x;
                        double y = pt.y;

                        if (topo_.tr)
                        {
                            x =  (px += x) * (*topo_.tr).scale_x + (*topo_.tr).translate_x;
                            y =  (py += y) * (*topo_.tr).scale_y + (*topo_.tr).translate_y;
                        }
                        coordinate coord;
                        coord.x = x;
                        coord.y = y;
                        processed_coords.push_back(coord);
                    }

                    using namespace boost::adaptors;

                    if (reverse)
                    {
                        BOOST_FOREACH (coordinate const& c , (processed_coords | reversed | sliced(0,processed_coords.size() - 1)))
                        {
                            if (first)
                            {
                                first = false;
                                poly_ptr->move_to(c.x,c.y);
                            }
                            else poly_ptr->line_to(c.x,c.y);
                        }
                    }
                    else
                    {
                        BOOST_FOREACH (coordinate const& c , processed_coords | sliced(0,processed_coords.size() - 1))
                        {
                            if (first)
                            {
                                first = false;
                                poly_ptr->move_to(c.x,c.y);
                            }
                            else poly_ptr->line_to(c.x,c.y);
                        }
                    }
                }
                poly_ptr->close_path();
            }
            feature->paths().push_back(poly_ptr.release());
        }
        assign_properties(*feature, multi_poly, tr_);
        return feature;
    }

    template<typename T>
    feature_ptr operator() (T const& ) const
    {
        return feature_ptr();
    }

    Context & ctx_;
    mapnik::transcoder const& tr_;
    topology const& topo_;
    std::size_t feature_id_;
};

}}

topojson_featureset::topojson_featureset(mapnik::topojson::topology const& topo,
                                         mapnik::transcoder const& tr,
                                         std::deque<std::size_t> const& index_array)
    : ctx_(boost::make_shared<mapnik::context_type>()),
      topo_(topo),
      tr_(tr),
      index_array_(index_array),
      index_itr_(index_array_.begin()),
      index_end_(index_array_.end()),
      feature_id_ (0) {}

topojson_featureset::~topojson_featureset() {}

mapnik::feature_ptr topojson_featureset::next()
{
    if (index_itr_ != index_end_)
    {
        std::size_t index = *index_itr_++;
        if ( index < topo_.geometries.size())
        {
            mapnik::topojson::geometry const& geom = topo_.geometries[index];
            mapnik::feature_ptr feature = boost::apply_visitor(
                mapnik::topojson::feature_generator<mapnik::context_ptr>(ctx_, tr_, topo_, feature_id_++),
                geom);
            return feature;
        }
    }

    return mapnik::feature_ptr();
}
