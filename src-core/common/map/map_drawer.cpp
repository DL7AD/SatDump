#define _CRT_NO_VA_START_VALIDATION
#include "map_drawer.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "shapefile.h"

namespace map
{
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles, image::Image &map_image, std::vector<double> color, std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, int maxLength)
    {
        for (std::string currentShapeFile : shapeFiles)
        {
            nlohmann::json shapeFile;
            {
                std::ifstream istream(currentShapeFile);
                istream >> shapeFile;
                istream.close();
            }

            for (const nlohmann::json &mapStruct : shapeFile.at("features"))
            {
                if (mapStruct["type"] != "Feature")
                    continue;

                std::string geometryType = mapStruct["geometry"]["type"];

                if (geometryType == "Polygon")
                {
                    std::vector<std::vector<std::pair<double, double>>> all_coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::vector<std::pair<double, double>>>>();

                    for (std::vector<std::pair<double, double>> coordinates : all_coordinates)
                    {
                        for (int i = 0; i < (int)coordinates.size() - 1; i++)
                        {
                            std::pair<double, double> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                             map_image.height(), map_image.width());
                            std::pair<double, double> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                           map_image.height(), map_image.width());

                            if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                continue;

                            if (start.first == -1 || end.first == -1)
                                continue;

                            map_image.draw_line(start.first, start.second, end.first, end.second, color);
                        }

                        {
                            std::pair<double, double> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                             map_image.height(), map_image.width());
                            std::pair<double, double> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                           map_image.height(), map_image.width());

                            if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                continue;

                            if (start.first == -1 || end.first == -1)
                                continue;

                            map_image.draw_line(start.first, start.second, end.first, end.second, color);
                        }
                    }
                }
                else if (geometryType == "MultiPolygon")
                {
                    std::vector<std::vector<std::vector<std::pair<double, double>>>> all_all_coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::vector<std::vector<std::pair<double, double>>>>>();

                    for (std::vector<std::vector<std::pair<double, double>>> all_coordinates : all_all_coordinates)
                    {
                        for (std::vector<std::pair<double, double>> coordinates : all_coordinates)
                        {
                            for (int i = 0; i < (int)coordinates.size() - 1; i++)
                            {
                                std::pair<double, double> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                                 map_image.height(), map_image.width());
                                std::pair<double, double> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                               map_image.height(), map_image.width());

                                if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                    continue;

                                if (start.first == -1 || end.first == -1)
                                    continue;

                                map_image.draw_line(start.first, start.second, end.first, end.second, color);
                            }

                            {
                                std::pair<double, double> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                                 map_image.height(), map_image.width());

                                std::pair<double, double> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                               map_image.height(),
                                                                               map_image.width());
                                if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                    continue;

                                if (start.first == -1 || end.first == -1)
                                    continue;

                                map_image.draw_line(start.first, start.second, end.first, end.second, color);
                            }
                        }
                    }
                }
                else if (geometryType == "LineString")
                {
                    std::vector<std::pair<double, double>> coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::pair<double, double>>>();

                    for (int i = 0; i < (int)coordinates.size() - 1; i++)
                    {
                        std::pair<double, double> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                         map_image.height(), map_image.width());
                        std::pair<double, double> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                       map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }

                    {
                        std::pair<double, double> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                         map_image.height(), map_image.width());
                        std::pair<double, double> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                       map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }
                }
                else if (geometryType == "Point")
                {
                    std::pair<double, double> coordinates = mapStruct["geometry"]["coordinates"].get<std::pair<double, double>>();
                    std::pair<double, double> cc = projectionFunc(coordinates.second, coordinates.first,
                                                                  map_image.height(), map_image.width());

                    if (cc.first == -1 || cc.first == -1)
                        continue;

                    map_image.draw_pixel(cc.first, cc.second, color);
                }
            }
        }
    }

    void drawProjectedCitiesGeoJson(std::vector<std::string> shapeFiles, image::Image &map_image, image::TextDrawer &text_drawer, std::vector<double> color, std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, int font_size, int cities_type, int cities_scale_rank)
    {
        if (!text_drawer.font_ready())
            return;

        for (std::string currentShapeFile : shapeFiles)
        {
            nlohmann::json shapeFile;
            {
                std::ifstream istream(currentShapeFile);
                istream >> shapeFile;
                istream.close();
            }

            for (const nlohmann::json &mapStruct : shapeFile.at("features"))
            {
                if (mapStruct["type"] != "Feature" || mapStruct["geometry"]["type"] != "Point")
                    continue;

                std::string featurecla = mapStruct["properties"]["featurecla"].get<std::string>();

                if ((cities_type == 0 && featurecla == "Admin-0 capital") || (cities_type == 1 && (featurecla == "Admin-1 capital" || featurecla == "Admin-0 capital")) || (cities_type == 2 && mapStruct["properties"]["scalerank"] <= cities_scale_rank))
                {
                    std::pair<double, double> coordinates = mapStruct["geometry"]["coordinates"].get<std::pair<double, double>>();
                    std::pair<double, double> cc = projectionFunc(coordinates.second, coordinates.first,
                                                                  map_image.height(), map_image.width());

                    if (cc.first == -1 || cc.second == -1)
                        continue;

                    map_image.draw_line(cc.first - font_size * 0.3, cc.second - font_size * 0.3, cc.first + font_size * 0.3, cc.second + font_size * 0.3, color);
                    map_image.draw_line(cc.first + font_size * 0.3, cc.second - font_size * 0.3, cc.first - font_size * 0.3, cc.second + font_size * 0.3, color);
                    map_image.draw_circle(cc.first, cc.second, 0.15 * font_size, color, true);

                    std::string name = mapStruct["properties"]["nameascii"];
                    // map_image.draw_text(cc.first, cc.second + 20 * ratio, color, font, name);
                    text_drawer.draw_text(map_image, cc.first, cc.second + font_size * 0.15, color, font_size, name);
                }
            }
        }
    }

    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles, image::Image &map_image, std::vector<double> color, std::function<std::pair<int, int>(double, double, int, int)> projectionFunc)
    {
        for (std::string currentShapeFile : shapeFiles)
        {
            std::ifstream inputFile(currentShapeFile, std::ios::binary);
            shapefile::Shapefile shape_file(inputFile);

            std::function<void(std::vector<std::vector<shapefile::point_t>>)> polylineDraw = [color, &map_image, &projectionFunc](std::vector<std::vector<shapefile::point_t>> parts)
            {
                int width = map_image.width();
                int height = map_image.height();

                for (std::vector<shapefile::point_t> coordinates : parts)
                {
                    std::pair<double, double> start = projectionFunc(coordinates[0].y, coordinates[0].x, height, width);
                    for (int i = 1; i < (int)coordinates.size() - 1; i++)
                    {
                        std::pair<double, double> end = projectionFunc(coordinates[i].y, coordinates[i].x, height, width);
                        if (start.first != -1 && start.second != -1 && end.first != -1 && end.second != -1)
                            map_image.draw_line(start.first, start.second, end.first, end.second, color);

                        start = end;
                    }
                }
            };

            std::function<void(shapefile::point_t)> pointDraw = [color, &map_image, &projectionFunc](shapefile::point_t coordinates)
            {
                std::pair<double, double> cc = projectionFunc(coordinates.y, coordinates.x,
                                                              map_image.height(), map_image.width());

                if (cc.first == -1 || cc.second == -1)
                    return;

                map_image.draw_pixel(cc.first, cc.second, color);
            };

            for (shapefile::PolyLineRecord &polylineRecord : shape_file.polyline_records)
                polylineDraw(polylineRecord.parts_points);

            for (shapefile::PolygonRecord &polygonRecord : shape_file.polygon_records)
                polylineDraw(polygonRecord.parts_points);

            for (shapefile::PointRecord &pointRecord : shape_file.point_records)
                pointDraw(pointRecord.point);

            for (shapefile::MultiPointRecord &multipointRecord : shape_file.multipoint_records)
                for (shapefile::point_t p : multipointRecord.points)
                    pointDraw(p);
        }
    }

    void drawProjectedMapLatLonGrid(image::Image &image, std::vector<double> color, std::function<std::pair<int, int>(double, double, int, int)> projectionFunc)
    {
        for (float lon = -180; lon < 180; lon += 10)
        {
            float last_lat = -90;
            for (float lat = -90; lat < 90; lat += 0.05)
            {
                std::pair<double, double> start = projectionFunc(last_lat, lon,
                                                                 image.height(), image.width());
                std::pair<double, double> end = projectionFunc(lat, lon,
                                                               image.height(), image.width());

                if (start.first != -1 && start.second != -1 && end.first != -1 && end.second != -1)
                    image.draw_line(start.first, start.second, end.first, end.second, color);

                last_lat = lat;
            }
        }

        for (float lat = -90; lat < 90; lat += 10)
        {
            float last_lon = -180;
            for (float lon = -180; lon < 180; lon += 0.05)
            {
                std::pair<double, double> start = projectionFunc(lat, last_lon,
                                                                 image.height(), image.width());
                std::pair<double, double> end = projectionFunc(lat, lon,
                                                               image.height(), image.width());

                if (start.first != -1 && start.second != -1 && end.first != -1 && end.second != -1)
                    image.draw_line(start.first, start.second, end.first, end.second, color);

                last_lon = lon;
            }
        }
    }

    /* void drawProjectedLabels(std::vector<CustomLabel> labels, image::Image &image, image::TextDrawer &text_drawer, std::vector<double> color, std::function<std::pair<int, int>(double, double, int, int)> projectionFunc, double ratio)
     {
         if (!text_drawer.font_ready())
             return;

         for (CustomLabel &currentLabel : labels)
         {
             std::pair<double, double> cc = projectionFunc(currentLabel.lat, currentLabel.lon,
                                                           image.height(), image.width());

             if (cc.first == -1 || cc.first == -1)
                 continue;

             image.draw_line(cc.first - 20 * ratio, cc.second - 20 * ratio, cc.first + 20 * ratio, cc.second + 20 * ratio, color);
             image.draw_line(cc.first + 20 * ratio, cc.second - 20 * ratio, cc.first - 20 * ratio, cc.second + 20 * ratio, color);
             image.draw_circle(cc.first, cc.second, 10 * ratio, color, true);

            text_drawer .draw_text(image,cc.first, cc.second + 20 * ratio, color, font, currentLabel.label);
            //text_drawer.draw_text(map_image, cc.first, cc.second + font_size * 0.15, color, font_size, name);
         }
     } */
}