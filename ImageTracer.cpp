// ImageTracer.cpp
//

#include "stdafx.h"
#include "ImageTracer.h"

#ifdef _OPENMP
#pragma message("-> Compiling with OpenMP")
#endif


namespace ImageTracer 
{

// Actual tracing implementation


//*****************************************************************************

TraceException::TraceException(const char* msg)
	: std::exception(msg)
{ }


//*****************************************************************************

Point::Point()
{ }

Point::Point(const float _x, const float _y)
	: x(_x)
	, y(_y)
{ }

Point::Point(const Point& pt)
	: x(pt.x)
	, y(pt.y)
{ }

Point Point::operator+(const Point& pt) const { return Point(x + pt.x, y + pt.y); }
Point Point::operator-(const Point& pt) const { return Point(x - pt.x, y - pt.y); }
Point Point::operator*(const Point& pt) const { return Point(x * pt.x, y * pt.y); }
Point Point::operator/(const Point& pt) const { return Point(x / pt.x, y / pt.y); }

Point Point::operator+(const float f) const { return Point(x + f, y + f); }
Point Point::operator-(const float f) const { return Point(x - f, y - f); }
Point Point::operator*(const float f) const { return Point(x * f, y * f); }
Point Point::operator/(const float f) const { return Point(x / f, y / f); }
	

//*****************************************************************************

Segment::Segment()
{ }

Segment::Segment(const Type _type, const Point& pt1, const Point& pt2, const Point& pt3)
	: type(_type)
	, x1(pt1.x)
	, y1(pt1.y)
	, x2(pt2.x)
	, y2(pt2.y)
	, x3(pt3.x)
	, y3(pt3.y)
{ }

Segment::Segment(const Segment& seg)
	: type(seg.type)
	, x1(seg.x1)
	, y1(seg.y1)
	, x2(seg.x2)
	, y2(seg.y2)
	, x3(seg.x3)
	, y3(seg.y3)
{ }

/*static*/ Segment Segment::Line(const Point& pt1, const Point& pt2) 
{ 
	static Point dummy;
	return Segment(Type::Type_Line, pt1, pt2, dummy); 
}

/*static*/ Segment Segment::QuadSpline(const Point& pt1, const Point& pt2, const Point& pt3) 
{ 
	return Segment(Type::Type_QuadSpline, pt1, pt2, pt3); 
}

SegmentList& SegmentList::Concat(const SegmentList& segments)
{
	insert(end(), segments.begin(), segments.end());
	return *this;
}


//*****************************************************************************

Poly::Poly()
{ }

Poly::Poly(const Poly& poly)
	: Segments(poly.Segments)
{ }

Poly::Poly(const SegmentList& segments)
	: Segments(segments)
{ }


//*****************************************************************************

Layer::Layer()
{ }

Layer::Layer(const Layer& layer)
	: Polygons(layer.Polygons)
	, ColorIndex(layer.ColorIndex)
{ }

Layer::Layer(const PolyList& polys, const int color_index)
	: Polygons(polys)
	, ColorIndex(color_index)
{ }


//*****************************************************************************

BBox::BBox()
	: BBox(-1, -1, -1, -1)
{ }

BBox::BBox(const int l, const int t, const int r, const int b)
{ 
	coords[0] = l;
	coords[1] = t;
	coords[2] = r;
	coords[3] = b;
}

BBox::BBox(const BBox& bbox)
{
	memcpy(coords, bbox.coords, sizeof(coords));
}

bool BBox::Includes(const BBox& childbbox) const
{
	return ((coords[0] < childbbox.coords[0]) 
			&& (coords[1] < childbbox.coords[1]) 
			&& (coords[2] > childbbox.coords[2]) 
			&& (coords[3] > childbbox.coords[3]))
			;
}

	
//*****************************************************************************

Path::Path()
	: isholepath(false)
{ }

Path::Path(const Path& p)
	: points(p.points)
	, linesegments(p.linesegments)
	, boundingbox(p.boundingbox)
	, isholepath(p.isholepath)
//TODO: , holechildren(p.holechildren)
	, segments(p.segments)
{ }


//*****************************************************************************

#define INDEX(row,col,width) (((row)*width)+(col))

// Traces color-indexed image data
ImageTracer* ImageTracer::Trace(byte* pixels, const int width, const int height, const Options& options)
{
	ImageTracer* trc = nullptr;
	try
	{
		trc = new ImageTracer();
		if (trc)
			trc->_Trace(pixels, width, height, options);
	}
	catch (...)
	{
		if (trc)
			delete trc;
		trc = nullptr;
	}
	return trc;
}

ImageTracer::ImageTracer()
{ }

void ImageTracer::_Trace(byte* pixels, const int width, const int height, const Options& options)
{
	const int length = width * height;

	// Find min/max color indices
	byte min = 255, max = 0;
	byte* px = pixels;
	for (int i = 0; i < length; ++i, ++px)
	{
		const byte b = *px;
		if (b < min)
			min = b;
		if (b > max)
			max = b;
	}
	if (min >= max)
		throw new TraceException("Can't trace empty image");
	if (max == 255)
		throw new TraceException("Color index 255 is reserved, please adjust your input");

	// Create new buffer with a 1px border around it, border uses color index 255
	int bordered_width = width + 2;
	int bordered_height = height + 2;
	int bordered_length = bordered_width * bordered_height;
	std::auto_ptr<byte> ap_bordered_pixels(new byte[bordered_length]);
	byte* bordered_pixels = ap_bordered_pixels.get();
	px = pixels;
	byte* dest = bordered_pixels;
	memset(dest, 255, bordered_width);
	dest += bordered_width;
	for (int row = 1; row <= height; ++row)
	{
		*dest = 255;
		++dest;

		memcpy(dest, px, width);
		dest += width;
		px += width;

		*dest = 255;
		++dest;
	}
	memset(dest, 255, bordered_width);


	// Sequential layering

	// Loop over all color indices found
#pragma omp parallel for shared(options, min, max, bordered_pixels, bordered_width, bordered_height, bordered_length)
	for (int color_index = min; color_index <= max; ++color_index)
	{
		// layeringstep -> pathscan -> internodes -> batchtracepaths
		std::auto_ptr<int> ap_layer(new int[bordered_length]);
		int* layer = ap_layer.get();
		_LayeringStep(bordered_pixels, bordered_width, bordered_height, (byte)color_index, layer);

		PathList tracedlayer =
			_BatchTracePaths(							
				_InterNodes(								
					_PathScan(
						layer, 
						bordered_width, 
						bordered_height,
						options.pathomit
					),							
					options							
				),						
				options.ltres,
				options.qtres						
			);
	
		// adding traced layer
		PolyList polys;
		for (auto p : tracedlayer)
			polys.push_back(Poly(p.segments));

#	pragma omp critical
		Layers.push_back(Layer(polys, color_index));
	}

}


// 1. Color quantization
// Using a form of k-means clustering repeatead options.colorquantcycles times. http://en.wikipedia.org/wiki/Color_quantization
//=> Skipped for now

// 2. Layer separation and edge detection
//
// Edge node types ( ▓: this layer or 1; ░: not this layer or 0 )
//
// 12  ░░  ▓░  ░▓  ▓▓  ░░  ▓░  ░▓  ▓▓  ░░  ▓░  ░▓  ▓▓  ░░  ▓░  ░▓  ▓▓
//
// 48  ░░  ░░  ░░  ░░  ░▓  ░▓  ░▓  ░▓  ▓░  ▓░  ▓░  ▓░  ▓▓  ▓▓  ▓▓  ▓▓
//     0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
void ImageTracer::_LayeringStep(byte* pixels, const int width, const int height, const byte color_index, int* layer)
{
	// Creating layers for each indexed color in arr
	int length = width * height;

	// Looping through all pixels and calculating edge node type
	byte *src = pixels;
	int* dest = layer;

	memset(dest, 0, width*sizeof(int));
	dest += width;

	src += width;
	for (int j = 1; j < height; j++)
	{
		*dest = 0;
		++dest;

		++src;
		for (int i = 1; i < width; ++i, ++src, ++dest)
		{
			*dest =
				(*((src-width)-1) == color_index ? 1 : 0) +
				(*((src-width)  ) == color_index ? 2 : 0) +
				(*( src       -1) == color_index ? 8 : 0) +
				(*( src         ) == color_index ? 4 : 0)
			;
		}
	}

}

// 3. Walking through an edge node array, discarding edge node types 0 and 15 and creating paths from the rest.
// Walk directions (dir): 0 > ; 1 ^ ; 2 < ; 3 v 
PathList ImageTracer::_PathScan(int* layer, const int width, const int height, const int pathomit)
{
	PathList::iterator pa;
	int px = 0;
	int py = 0;
	int dir = 0;
	bool pathfinished = true;
	bool holepath = false;

	PathList paths;

	#define L(row,col) layer[INDEX(row,col,width)]
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			if ((L(j, i) == 4) || (L(j, i) == 11))
			{ // Other values are not valid

				// Init
				px = i;
				py = j;
				pa = paths.insert(paths.end(), Path());
				pa->boundingbox = BBox(px, py, px, py);
				pathfinished = false;
				holepath = (L(j, i) == 11);
				dir = 1;

				// Path points loop
				while (!pathfinished)
				{
					// New path point
					pa->points.push_back(Point((float)(px - 1), (float)(py - 1)));
					pa->linesegments.push_back(-1);

					// Bounding box
					if ((px - 1) < pa->boundingbox.coords[0]) { pa->boundingbox.coords[0] = px - 1; }
					if ((px - 1) > pa->boundingbox.coords[2]) { pa->boundingbox.coords[2] = px - 1; }
					if ((py - 1) < pa->boundingbox.coords[1]) { pa->boundingbox.coords[1] = py - 1; }
					if ((py - 1) > pa->boundingbox.coords[3]) { pa->boundingbox.coords[3] = py - 1; }

					// Next: look up the replacement, direction and coordinate changes = clear this cell, turn if required, walk forward
					const int *lookuprow = _pathscan_combined_lookup[L(py, px)][dir];
					L(py, px) = lookuprow[0];
					dir = lookuprow[1];
					px += lookuprow[2];
					py += lookuprow[3];

					// Close path
					if ((px - 1 == pa->points[0].x) && (py - 1 == pa->points[0].y))
					{
						pathfinished = true;

						// Discarding paths shorter than pathomit
						if (pa->points.size() < pathomit)
						{
							paths.pop_back();
						}
						else
						{
							pa->isholepath = holepath ? true : false;

							// Finding the parent shape for this hole
							if (holepath)
							{
								PathList::iterator parentidx = paths.begin();
								BBox parentbbox(-1, -1, width + 1, height + 1);
								for (PathList::iterator parent = paths.begin(); parent != pa; parent++)
								{
									if ((!parent->isholepath) &&
										parent->boundingbox.Includes(pa->boundingbox) &&
										parentbbox.Includes(pa->boundingbox)
									)
									{
										parentidx = parent;
										parentbbox = parent->boundingbox;
									}
								}

								//TODO: parentidx->holechildren.push(paths.distance(begin(), pa));

							}// End of holepath parent finding
						}

					}// End of Close path

				}// End of Path points loop

			}// End of Follow path

		}// End of i loop
	}// End of j loop

	return paths;
}

