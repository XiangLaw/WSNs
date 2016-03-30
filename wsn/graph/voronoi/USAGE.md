1. Calculate voronoi diagram
```
	std::vector<VoronoiSite*> sites;

	for (;;) {
	    Point p = ;
		sites.push_back(new VoronoiSite(p.x(), p.y()));
	}

	VoronoiDiagram *diagram = Voronoi::create(sites);

	delete diagram;
```

2. Usage Voronoi diagram

**Voronoi cells**
```
	for (std::map<VoronoiSite*, VoronoiCell*>::iterator it = diagram.cells().begin(); it != diagram.cells().end(); ++it) {
		std::pair<VoronoiSite*, VoronoiCell*> pair = *it;
		VoronoiSite* site = pair.first;
		VoronoiCell* cell = pair.second;
```

**Voronoi Edge**
```
    for (std::vector<VoronoiEdge*>::iterator edgesIt = cell->edges.begin(); edgesIt != cell->edges.end(); ++edgesIt) {
        if (edgesIt->edge().isSegment()) {
            Point p1 = edgesIt()->edge().startPoint();
            Point p2 = edgesIt()->edge().endPoint();
        } else {
            Point p = edgesIt()->edge().startPoint();
        }
    }
```
