#pragma once

#include <list>
#include "../geometry_library/geo_lib.h"

using namespace std;

// pair<distance, index>, distance = distance to source, index = index in vertices list
typedef std::pair<double, int> pair_;

class OGraph {
    std::list<pair_> *adj;  // danh sach dinh ke
    std::vector<Tangent> tagent_set_;   // danh sach dinh, moi dinh la 1 doan tiep tuyen giua 2 core polygon
    std::vector<CorePolygon> core_set_;   // tap cac core polygon de dung graph
    std::vector<int> STP;   // shortest path
    std::vector<std::vector<int>> all_paths_;
    std::vector<std::vector<BasePathPoint>> base_paths_;
    std::vector<std::vector<BasePathPoint>> all_paths_points;

    void addVertexToPath(std::vector<int> parent, int j);

    double boundaryLength(Point, Point, CorePolygon);

    double boundaryLength(Point, Point, CorePolygon, bool &);

    int getVertexIndexInPolygon(Point, std::vector<Point>);

    double polygonPerimeter(CorePolygon);

    void addVertexToPath(int u, int d, bool visited[], int path[], int &path_index);

    bool isAllAcuteAngle(std::vector<BasePathPoint> path);

public:
    OGraph(std::vector<CorePolygon> &);

    std::vector<std::vector<BasePathPoint>> basePaths(double epsilon, int n);

    void findAllPaths();

    double shortestPathLength();

    std::vector<BasePathPoint> shortestPath();

    void findAllPaths(int s, int d);

    std::vector<Point> boundaryPath(Point, Point, CorePolygon);

    // todo: chi lay nhung duong di toan chua goc nhon doi voi vector (s,t)
    void findCuteAnglePaths();
};