// 4. interpollating between path points for nodes with 8 directions ( East, SouthEast, S, SW, W, NW, N, NE )
PathList ImageTracer::_InterNodes(const PathList& paths, const Options& options)
{
	PathList ins;

	// paths loop
	for (PathList::const_iterator pa = paths.begin(); pa != paths.end(); pa++)
	{
		PathList::iterator n = ins.insert(ins.end(), Path());
		n->boundingbox  = pa->boundingbox;
		//TODO: n->holechildren = std::stack<int>(pa->holechildren);
		n->isholepath   = pa->isholepath;
		int palen = (int)pa->points.size();

		// pathpoints loop
		for (int pcnt = 0; pcnt < palen; pcnt++)
		{
			// next and previous point indexes
			int nextidx  = (pcnt + 1) % palen;
			int nextidx2 = (pcnt + 2) % palen;
			int previdx  = (pcnt - 1 + palen) % palen;
			int previdx2 = (pcnt - 2 + palen) % palen;

			Point pt = (pa->points[pcnt] + pa->points[nextidx]) / 2;

			// right angle enhance
			if (options.rightangleenhance && _TestRightAngle(*pa, previdx2, previdx, pcnt, nextidx, nextidx2))
			{
				// Fix previous direction
				if (n->points.size() > 0)
				{
					n->linesegments.back() = _GetDirection(
						n->points.back(),
						pa->points[pcnt]
					);
				}

				// This corner point
				n->points.push_back(pa->points[pcnt]);
				n->linesegments.push_back(_GetDirection(
					pa->points[pcnt],
					pt
				));

			}// End of right angle enhance

			// interpolate between two path points
			n->points.push_back(pt);
			n->linesegments.push_back(_GetDirection(
				pt,
				((pa->points[nextidx] + pa->points[nextidx2]) / 2)
			));

		}// End of pathpoints loop
						
	}// End of paths loop
		
	return ins;

}

