// ImageTracer.h

#pragma once


#include <vector>
#include <stack>


namespace ImageTracer 
{

	// Actual tracing implementation


	class TraceException 
		: public std::exception
	{
	public:
		TraceException(const char* msg);
	};


	template<typename _Type>
	class Vector 
		: public std::vector<_Type>
	{
		typedef std::vector<_Type> _base;

	public:

		int distance(iterator start, iterator end) const
		{
			iterator::difference_type d = end - start;
			if (d < 0)
				d += (iterator::difference_type)size();
			return (int)d;
		}

		int distance(const_iterator start, const_iterator end) const
		{
			const_iterator::difference_type d = end - start;
			if (d < 0)
				d += (const_iterator::difference_type)size();
			return (int)d;
		}

		iterator next(iterator start, int n = 1) const
		{
			iterator it = start + n;
			if (it >= end())
			{
				// Needs wrapping
				const_iterator::difference_type d = it - end();
				it = begin() + d;
			}
			return it;
		}

		const_iterator next(const_iterator start, int n = 1) const
		{
			const_iterator it = start + n;
			if (it >= cend())
			{
				// Needs wrapping
				const_iterator::difference_type d = it - cend();
				it = cbegin() + d;
			}
			return it;
		}
	};


	class IntList 
		: public Vector<int>
	{ };


	class Point
	{
	public:
		float x, y;

		Point();
		Point(const float _x, const float _y);
		Point(const Point& pt);

		Point operator+(const Point& pt) const;
		Point operator-(const Point& pt) const;
		Point operator*(const Point& pt) const;
		Point operator/(const Point& pt) const;
		
		Point operator+(const float f) const;
		Point operator-(const float f) const;
		Point operator*(const float f) const;
		Point operator/(const float f) const;
	};

	class PointList 
		: public Vector<Point>
	{ };


	class Segment
	{
	public:
		enum Type { Type_Line, Type_QuadSpline };

		Type type;
		float x1, y1, x2, y2, x3, y3;

		Segment();
		Segment(const Type _type, const Point& pt1, const Point& pt2, const Point& pt3);
		Segment(const Segment& seg);

		static Segment Line(const Point& pt1, const Point& pt2);
		static Segment QuadSpline(const Point& pt1, const Point& pt2, const Point& pt3);
	};

	class SegmentList 
		: public Vector<Segment>
	{
	public:
		SegmentList& Concat(const SegmentList& segments);
	};


	class Poly
	{
	public:
		SegmentList Segments;

		Poly();
		Poly(const Poly& poly);
		Poly(const SegmentList& segments);
	};

	class PolyList 
		: public Vector<Poly> 
	{ };

	
	class Layer
	{
	public:
		PolyList Polygons;
		int      ColorIndex;

		Layer();
		Layer(const Layer& layer);
		Layer(const PolyList& polys, const int color_index);
	};

	class LayerList 
		: public Vector<Layer>
	{ };


	class BBox
	{
	public:
		int coords[4];

		BBox();
		BBox(const int l, const int t, const int r, const int b);
		BBox(const BBox& bbox);

		bool Includes(const BBox& childbbox) const;
	};

	
	class Path
	{
	public:
		PointList   points;
		IntList     linesegments;
		BBox        boundingbox;
		bool        isholepath;
	//TODO: IntList     holechildren;
		SegmentList segments;

		Path();
		Path(const Path& p);
	};

	class PathList
		: public Vector<Path>
	{ };


	class Options
	{
	public:

		// Tracing

		//Enable or disable CORS Image loading
		//corsenabled : false,
		//=> Not used

		// Error treshold for straight lines.
		float ltres = 1;

		// Error treshold for Error treshold for quadratic splines.
		float qtres = 1;

		// Edge node paths shorter than this will be discarded for noise reduction.
		int pathomit = 8;

		// Enhance right angle corners.
		bool rightangleenhance = true;


