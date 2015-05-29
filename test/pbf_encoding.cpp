#include "catch.hpp"

// test utils
#include "test_utils.hpp"
#include "vector_tile_projection.hpp"

// vector output api
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile_datasource.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "pbf_reader.hpp"

#include <mapnik/util/fs.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/image_util.hpp>

#include <sstream>
#include <fstream>

TEST_CASE( "basic pbf rendering using pbf.hpp", "time how long it takes to render a big PBF using pbf.hpp" ) {
    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINDIR);
    unsigned _x=20,_y=20,_z=6;
    double minx,miny,maxx,maxy;
    mapnik::vector_tile_impl::spherical_mercator merc(512);
    merc.xyz(_x,_y,_z,minx,miny,maxx,maxy);
    mapnik::box2d<double> bbox;
    bbox.init(minx,miny,maxx,maxy);
    unsigned tile_size = 512;
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    map.set_buffer_size(256);
    mapnik::layer lyr("layer",map.srs());

    std::ifstream in("test/data/5_16_11.pbf");
    std::string pbuf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // Now construct a pbf.hpp reader
    mapbox::util::pbf pbf_tile(pbuf.c_str(), pbuf.size());
    // First message should be the layer created above
    pbf_tile.next();
    mapbox::util::pbf pbf_layer { pbf_tile.get_message() };
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds
        = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(pbf_layer,0,0,0,tile_size);

    //mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

    lyr.set_datasource(ds);
    map.add_layer(lyr);

    mapnik::load_map(map,"test/data/style.xml");
    map.zoom_to_box(bbox);

    mapnik::image_rgba8 im(map.width(),map.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map,im);
    ren2.apply();
    mapnik::save_to_file(im,"test.png","png32");

}


TEST_CASE( "basic pbf rendering using Google Protobuf", "time how long it takes to render a big PBF using Google Protobuf objects" ) {
    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINDIR);
    unsigned _x=20,_y=20,_z=6;
    double minx,miny,maxx,maxy;
    mapnik::vector_tile_impl::spherical_mercator merc(512);
    merc.xyz(_x,_y,_z,minx,miny,maxx,maxy);
    mapnik::box2d<double> bbox;
    bbox.init(minx,miny,maxx,maxy);
    unsigned tile_size = 512;
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    map.set_buffer_size(256);
    mapnik::layer lyr("layer",map.srs());

    std::ifstream in("test/data/5_16_11.pbf");
    std::string pbuf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    vector_tile::Tile tile;
    tile.ParseFromString(pbuf);
    vector_tile::Tile_Layer const& layer = tile.layers(0);

    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds
        = std::make_shared<mapnik::vector_tile_impl::tile_datasource>(layer,0,0,0,tile_size);

    lyr.set_datasource(ds);
    map.add_layer(lyr);

    mapnik::load_map(map,"test/data/style.xml");
    map.zoom_to_box(bbox);

    mapnik::image_rgba8 im(map.width(),map.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map,im);
    ren2.apply();
    mapnik::save_to_file(im,"test.png","png32");

}