bool ImageTracer::_TestRightAngle(const Path& path, const int idx1, const int idx2, const int idx3, const int idx4, const int idx5) const
{
	return (((path.points[idx3].x == path.points[idx1].x) &&
			 (path.points[idx3].x == path.points[idx2].x) &&
			 (path.points[idx3].y == path.points[idx4].y) &&
			 (path.points[idx3].y == path.points[idx5].y)
			) ||
			((path.points[idx3].y == path.points[idx1].y) &&
			 (path.points[idx3].y == path.points[idx2].y) &&
			 (path.points[idx3].x == path.points[idx4].x) &&
			 (path.points[idx3].x == path.points[idx5].x)
			)
	);
}

template <typename T> __forceinline int sign(const T val1, const T val2) {
    return (val1 < val2) - (val2 < val1);
}

int ImageTracer::_GetDirection(const Point& pt1, const Point& pt2) const
{
	const int sx = sign(pt1.x, pt2.x) + 1;
	const int sy = sign(pt1.y, pt2.y) + 1;
	return _direction_lookup[sx][sy];
}


// 5. tracepath() : recursively trying to fit straight and quadratic spline segments on the 8 direction internode path
//
// 5.1. Find sequences of points with only 2 segment types
// 5.2. Fit a straight line on the sequence
// 5.3. If the straight line fails (distance error > ltres), find the point with the biggest error
// 5.4. Fit a quadratic spline through errorpoint (project this to get controlpoint), then measure errors on every point in the sequence
// 5.5. If the spline fails (distance error > qtres), find the point with the biggest error, set splitpoint = fitting point
// 5.6. Split sequence and recursively apply 5.2. - 5.6. to startpoint-splitpoint and splitpoint-endpoint sequences
Path ImageTracer::_TracePath(const Path& path, const float ltres, const float qtres)
{
	Path smp;
	smp.boundingbox = path.boundingbox;
	//TODO: smp.holechildren = std::stack<int>(path.holechildren);
	smp.isholepath = path.isholepath;

	PointList::const_iterator p = path.points.begin();
	IntList::const_iterator line = path.linesegments.begin();
	IntList::const_iterator last = line + (path.linesegments.size() - 1);
	IntList::const_iterator seq_end;
	PointList::const_iterator p_end;
	while (line != path.linesegments.end())
	{
		// 5.1. Find sequences of points with only 2 segment types
		int segtype1 = *line;
		int segtype2 = -1;
		seq_end = line + 1;
		p_end = p + 1;

		while (
			((*seq_end == segtype1) || (*seq_end == segtype2) || (segtype2 == -1))
			&& (seq_end < last))
		{
			if ((*seq_end != segtype1) && (segtype2 == -1)) 
				segtype2 = *seq_end;
			seq_end++;
			p_end++;
		}
		if (seq_end == last) 
		{ 
			seq_end = path.linesegments.begin(); 
			p_end   = path.points.begin(); 
		}

		// 5.2. - 5.6. Split sequence and recursively apply 5.2. - 5.6. to startpoint-splitpoint and splitpoint-endpoint sequences
		/*smp.segments =*/ smp.segments.Concat(_FitSeq(path, ltres, qtres, p, p_end));

		// forward pcnt;
		if (seq_end != path.linesegments.begin()) 
		{ 
			line = seq_end; 
			p = p_end; 
		} 
		else 
			break;

	}// End of pcnt loop

	return smp;
}

