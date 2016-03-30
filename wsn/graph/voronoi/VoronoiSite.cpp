#include <wsn/graph/voronoi/VoronoiSite.h>

using namespace voronoi;
using namespace geometry;

VoronoiSite::VoronoiSite() {
}

VoronoiSite::VoronoiSite(const Point &position) : _position(position) {
}

VoronoiSite::VoronoiSite(double x, double y) : _position(x, y) {
}

const geometry::Point &VoronoiSite::position() const {
    return _position;
}
