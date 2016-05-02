#pragma once

#include <vector>

#include <wsn/graph/voronoi/VoronoiSite.h>
#include <wsn/graph/voronoi/VoronoiEdge.h>

namespace voronoi {

    class VoronoiCell {
    public:
        VoronoiCell(VoronoiSite *site);

        VoronoiSite *site;
        std::vector<VoronoiEdge *> edges;
    };

} //end namespace voronoi