		// Color quantization
		//
		//colorsampling : 2,
		//numberofcolors : 16,
		//mincolorratio : 0,
		//colorquantcycles : 3,
		//=> Not implemented yet


		// Layering method
		//
		//layering : 0,
		//=> Only sequencial layering for now


		// SVG rendering
		//
		//strokewidth : 1,
		//linefilter : false,
		//scale : 1,
		//roundcoords : 1,
		//viewbox : false,
		//desc : false,
		//lcpr : 0,
		//qcpr : 0,
		//=> Not implemented yet, and I don't think I will


		// Blur
		//
		//blurradius : 0,
		//blurdelta : 20
		//=> Not implemented yet, might be added together with color quantization


		// Ready-made setups
		//
		//'posterized1': { colorsampling:0, numberofcolors:2 },
		//'posterized2': { numberofcolors:4, blurradius:5 },
		//'curvy': { ltres:0.01, linefilter:true, rightangleenhance:false },
		//'sharp': { qtres:0.01, linefilter:false },
		//'detailed': { pathomit:0, roundcoords:2, ltres:0.5, qtres:0.5, numberofcolors:64 },
		//'smoothed': { blurradius:5, blurdelta: 64 },
		//'grayscale': { colorsampling:0, colorquantcycles:1, numberofcolors:7 },
		//'fixedpalette': { colorsampling:0, colorquantcycles:1, numberofcolors:27 },
		//'randomsampling1': { colorsampling:1, numberofcolors:8 },
		//'randomsampling2': { colorsampling:1, numberofcolors:64 },
		//'artistic1': { colorsampling:0, colorquantcycles:1, pathomit:0, blurradius:5, blurdelta: 64, ltres:0.01, linefilter:true, numberofcolors:16, strokewidth:2 },
		//'artistic2': { qtres:0.01, colorsampling:0, colorquantcycles:1, numberofcolors:4, strokewidth:0 },
		//'artistic3': { qtres:10, ltres:10, numberofcolors:8 },
		//'artistic4': { qtres:10, ltres:10, numberofcolors:64, blurradius:5, blurdelta: 256, strokewidth:2 },
		//'posterized3': { ltres: 1, qtres: 1, pathomit: 20, rightangleenhance: true, colorsampling: 0, numberofcolors: 3,
		//	mincolorratio: 0, colorquantcycles: 3, blurradius: 3, blurdelta: 20, strokewidth: 0, linefilter: false,
		//	roundcoords: 1, pal: [ { r: 0, g: 0, b: 100, a: 255 }, { r: 255, g: 255, b: 255, a: 255 } ] }
	};


	class ImageTracer
	{
	public:
		// Actual layers traced from input
		LayerList Layers;

		// Traces color-indexed image data
		static ImageTracer* Trace(byte* pixels, const int width, const int height, const Options& options);

	private:
		ImageTracer();

		void _Trace(byte* pixels, const int width, const int height, const Options& options);

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
		void _LayeringStep(byte* pixels, const int width, const int height, const byte color_index, int* layer);

		// 3. Walking through an edge node array, discarding edge node types 0 and 15 and creating paths from the rest.
		// Walk directions (dir): 0 > ; 1 ^ ; 2 < ; 3 v 
		PathList _PathScan(int* layer, const int width, const int height, const int pathomit);

		// 4. interpollating between path points for nodes with 8 directions ( East, SouthEast, S, SW, W, NW, N, NE )
		PathList _InterNodes(const PathList& paths, const Options& options);

		bool _TestRightAngle(const Path& path, const int idx1, const int idx2, const int idx3, const int idx4, const int idx5) const;

		int _GetDirection(const Point& pt1, const Point& pt2) const;

