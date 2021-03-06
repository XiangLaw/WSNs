#pragma once

#include <vector>
#include <map>
#include <set>

#include <wsn/graph/voronoi/VoronoiSite.h>
#include <wsn/graph/voronoi/VoronoiEdge.h>
#include <wsn/graph/voronoi/VoronoiCell.h>

namespace voronoi {

    class VoronoiDiagram {
    public:
        VoronoiDiagram();

        ~VoronoiDiagram();

        static VoronoiDiagram *create(std::vector<VoronoiSite *> &sites);

        std::vector<VoronoiSite *> &sites();

        std::vector<VoronoiEdge *> &edges();

        std::map<VoronoiSite *, VoronoiCell *> &cells();

        std::set<geometry::Point> &vertices();

        void initialize(std::vector<VoronoiSite *> &sites);

        void calculate();

        VoronoiEdge *createEdge(VoronoiSite *left, VoronoiSite *right);

        void addVertex(geometry::Point);

    protected:
        std::vector<VoronoiSite *> _sites;
        std::vector<VoronoiEdge *> _edges;
        std::map<VoronoiSite *, VoronoiCell *> _cells;
        std::set<geometry::Point> _vextices;

        static void removeDuplicates(std::vector<VoronoiSite *> &sites);
    };

} //end namespace voronoi

