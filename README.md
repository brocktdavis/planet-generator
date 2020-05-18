Planet Generator and Renderer
============================
_Brock T Davis, 2020_

Originally created for CS 354: Computer Graphics with Sarah Abraham at UT Austin

## Features
- Generation of a spherical planet
- Doesn't explicity calculate biomes. Instead, renders different colors based on elevation and lattitude
- User customization through command line arguments
  - polygon count
  - ocean height, which affects percentage of world that is ocean or terrain
  - world generation seed
  - turning on off rendering of the planet, wire mesh, and convex hull
  - changing colors for the ocean, coast, terrain, and snow
  - see details by using the `-h` flag on startup

### Wishlist
- More simulations, e.g. erosion, precipation
- Using plate tectonics for elevation instead of the current Simplex Noise implementation
- Exporting the model to either an image or some asset file
- User interface for changing program parameters

## Building
_In root directory_
```
mkdir build
cd build
cmake ..
make
./bin/planets [-h]
```
## Procedure
1. Creating planet mesh
  - Generate random points on the surface of the sphere
    - Relax these points from random, which is clumpy, by replacing each point with the center of its Voronoi region
  - From these points, calculate the Voronoi tesselation
    - Get the convex hull of the points
    - For each triangle in the convex hull, project the circumcenter to the surface of the sphere. These are the vertices of the Vornoi tesselation
    - Generate groups of points, which are the polygons of the Voronoi tesselation
    - Sort each group by its angle counterclockwise from the first point in the group
  - Create regions from the Vornoi tesselation, which store simulation data about each Voronoi polygon
  - Do an elevation simulation on the regions
    - For each region, use simplex noise to get the change in elevation of the vertex from its normalized surface location
    - For each vertex, set its elevation as the average elevation from each region to which it belongs
  - Populate vertex and and index data structures for rendering
2. Rendering
  - Vertex Shader
    - Calculate the elevation as the distance from the ocean level
    - Calcualte the lattitude as the absolute value of the y-coordinate of the vertex
  - Fragment Shader
    - If the elevation of the fragment is below ocean level, normalize it to ocean height
    - Otherwise set terrain color
      - add snow color at extremes of hight lattitude and elevation (raising them to high powers so only high values get the color)
      - mix between coast and vegetation color along a sigmoid interpolation
  - Camera Controls
    - AD: Rotates the camera around the planet's axis
    - WS: Zoom the camera in and out
    - Right-Click Drag: Rotates the camera up and down longitudinally

## Code Attribution
This project uses a 3D convex hull library (<https://github.com/akuukka/quickhull>), and a Simplex noise library (<https://github.com/SRombauts/SimplexNoise>) under the [`lib/`](lib/) directory. The spherical Voronoi algorithm from SciPy is also converted. See the [credits file](CREDITS.txt) for details about these.

## License
This code is ditributed under the MIT License. See the [license file](LICENSE.txt) for details.
