// ImageTracerDotNet.h

#pragma once


#include "ImageTracer.h"


namespace ImageTracerDotNet
{

	// Thin CLI wrapper for C++ ImageTracer


	public ref class TraceException 
		: Exception
	{
	public:
		TraceException(String^ msg, ...array<Object^>^ args);
		TraceException(ImageTracer::TraceException& exc);
	};


	public ref class Segment
	{
	public:
		enum class Type { Line, QuadSpline };

		Segment(ImageTracer::Segment& seg);

		Type type;
		float x1, y1, x2, y2, x3, y3;
	};


	public ref class Segments 
		: public List<Segment^>
	{
	public:
		Segments(ImageTracer::SegmentList& list);
	};


	public ref class Poly
	{
	public:
		property ReadOnlyCollection<Segment^>^ Segments { 
			ReadOnlyCollection<Segment^>^ get(); 
		}

		Poly(ImageTracer::Poly& poly);

	private:
		::ImageTracerDotNet::Segments^ _segments;
	};


	public ref class Polys 
		: public List<Poly^>
	{
	public:
		Polys(ImageTracer::PolyList& list);
	};

	
	public ref class Layer
	{
	public:
		property ReadOnlyCollection<Poly^>^ Polygons { 
			ReadOnlyCollection<Poly^>^ get();
		}

		property int ColorIndex { 
			int get(); 
		}

		Layer(ImageTracer::Layer& layer);

	private:
		Polys^ _polys;
		int    _color_index;
	};


	public ref class Options
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


	public ref class ImageTracerDotNet
	{
	public:

		static ImageTracerDotNet^ Trace(array<byte,2>^ pixels, Options^ options);
		static ImageTracerDotNet^ Trace(array<byte>^ pixels, int width, int height, Options^ options);

		property ReadOnlyCollection<Layer^>^ Layers { 
			ReadOnlyCollection<Layer^>^ get();
		};


	private:

		ImageTracerDotNet();

		void _Trace(array<byte>^ pixels, int width, int height, Options^ options);

		// Polys traced
		List<Layer^>^ _layers;

	};

}