// 5.2. - 5.6. recursively fitting a straight or quadratic line segment on this sequence of path nodes,
// called from tracepath()
SegmentList ImageTracer::_FitSeq(const Path& path, const float ltres, const float qtres, PointList::const_iterator seq_start, PointList::const_iterator seq_end)
{
	SegmentList segments;

	// variables
	PointList::const_iterator errorpoint = seq_start;
	float errorval = 0;
	float dist2;
	bool curvepass = true;
	float tl = (float)path.points.distance(seq_start, seq_end);

	// 5.2. Fit a straight line on the sequence
	float pl;
	Point v = (*seq_end - *seq_start) / tl;
	PointList::const_iterator p = path.points.next(seq_start);
	while (p != seq_end)
	{
		pl = (float)path.points.distance(seq_start, p);

		Point pt = *p - (*seq_start + (v * pl));
		dist2 = (pt.x * pt.x) + (pt.y * pt.y);

		if (dist2 > ltres) { curvepass = false; }
		if (dist2 > errorval) { errorpoint = p; errorval = dist2; }

		p = path.points.next(p);
	}
	// return straight line if fits
	if (curvepass)
	{
		segments.push_back(Segment::Line(*seq_start, *seq_end));
		return segments;
	}

	// 5.3. If the straight line fails (distance error>ltres), find the point with the biggest error
	PointList::const_iterator fitpoint = errorpoint;
	curvepass = true;
	errorval = 0;

	// 5.4. Fit a quadratic spline through this point, measure errors on every point in the sequence
	// helpers and projecting to get control point
	float t = path.points.distance(seq_start, fitpoint) / tl;
	float t1 = (1 - t) * (1 - t);
	float t2 = 2 * (1 - t) * t;
	float t3 = t * t;
	Point cp = ((*seq_start * t1) + (*seq_end * t3) - *fitpoint) / -t2;

	// Check every point
	p = path.points.next(seq_start);
	while (p != seq_end)
	{
		t = path.points.distance(seq_start, p) / tl;
		t1 = (1 - t) * (1 - t);
		t2 = 2 * (1 - t) * t;
		t3 = t * t;

		Point pt = *p - ((*seq_start * t1) + (cp * t2) + (*seq_end * t3));
		dist2 = (pt.x * pt.x) + (pt.y * pt.y);

		if (dist2 > qtres) { curvepass = false; }
		if (dist2 > errorval) { errorpoint = p; errorval = dist2; }

		p = path.points.next(p);
	}
	// return spline if fits
	if (curvepass)
	{
		segments.push_back(Segment::QuadSpline(*seq_start, cp, *seq_end));
		return segments;
	}

	// 5.5. If the spline fails (distance error>qtres), find the point with the biggest error
	PointList::const_iterator splitpoint = fitpoint;

	// 5.6. Split sequence and recursively apply 5.2. - 5.6. to startpoint-splitpoint and splitpoint-endpoint sequences
	return _FitSeq(path, ltres, qtres, seq_start, splitpoint).Concat(
		_FitSeq(path, ltres, qtres, splitpoint, seq_end)
	);
}

// 5. Batch tracing paths
PathList ImageTracer::_BatchTracePaths(const PathList& internodepaths, const float ltres, const float qtres)
{
	PathList btracedpaths;

	for (auto p : internodepaths)
		btracedpaths.push_back(_TracePath(p, ltres, qtres));

	return btracedpaths;
}


};