		// 5. tracepath() : recursively trying to fit straight and quadratic spline segments on the 8 direction internode path
		//
		// 5.1. Find sequences of points with only 2 segment types
		// 5.2. Fit a straight line on the sequence
		// 5.3. If the straight line fails (distance error > ltres), find the point with the biggest error
		// 5.4. Fit a quadratic spline through errorpoint (project this to get controlpoint), then measure errors on every point in the sequence
		// 5.5. If the spline fails (distance error > qtres), find the point with the biggest error, set splitpoint = fitting point
		// 5.6. Split sequence and recursively apply 5.2. - 5.6. to startpoint-splitpoint and splitpoint-endpoint sequences
		Path _TracePath(const Path& path, const float ltres, const float qtres);

		// 5.2. - 5.6. recursively fitting a straight or quadratic line segment on this sequence of path nodes,
		// called from tracepath()
		SegmentList _FitSeq(const Path& path, const float ltres, const float qtres, PointList::const_iterator seqstart, PointList::const_iterator seqend);

		// 5. Batch tracing paths
		PathList _BatchTracePaths(const PathList& internodepaths, const float ltres, const float qtres);


		// Lookup tables for pathscan
		// pathscan_combined_lookup[ arr[py][px] ][ dir ] = [nextarrpypx, nextdir, deltapx, deltapy];
		const int _pathscan_combined_lookup[16][4][4] = {
			{{-1,-1,-1,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}},// arr[py,px]===0 is invalid
			{{ 0, 1, 0,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}, { 0, 2,-1, 0}},
			{{-1,-1,-1,-1}, {-1,-1,-1,-1}, { 0, 1, 0,-1}, { 0, 0, 1, 0}},
			{{ 0, 0, 1, 0}, {-1,-1,-1,-1}, { 0, 2,-1, 0}, {-1,-1,-1,-1}},
		
			{{-1,-1,-1,-1}, { 0, 0, 1, 0}, { 0, 3, 0, 1}, {-1,-1,-1,-1}},
			{{13, 3, 0, 1}, {13, 2,-1, 0}, { 7, 1, 0,-1}, { 7, 0, 1, 0}},
			{{-1,-1,-1,-1}, { 0, 1, 0,-1}, {-1,-1,-1,-1}, { 0, 3, 0, 1}},
			{{ 0, 3, 0, 1}, { 0, 2,-1, 0}, {-1,-1,-1,-1}, {-1,-1,-1,-1}},
		
			{{ 0, 3, 0, 1}, { 0, 2,-1, 0}, {-1,-1,-1,-1}, {-1,-1,-1,-1}},
			{{-1,-1,-1,-1}, { 0, 1, 0,-1}, {-1,-1,-1,-1}, { 0, 3, 0, 1}},
			{{11, 1, 0,-1}, {14, 0, 1, 0}, {14, 3, 0, 1}, {11, 2,-1, 0}},
			{{-1,-1,-1,-1}, { 0, 0, 1, 0}, { 0, 3, 0, 1}, {-1,-1,-1,-1}},
		
			{{ 0, 0, 1, 0}, {-1,-1,-1,-1}, { 0, 2,-1, 0}, {-1,-1,-1,-1}},
			{{-1,-1,-1,-1}, {-1,-1,-1,-1}, { 0, 1, 0,-1}, { 0, 0, 1, 0}},
			{{ 0, 1, 0,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}, { 0, 2,-1, 0}},
			{{-1,-1,-1,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}, {-1,-1,-1,-1}}// arr[py,px]===15 is invalid
		};

		// Directional lookup based on signs of point differences: dx=x1-x2, dy=y1-y2
		// With signs -1,0,+1 -> indices 0,1,2 for both x(row) and y(col)
		const int _direction_lookup[3][3] = {
			{ 1, 0, 7 }, // dx=-1 -> dy=-1 -> SE, dy=0 -> E, dy=+1 -> NE
			{ 2, 8, 6 }, // dx= 0 -> dy=-1 -> S , dy=0 -> C, dy=+1 -> N
			{ 3, 4, 5 }, // dx=+1 -> dy=-1 -> SW, dy=0 -> W, dy=+1 -> NW
		};

	};

};
