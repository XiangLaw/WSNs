#pragma once

#include <wsn/geometry/Point.h>

namespace voronoi {

    class VoronoiSite {
    public:
        VoronoiSite();

        VoronoiSite(const geometry::Point &position);

        VoronoiSite(double x, double y);

        const geometry::Point &position() const;

    protected:
        geometry::Point _position;
    };

} //end namespace voronoi

