// This is the main DLL file.

#include "stdafx.h"

#include "ImageTracerDotNet.h"


namespace ImageTracerDotNet
{

// Thin CLI wrapper for C++ ImageTracer


//*****************************************************************************

TraceException::TraceException(String^ msg, ...array<Object^>^ args)
	: Exception(String::Format(msg, args))
{ }

TraceException::TraceException(ImageTracer::TraceException& exc)
	: Exception(gcnew String(exc.what()))
{ }


//*****************************************************************************

Segment::Segment(ImageTracer::Segment& seg) 
{
	if (seg.type == ImageTracer::Segment::Type::Type_Line)
		type = Type::Line;
	else if (seg.type == ImageTracer::Segment::Type::Type_QuadSpline)
		type = Type::QuadSpline;
	else
		throw gcnew TraceException("Invalid type");

	x1 = seg.x1;
	y1 = seg.y1;
	x2 = seg.x2;
	y2 = seg.y2;
	x3 = seg.x3;
	y3 = seg.y3;
}


//*****************************************************************************

Segments::Segments(ImageTracer::SegmentList& list)
{
	for (auto s : list)
		Add(gcnew Segment(s));
}


//*****************************************************************************

ReadOnlyCollection<Segment^>^ Poly::Segments::get() 
{ 
	return _segments->AsReadOnly(); 
} 

Poly::Poly(ImageTracer::Poly& poly)
{
	_segments = gcnew ::ImageTracerDotNet::Segments(poly.Segments);
}


//*****************************************************************************

Polys::Polys(ImageTracer::PolyList& list)
{
	for (auto p : list)
		Add(gcnew Poly(p));
}


//*****************************************************************************

ReadOnlyCollection<Poly^>^ Layer::Polygons::get()
{
	return _polys->AsReadOnly(); 
}

int Layer::ColorIndex::get() 
{
	return _color_index; 
} 

Layer::Layer(ImageTracer::Layer& layer)
{
	_polys       = gcnew Polys(layer.Polygons);
	_color_index = layer.ColorIndex;
}


//*****************************************************************************

/*static*/ ImageTracerDotNet^ ImageTracerDotNet::Trace(array<byte,2>^ pixels, Options^ options)
{
	int width = pixels->GetLength(1);
	int height = pixels->GetLength(0);

	array<byte>^ pixels1 = gcnew array<byte>(height * width);
	pin_ptr<byte> pp_pixels = &pixels[0,0];
	pin_ptr<byte> pp_pixels1 = &pixels1[0];
	memcpy((byte*)pp_pixels1, (byte*)pp_pixels, pixels->Length);

	return Trace(pixels1, width, height, options);
}

/*static*/ ImageTracerDotNet^ ImageTracerDotNet::Trace(array<byte>^ pixels, int width, int height, Options^ options)
{
	ImageTracerDotNet^ trc = nullptr;
	try
	{
		trc = gcnew ImageTracerDotNet();
		if (trc != nullptr)
			trc->_Trace(pixels, width, height, options);
	}
	catch (Exception^ /*exc*/)
	{
		trc = nullptr;
	}
	return trc;
}

ReadOnlyCollection<Layer^>^ ImageTracerDotNet::Layers::get()
{
	return _layers->AsReadOnly(); 
} 


ImageTracerDotNet::ImageTracerDotNet()
{ }

void ImageTracerDotNet::_Trace(array<byte>^ pixels, int width, int height, Options^ options)
{
	pin_ptr<byte> pp = &pixels[0];
	byte* p = pp;

	ImageTracer::Options opt;
	opt.ltres = options->ltres;
	opt.qtres = options->qtres;
	opt.pathomit = options->pathomit;
	opt.rightangleenhance = options->rightangleenhance;

	ImageTracer::ImageTracer* trc = nullptr;
	try 
	{
		_layers = gcnew List<Layer^>();
		trc = ImageTracer::ImageTracer::Trace(p, width, height, opt);
		if (trc)
		{
			for (auto l : trc->Layers)
				_layers->Add(gcnew Layer(l));
		}
	}
	catch (ImageTracer::TraceException& exc)
	{
		throw gcnew TraceException(exc);
	}
	finally
	{
		if (trc)
			delete trc;
	}
}


};
