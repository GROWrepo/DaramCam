#include "DaramCam.h"
#include "DaramCam.Internal.h"

#pragma intrinsic(memcpy) 

DCBitmap::DCBitmap ( unsigned _width, unsigned _height, unsigned _colorDepth )
	: byteArray ( nullptr )
{
	Resize ( _width, _height, _colorDepth );
}

DCBitmap::~DCBitmap ()
{
	if ( byteArray )
		delete [] byteArray;
	byteArray = nullptr;
}

unsigned char * DCBitmap::GetByteArray () { return byteArray; }
unsigned DCBitmap::GetWidth () { return width; }
unsigned DCBitmap::GetHeight () { return height; }
unsigned DCBitmap::GetColorDepth () { return colorDepth; }
unsigned DCBitmap::GetStride () { return stride; }

unsigned DCBitmap::GetByteArraySize ()
{
	return colorDepth == 3 ? ( ( ( stride + 3 ) / 4 ) * 4 ) * height : width * height * 4;
}

IWICBitmap * DCBitmap::ToWICBitmap ( bool useShared )
{
	IWICBitmap * bitmap;
	if ( useShared )
	{
		g_piFactory->CreateBitmapFromMemory ( width, height, colorDepth == 3 ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA,
			stride, GetByteArraySize (), GetByteArray (), &bitmap );
	}
	else
	{
		g_piFactory->CreateBitmap ( width, height, colorDepth == 3 ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA, WICBitmapCacheOnDemand, &bitmap );

		unsigned arrLen = GetByteArraySize ();
		WICRect wicRect = { 0, 0, ( int ) width, ( int ) height };
		IWICBitmapLock * bitmapLock;
		bitmap->Lock ( &wicRect, WICBitmapLockWrite, &bitmapLock );

		WICInProcPointer ptr;
		bitmapLock->GetDataPointer ( &arrLen, &ptr );
		unsigned wicStride = 0;
		bitmapLock->GetStride ( &wicStride );
		memcpy ( ptr, byteArray, stride * height );
		bitmapLock->Release ();
	}

	return bitmap;
}

void DCBitmap::Resize ( unsigned _width, unsigned _height, unsigned _colorDepth )
{
	if ( _width == width && _height == _height && colorDepth == _colorDepth ) return;
	if ( byteArray ) delete [] byteArray;
	if ( ( _width == 0 && _height == 0 ) || ( _colorDepth != 3 && _colorDepth != 4 ) ) { byteArray = nullptr; return; }

	width = _width; height = _height; colorDepth = _colorDepth;
	stride = ( _width * ( colorDepth * 8 ) + 7 ) / 8;
	byteArray = new unsigned char [ GetByteArraySize () ];
}

COLORREF DCBitmap::GetColorRef ( unsigned x, unsigned y )
{
	unsigned basePos = ( x * colorDepth ) + ( y * stride );
	return RGB ( byteArray [ basePos + 0 ], byteArray [ basePos + 1 ], byteArray [ basePos + 2 ] );
}

void DCBitmap::SetColorRef ( COLORREF colorRef, unsigned x, unsigned y )
{
	unsigned basePos = ( x * colorDepth ) + ( y * stride );
	memcpy ( byteArray + basePos, &colorRef, colorDepth );
}

void DCBitmap::CopyFrom ( HDC hDC, HBITMAP hBitmap )
{
	BITMAPINFO bmpInfo = { 0, };
	bmpInfo.bmiHeader.biSize = sizeof ( BITMAPINFOHEADER );
	GetDIBits ( hDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS );
	if ( width != bmpInfo.bmiHeader.biWidth || height != bmpInfo.bmiHeader.biHeight )
		Resize ( bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight, 3 );
	bmpInfo.bmiHeader.biBitCount = 24;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	for ( unsigned i = 0; i < height; ++i )
		GetDIBits ( hDC, hBitmap, i, 1, ( byteArray + ( ( height - 1 - i ) * stride ) ), &bmpInfo, DIB_RGB_COLORS );
}