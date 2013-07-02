#include <boost/version.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <mapnik/raster.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/util/fs.hpp>
#include <vector>
#include <algorithm>

#include "utils.hpp"

bool compare_pixels(mapnik::image_data_32 const& im1,mapnik::image_data_32 const& im2)
{
    unsigned int width = im1.width();
    unsigned int height = im1.height();
    if ((width != im2.width()) || height != im2.height())
    {
        return false;
    }
    for (unsigned int y = 0; y < height; ++y)
    {
        const unsigned int* row_from = im1.getRow(y);
        const unsigned int* row_to = im2.getRow(y);
        for (unsigned int x = 0; x < width; ++x)
        {
           if (row_from[x] != row_to[x])
           {
               return false;
           }
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    std::vector<std::string> args;
    for (int i=1;i<argc;++i)
    {
        args.push_back(argv[i]);
    }
    bool quiet = std::find(args.begin(), args.end(), "-q")!=args.end();

    std::string should_throw;
    boost::optional<std::string> type;
    try
    {
        BOOST_TEST(set_working_dir(args));

        should_throw = "./tests/cpp_tests/data/blank.jpg";
        BOOST_TEST( mapnik::util::exists( should_throw ) );
        type = mapnik::type_from_filename(should_throw);
        BOOST_TEST( type );
        try
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(should_throw,*type));
            if (reader.get()) BOOST_TEST( false );
        }
        catch (std::exception const&)
        {
            BOOST_TEST( true );
        }

        should_throw = "./tests/cpp_tests/data/blank.png";
        BOOST_TEST( mapnik::util::exists( should_throw ) );
        type = mapnik::type_from_filename(should_throw);
        BOOST_TEST( type );
        try
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(should_throw,*type));
            if (reader.get()) BOOST_TEST( false );
        }
        catch (std::exception const&)
        {
            BOOST_TEST( true );
        }

        should_throw = "./tests/cpp_tests/data/blank.tiff";
        BOOST_TEST( mapnik::util::exists( should_throw ) );
        type = mapnik::type_from_filename(should_throw);
        BOOST_TEST( type );
        try
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(should_throw,*type));
            if (reader.get()) BOOST_TEST( false );
        }
        catch (std::exception const&)
        {
            BOOST_TEST( true );
        }

        should_throw = "./tests/data/images/xcode-CgBI.png";
        BOOST_TEST( mapnik::util::exists( should_throw ) );
        type = mapnik::type_from_filename(should_throw);
        BOOST_TEST( type );
        try
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(should_throw,*type));
            if (reader.get()) BOOST_TEST( false );
        }
        catch (std::exception const&)
        {
            BOOST_TEST( true );
        }

        // test initializing image using external pixel buffer
        mapnik::image_32 im(256,256);
        mapnik::image_32 im2(256,256,im.data().getData());
        im.set_background(mapnik::color("green"));
        // compare exact pixels
        BOOST_TEST(compare_pixels(im.data(),im2.data()));
        // compare pointers
        BOOST_TEST_EQ( im.data().getData(), im2.data().getData() );
        im2.set_background(mapnik::color("blue"));
        BOOST_TEST(compare_pixels(im.data(),im2.data()));
        BOOST_TEST_EQ( im.data().getData(), im2.data().getData() );
        mapnik::image_32 im3(256,256);
        BOOST_TEST(!compare_pixels(im.data(),im3.data()));
        BOOST_TEST_NE( im.data().getData(), im3.data().getData() );
        boost::shared_ptr<mapnik::image_32> im_ptr = boost::make_shared<mapnik::image_32>(256,256,im.data().getData());
        im.set_background(mapnik::color("red"));
        BOOST_TEST(compare_pixels(im.data(),im_ptr->data()));
        BOOST_TEST_EQ( im.data().getData(), im_ptr->data().getData() );
        // mapnik::raster
        mapnik::box2d<double> box(0,0,im_ptr->width(),im_ptr->height());
        mapnik::raster ras(im_ptr,box);
        BOOST_TEST(compare_pixels(ras.data_,im_ptr->data()));
        BOOST_TEST_EQ( ras.data_.getData(), im_ptr->data().getData() );
        mapnik::raster ras_new(box,im_ptr->width(),im_ptr->height(),false);
        BOOST_TEST(!compare_pixels(ras_new.data_,im_ptr->data()));
        BOOST_TEST_NE( ras_new.data_.getData(), im_ptr->data().getData() );

    }
    catch (std::exception const & ex)
    {
        std::clog << "C++ image i/o problem: " << ex.what() << "\n";
        BOOST_TEST(false);
    }

    if (!::boost::detail::test_errors()) {
        if (quiet) std::clog << "\x1b[1;32m.\x1b[0m";
        else std::clog << "C++ image i/o: \x1b[1;32m✓ \x1b[0m\n";
#if BOOST_VERSION >= 104600
        ::boost::detail::report_errors_remind().called_report_errors_function = true;
#endif
    } else {
        return ::boost::report_errors();
    }
}
