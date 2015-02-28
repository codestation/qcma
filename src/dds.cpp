/* This file is part of the KDE project
   Copyright (C) 2003 Ignacio Castaño <castano@ludicon.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   Almost all this code is based on nVidia's DDS-loading example
   and the DevIl's source code by Denton Woods.

   Modified by codestation for inclusion in QCMA <codestation404@gmail.com>
*/

/* this code supports:
 * reading:
 *     rgb and dxt dds files
 *     cubemap dds files
 *     volume dds files -- TODO
 * writing:
 *     rgb dds files only -- TODO
 */

#include "dds.h"

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QImage>
#include <QDataStream>

#include <math.h> // sqrtf

#ifndef __USE_ISOC99
#define sqrtf(x) ((float)sqrt(x))
#endif

typedef quint32 uint;
typedef quint16 ushort;
typedef quint8 uchar;

#if !defined(MAKEFOURCC)
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		(uint(uchar(ch0)) | (uint(uchar(ch1)) << 8) | \
		(uint(uchar(ch2)) << 16) | (uint(uchar(ch3)) << 24 ))
#endif

#define HORIZONTAL 1
#define VERTICAL 2
#define CUBE_LAYOUT	HORIZONTAL

struct Color8888 {
    uchar r, g, b, a;
};

union Color565 {
    struct {
        ushort b : 5;
        ushort g : 6;
        ushort r : 5;
    } c;
    ushort u;
};

union Color1555 {
    struct {
        ushort b : 5;
        ushort g : 5;
        ushort r : 5;
        ushort a : 1;
    } c;
    ushort u;
};

union Color4444 {
    struct {
        ushort b : 4;
        ushort g : 4;
        ushort r : 4;
        ushort a : 4;
    } c;
    ushort u;
};


static const uint FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
static const uint FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
static const uint FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
static const uint FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
static const uint FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
static const uint FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
static const uint FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
static const uint FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

static const uint DDSD_CAPS = 0x00000001l;
static const uint DDSD_PIXELFORMAT = 0x00001000l;
static const uint DDSD_WIDTH = 0x00000004l;
static const uint DDSD_HEIGHT = 0x00000002l;
static const uint DDSD_PITCH = 0x00000008l;

static const uint DDSCAPS_TEXTURE = 0x00001000l;
static const uint DDSCAPS2_VOLUME = 0x00200000l;
static const uint DDSCAPS2_CUBEMAP = 0x00000200l;

static const uint DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400l;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800l;
static const uint DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000l;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000l;
static const uint DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000l;
static const uint DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000l;

static const uint DDPF_RGB = 0x00000040l;
static const uint DDPF_FOURCC = 0x00000004l;
static const uint DDPF_ALPHAPIXELS = 0x00000001l;

enum DDSType {
    DDS_A8R8G8B8 = 0,
    DDS_A1R5G5B5 = 1,
    DDS_A4R4G4B4 = 2,
    DDS_R8G8B8 = 3,
    DDS_R5G6B5 = 4,
    DDS_DXT1 = 5,
    DDS_DXT2 = 6,
    DDS_DXT3 = 7,
    DDS_DXT4 = 8,
    DDS_DXT5 = 9,
    DDS_RXGB = 10,
    DDS_ATI2 = 11,
    DDS_UNKNOWN
};


struct DDSPixelFormat {
    uint size;
    uint flags;
    uint fourcc;
    uint bitcount;
    uint rmask;
    uint gmask;
    uint bmask;
    uint amask;
};

static QDataStream & operator>> ( QDataStream & s, DDSPixelFormat & pf )
{
    s >> pf.size;
    s >> pf.flags;
    s >> pf.fourcc;
    s >> pf.bitcount;
    s >> pf.rmask;
    s >> pf.gmask;
    s >> pf.bmask;
    s >> pf.amask;
    return s;
}

struct DDSCaps {
    uint caps1;
    uint caps2;
    uint caps3;
    uint caps4;
};

static QDataStream & operator>> ( QDataStream & s, DDSCaps & caps )
{
    s >> caps.caps1;
    s >> caps.caps2;
    s >> caps.caps3;
    s >> caps.caps4;
    return s;
}

struct DDSHeader {
    uint size;
    uint flags;
    uint height;
    uint width;
    uint pitch;
    uint depth;
    uint mipmapcount;
    uint reserved[11];
    DDSPixelFormat pf;
    DDSCaps caps;
    uint notused;
};

static QDataStream & operator>> ( QDataStream & s, DDSHeader & header )
{
    s >> header.size;
    s >> header.flags;
    s >> header.height;
    s >> header.width;
    s >> header.pitch;
    s >> header.depth;
    s >> header.mipmapcount;
    for( int i = 0; i < 11; i++ ) {
        s >> header.reserved[i];
    }
    s >> header.pf;
    s >> header.caps;
    s >> header.notused;
    return s;
}

static bool IsValid( const DDSHeader & header )
{
    if( header.size != 124 ) {
        return false;
    }
    const uint required = (DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT);
    if( (header.flags & required) != required ) {
        return false;
    }
    if( header.pf.size != 32 ) {
        return false;
    }
    if( !(header.caps.caps1 & DDSCAPS_TEXTURE) ) {
        return false;
    }
    return true;
}


// Get supported type. We currently support 10 different types.
static DDSType GetType( const DDSHeader & header )
{
    if( header.pf.flags & DDPF_RGB ) {
        if( header.pf.flags & DDPF_ALPHAPIXELS ) {
            switch( header.pf.bitcount ) {
            case 16:
                return (header.pf.amask == 0x8000) ? DDS_A1R5G5B5 : DDS_A4R4G4B4;
            case 32:
                return DDS_A8R8G8B8;
            }
        } else {
            switch( header.pf.bitcount ) {
            case 16:
                return DDS_R5G6B5;
            case 24:
                return DDS_R8G8B8;
            }
        }
    } else if( header.pf.flags & DDPF_FOURCC ) {
        switch( header.pf.fourcc ) {
        case FOURCC_DXT1:
            return DDS_DXT1;
        case FOURCC_DXT2:
            return DDS_DXT2;
        case FOURCC_DXT3:
            return DDS_DXT3;
        case FOURCC_DXT4:
            return DDS_DXT4;
        case FOURCC_DXT5:
            return DDS_DXT5;
        case FOURCC_RXGB:
            return DDS_RXGB;
        case FOURCC_ATI2:
            return DDS_ATI2;
        }
    }
    return DDS_UNKNOWN;
}

static bool HasAlpha( const DDSHeader & header )
{
    return header.pf.flags & DDPF_ALPHAPIXELS;
}

static bool IsCubeMap( const DDSHeader & header )
{
    return header.caps.caps2 & DDSCAPS2_CUBEMAP;
}

static bool IsSupported( const DDSHeader & header )
{
    if( header.caps.caps2 & DDSCAPS2_VOLUME ) {
        return false;
    }
    if( GetType(header) == DDS_UNKNOWN ) {
        return false;
    }
    return true;
}

static bool LoadA8R8G8B8( QDataStream & s, const DDSHeader & header, QImage & img  )
{
    const uint w = header.width;
    const uint h = header.height;

    for( uint y = 0; y < h; y++ ) {
        QRgb * scanline = (QRgb *) img.scanLine( y );
        for( uint x = 0; x < w; x++ ) {
            uchar r, g, b, a;
            s >> b >> g >> r >> a;
            scanline[x] = qRgba(r, g, b, a);
        }
    }

    return true;
}

static bool LoadR8G8B8( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    for( uint y = 0; y < h; y++ ) {
        QRgb * scanline = (QRgb *) img.scanLine( y );
        for( uint x = 0; x < w; x++ ) {
            uchar r, g, b;
            s >> b >> g >> r;
            scanline[x] = qRgb(r, g, b);
        }
    }

    return true;
}

static bool LoadA1R5G5B5( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    for( uint y = 0; y < h; y++ ) {
        QRgb * scanline = (QRgb *) img.scanLine( y );
        for( uint x = 0; x < w; x++ ) {
            Color1555 color;
            s >> color.u;
            uchar a = (color.c.a != 0) ? 0xFF : 0;
            uchar r = (color.c.r << 3) | (color.c.r >> 2);
            uchar g = (color.c.g << 3) | (color.c.g >> 2);
            uchar b = (color.c.b << 3) | (color.c.b >> 2);
            scanline[x] = qRgba(r, g, b, a);
        }
    }

    return true;
}

static bool LoadA4R4G4B4( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    for( uint y = 0; y < h; y++ ) {
        QRgb * scanline = (QRgb *) img.scanLine( y );
        for( uint x = 0; x < w; x++ ) {
            Color4444 color;
            s >> color.u;
            uchar a = (color.c.a << 4) | color.c.a;
            uchar r = (color.c.r << 4) | color.c.r;
            uchar g = (color.c.g << 4) | color.c.g;
            uchar b = (color.c.b << 4) | color.c.b;
            scanline[x] = qRgba(r, g, b, a);
        }
    }

    return true;
}

static bool LoadR5G6B5( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    for( uint y = 0; y < h; y++ ) {
        QRgb * scanline = (QRgb *) img.scanLine( y );
        for( uint x = 0; x < w; x++ ) {
            Color565 color;
            s >> color.u;
            uchar r = (color.c.r << 3) | (color.c.r >> 2);
            uchar g = (color.c.g << 2) | (color.c.g >> 4);
            uchar b = (color.c.b << 3) | (color.c.b >> 2);
            scanline[x] = qRgb(r, g, b);
        }
    }

    return true;
}

static QDataStream & operator>> ( QDataStream & s, Color565 & c )
{
    return s >> c.u;
}


struct BlockDXT {
    Color565 col0;
    Color565 col1;
    uchar row[4];

    void GetColors( Color8888 color_array[4] ) {
        color_array[0].r = (col0.c.r << 3) | (col0.c.r >> 2);
        color_array[0].g = (col0.c.g << 2) | (col0.c.g >> 4);
        color_array[0].b = (col0.c.b << 3) | (col0.c.b >> 2);
        color_array[0].a = 0xFF;

        color_array[1].r = (col1.c.r << 3) | (col1.c.r >> 2);
        color_array[1].g = (col1.c.g << 2) | (col1.c.g >> 4);
        color_array[1].b = (col1.c.b << 3) | (col1.c.b >> 2);
        color_array[1].a = 0xFF;

        if( col0.u > col1.u ) {
            // Four-color block: derive the other two colors.
            color_array[2].r = (2 * color_array[0].r + color_array[1].r) / 3;
            color_array[2].g = (2 * color_array[0].g + color_array[1].g) / 3;
            color_array[2].b = (2 * color_array[0].b + color_array[1].b) / 3;
            color_array[2].a = 0xFF;

            color_array[3].r = (2 * color_array[1].r + color_array[0].r) / 3;
            color_array[3].g = (2 * color_array[1].g + color_array[0].g) / 3;
            color_array[3].b = (2 * color_array[1].b + color_array[0].b) / 3;
            color_array[3].a = 0xFF;
        } else {
            // Three-color block: derive the other color.
            color_array[2].r = (color_array[0].r + color_array[1].r) / 2;
            color_array[2].g = (color_array[0].g + color_array[1].g) / 2;
            color_array[2].b = (color_array[0].b + color_array[1].b) / 2;
            color_array[2].a = 0xFF;

            // Set all components to 0 to match DXT specs.
            color_array[3].r = 0x00; // color_array[2].r;
            color_array[3].g = 0x00; // color_array[2].g;
            color_array[3].b = 0x00; // color_array[2].b;
            color_array[3].a = 0x00;
        }
    }
};


static QDataStream & operator>> ( QDataStream & s, BlockDXT & c )
{
    return s >> c.col0 >> c.col1 >> c.row[0] >> c.row[1] >> c.row[2] >> c.row[3];
}

struct BlockDXTAlphaExplicit {
    ushort row[4];
};

static QDataStream & operator>> ( QDataStream & s, BlockDXTAlphaExplicit & c )
{
    return s >> c.row[0] >> c.row[1] >> c.row[2] >> c.row[3];
}

struct BlockDXTAlphaLinear {
    uchar alpha0;
    uchar alpha1;
    uchar bits[6];

    void GetAlphas( uchar alpha_array[8] ) {
        alpha_array[0] = alpha0;
        alpha_array[1] = alpha1;

        // 8-alpha or 6-alpha block?
        if( alpha_array[0] > alpha_array[1] ) {
            // 8-alpha block:  derive the other 6 alphas.
            // 000 = alpha_0, 001 = alpha_1, others are interpolated

            alpha_array[2] = ( 6 * alpha0 +     alpha1) / 7;	// bit code 010
            alpha_array[3] = ( 5 * alpha0 + 2 * alpha1) / 7;	// Bit code 011
            alpha_array[4] = ( 4 * alpha0 + 3 * alpha1) / 7;	// Bit code 100
            alpha_array[5] = ( 3 * alpha0 + 4 * alpha1) / 7;	// Bit code 101
            alpha_array[6] = ( 2 * alpha0 + 5 * alpha1) / 7;	// Bit code 110
            alpha_array[7] = (     alpha0 + 6 * alpha1) / 7;	// Bit code 111
        } else {
            // 6-alpha block:  derive the other alphas.
            // 000 = alpha_0, 001 = alpha_1, others are interpolated

            alpha_array[2] = (4 * alpha0 +     alpha1) / 5;		// Bit code 010
            alpha_array[3] = (3 * alpha0 + 2 * alpha1) / 5;		// Bit code 011
            alpha_array[4] = (2 * alpha0 + 3 * alpha1) / 5;		// Bit code 100
            alpha_array[5] = (    alpha0 + 4 * alpha1) / 5;		// Bit code 101
            alpha_array[6] = 0x00;								// Bit code 110
            alpha_array[7] = 0xFF;								// Bit code 111
        }
    }

    void GetBits( uchar bit_array[16] ) {
        // Split 24 packed bits into 8 bytes, 3 bits at a time.
        uint b = bits[0] | bits[1] << 8 | bits[2] << 16;
        bit_array[0] = uchar(b & 0x07);
        b >>= 3;
        bit_array[1] = uchar(b & 0x07);
        b >>= 3;
        bit_array[2] = uchar(b & 0x07);
        b >>= 3;
        bit_array[3] = uchar(b & 0x07);
        b >>= 3;
        bit_array[4] = uchar(b & 0x07);
        b >>= 3;
        bit_array[5] = uchar(b & 0x07);
        b >>= 3;
        bit_array[6] = uchar(b & 0x07);
        b >>= 3;
        bit_array[7] = uchar(b & 0x07);

        b = bits[3] | bits[4] << 8 | bits[5] << 16;
        bit_array[8] = uchar(b & 0x07);
        b >>= 3;
        bit_array[9] = uchar(b & 0x07);
        b >>= 3;
        bit_array[10] = uchar(b & 0x07);
        b >>= 3;
        bit_array[11] = uchar(b & 0x07);
        b >>= 3;
        bit_array[12] = uchar(b & 0x07);
        b >>= 3;
        bit_array[13] = uchar(b & 0x07);
        b >>= 3;
        bit_array[14] = uchar(b & 0x07);
        b >>= 3;
        bit_array[15] = uchar(b & 0x07);
    }
};

static QDataStream & operator>> ( QDataStream & s, BlockDXTAlphaLinear & c )
{
    s >> c.alpha0 >> c.alpha1;
    return s >> c.bits[0] >> c.bits[1] >> c.bits[2] >> c.bits[3] >> c.bits[4] >> c.bits[5];
}

static bool LoadDXT1( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    BlockDXT block;
    QRgb * scanline[4];

    for( uint y = 0; y < h; y += 4 ) {
        for( uint j = 0; j < 4; j++ ) {
            scanline[j] = (QRgb *) img.scanLine( y + j );
        }
        for( uint x = 0; x < w; x += 4 ) {

            // Read 64bit color block.
            s >> block;

            // Decode color block.
            Color8888 color_array[4];
            block.GetColors(color_array);

            // bit masks = 00000011, 00001100, 00110000, 11000000
            const uint masks[4] = { 3, 3<<2, 3<<4, 3<<6 };
            const int shift[4] = { 0, 2, 4, 6 };

            // Write color block.
            for( uint j = 0; j < 4; j++ ) {
                for( uint i = 0; i < 4; i++ ) {
                    if( img.valid( x+i, y+j ) ) {
                        uint idx = (block.row[j] & masks[i]) >> shift[i];
                        scanline[j][x+i] = qRgba(color_array[idx].r, color_array[idx].g, color_array[idx].b, color_array[idx].a);
                    }
                }
            }
        }
    }
    return true;
}

static bool LoadDXT3( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    BlockDXT block;
    BlockDXTAlphaExplicit alpha;
    QRgb * scanline[4];

    for( uint y = 0; y < h; y += 4 ) {
        for( uint j = 0; j < 4; j++ ) {
            scanline[j] = (QRgb *) img.scanLine( y + j );
        }
        for( uint x = 0; x < w; x += 4 ) {

            // Read 128bit color block.
            s >> alpha;
            s >> block;

            // Decode color block.
            Color8888 color_array[4];
            block.GetColors(color_array);

            // bit masks = 00000011, 00001100, 00110000, 11000000
            const uint masks[4] = { 3, 3<<2, 3<<4, 3<<6 };
            const int shift[4] = { 0, 2, 4, 6 };

            // Write color block.
            for( uint j = 0; j < 4; j++ ) {
                ushort a = alpha.row[j];
                for( uint i = 0; i < 4; i++ ) {
                    if( img.valid( x+i, y+j ) ) {
                        uint idx = (block.row[j] & masks[i]) >> shift[i];
                        color_array[idx].a = a & 0x0f;
                        color_array[idx].a = color_array[idx].a | (color_array[idx].a << 4);
                        scanline[j][x+i] = qRgba(color_array[idx].r, color_array[idx].g, color_array[idx].b, color_array[idx].a);
                    }
                    a >>= 4;
                }
            }
        }
    }
    return true;
}

static bool LoadDXT2( QDataStream & s, const DDSHeader & header, QImage & img )
{
    if( !LoadDXT3(s, header, img) ) {
        return false;
    }
    //UndoPremultiplyAlpha(img);
    return true;
}

static bool LoadDXT5( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    BlockDXT block;
    BlockDXTAlphaLinear alpha;
    QRgb * scanline[4];

    for( uint y = 0; y < h; y += 4 ) {
        for( uint j = 0; j < 4; j++ ) {
            scanline[j] = (QRgb *) img.scanLine( y + j );
        }
        for( uint x = 0; x < w; x += 4 ) {

            // Read 128bit color block.
            s >> alpha;
            s >> block;

            // Decode color block.
            Color8888 color_array[4];
            block.GetColors(color_array);

            uchar alpha_array[8];
            alpha.GetAlphas(alpha_array);

            uchar bit_array[16];
            alpha.GetBits(bit_array);

            // bit masks = 00000011, 00001100, 00110000, 11000000
            const uint masks[4] = { 3, 3<<2, 3<<4, 3<<6 };
            const int shift[4] = { 0, 2, 4, 6 };

            // Write color block.
            for( uint j = 0; j < 4; j++ ) {
                for( uint i = 0; i < 4; i++ ) {
                    if( img.valid( x+i, y+j ) ) {
                        uint idx = (block.row[j] & masks[i]) >> shift[i];
                        color_array[idx].a = alpha_array[bit_array[j*4+i]];
                        scanline[j][x+i] = qRgba(color_array[idx].r, color_array[idx].g, color_array[idx].b, color_array[idx].a);
                    }
                }
            }
        }
    }

    return true;
}
static bool LoadDXT4( QDataStream & s, const DDSHeader & header, QImage & img )
{
    if( !LoadDXT5(s, header, img) ) {
        return false;
    }
    //UndoPremultiplyAlpha(img);
    return true;
}

static bool LoadRXGB( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    BlockDXT block;
    BlockDXTAlphaLinear alpha;
    QRgb * scanline[4];

    for( uint y = 0; y < h; y += 4 ) {
        for( uint j = 0; j < 4; j++ ) {
            scanline[j] = (QRgb *) img.scanLine( y + j );
        }
        for( uint x = 0; x < w; x += 4 ) {

            // Read 128bit color block.
            s >> alpha;
            s >> block;

            // Decode color block.
            Color8888 color_array[4];
            block.GetColors(color_array);

            uchar alpha_array[8];
            alpha.GetAlphas(alpha_array);

            uchar bit_array[16];
            alpha.GetBits(bit_array);

            // bit masks = 00000011, 00001100, 00110000, 11000000
            const uint masks[4] = { 3, 3<<2, 3<<4, 3<<6 };
            const int shift[4] = { 0, 2, 4, 6 };

            // Write color block.
            for( uint j = 0; j < 4; j++ ) {
                for( uint i = 0; i < 4; i++ ) {
                    if( img.valid( x+i, y+j ) ) {
                        uint idx = (block.row[j] & masks[i]) >> shift[i];
                        color_array[idx].a = alpha_array[bit_array[j*4+i]];
                        scanline[j][x+i] = qRgb(color_array[idx].a, color_array[idx].g, color_array[idx].b);
                    }
                }
            }
        }
    }

    return true;
}

static bool LoadATI2( QDataStream & s, const DDSHeader & header, QImage & img )
{
    const uint w = header.width;
    const uint h = header.height;

    BlockDXTAlphaLinear xblock;
    BlockDXTAlphaLinear yblock;
    QRgb * scanline[4];

    for( uint y = 0; y < h; y += 4 ) {
        for( uint j = 0; j < 4; j++ ) {
            scanline[j] = (QRgb *) img.scanLine( y + j );
        }
        for( uint x = 0; x < w; x += 4 ) {

            // Read 128bit color block.
            s >> xblock;
            s >> yblock;

            // Decode color block.
            uchar xblock_array[8];
            xblock.GetAlphas(xblock_array);

            uchar xbit_array[16];
            xblock.GetBits(xbit_array);

            uchar yblock_array[8];
            yblock.GetAlphas(yblock_array);

            uchar ybit_array[16];
            yblock.GetBits(ybit_array);

            // Write color block.
            for( uint j = 0; j < 4; j++ ) {
                for( uint i = 0; i < 4; i++ ) {
                    if( img.valid( x+i, y+j ) ) {
                        const uchar nx = xblock_array[xbit_array[j*4+i]];
                        const uchar ny = yblock_array[ybit_array[j*4+i]];

                        const float fx = float(nx) / 127.5f - 1.0f;
                        const float fy = float(ny) / 127.5f - 1.0f;
                        const float fz = sqrtf(1.0f - fx*fx - fy*fy);
                        const uchar nz = uchar((fz + 1.0f) * 127.5f);

                        scanline[j][x+i] = qRgb(nx, ny, nz);
                    }
                }
            }
        }
    }

    return true;
}



typedef bool (* TextureLoader)( QDataStream & s, const DDSHeader & header, QImage & img );

// Get an appropriate texture loader for the given type.
static TextureLoader GetTextureLoader( DDSType type )
{
    switch( type ) {
    case DDS_A8R8G8B8:
        return LoadA8R8G8B8;
    case DDS_A1R5G5B5:
        return LoadA1R5G5B5;
    case DDS_A4R4G4B4:
        return LoadA4R4G4B4;
    case DDS_R8G8B8:
        return LoadR8G8B8;
    case DDS_R5G6B5:
        return LoadR5G6B5;
    case DDS_DXT1:
        return LoadDXT1;
    case DDS_DXT2:
        return LoadDXT2;
    case DDS_DXT3:
        return LoadDXT3;
    case DDS_DXT4:
        return LoadDXT4;
    case DDS_DXT5:
        return LoadDXT5;
    case DDS_RXGB:
        return LoadRXGB;
    case DDS_ATI2:
        return LoadATI2;
    default:
        return NULL;
    };
}


// Load a 2d texture.
static bool LoadTexture( QDataStream & s, const DDSHeader & header, QImage & img )
{
    // Create dst image.
    img = QImage( header.width, header.height, QImage::Format_RGB32 );

    // Read image.
    DDSType type = GetType( header );

    // Enable alpha buffer for transparent or DDS images.
    if( HasAlpha( header ) || type >= DDS_DXT1 ) {
        img = img.convertToFormat( QImage::Format_ARGB32 );
    }

    TextureLoader loader = GetTextureLoader( type );
    if( loader == NULL ) {
        return false;
    }

    return loader( s, header, img );
}


static int FaceOffset( const DDSHeader & header )
{

    DDSType type = GetType( header );

    int mipmap = qMax(header.mipmapcount, 1U);
    int size = 0;
    int w = header.width;
    int h = header.height;

    if( type >= DDS_DXT1 ) {
        int multiplier = (type == DDS_DXT1) ? 8 : 16;
        do {
            int face_size = qMax(w/4,1) * qMax(h/4,1) * multiplier;
            size += face_size;
            w >>= 1;
            h >>= 1;
        } while( --mipmap );
    } else {
        int multiplier = header.pf.bitcount / 8;
        do {
            int face_size = w * h * multiplier;
            size += face_size;
            w = qMax( w>>1, 1 );
            h = qMax( h>>1, 1 );
        } while( --mipmap );
    }

    return size;
}

#if CUBE_LAYOUT == HORIZONTAL
static int face_offset[6][2] = { {2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {3, 1} };
#elif CUBE_LAYOUT == VERTICAL
static int face_offset[6][2] = { {2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {1, 3} };
#endif
static int face_flags[6] = {
    DDSCAPS2_CUBEMAP_POSITIVEX,
    DDSCAPS2_CUBEMAP_NEGATIVEX,
    DDSCAPS2_CUBEMAP_POSITIVEY,
    DDSCAPS2_CUBEMAP_NEGATIVEY,
    DDSCAPS2_CUBEMAP_POSITIVEZ,
    DDSCAPS2_CUBEMAP_NEGATIVEZ
};

// Load unwrapped cube map.
static bool LoadCubeMap( QDataStream & s, const DDSHeader & header, QImage & img )
{
    // Create dst image.
#if CUBE_LAYOUT == HORIZONTAL
    img = QImage( 4 * header.width, 3 * header.height, QImage::Format_RGB32 );
#elif CUBE_LAYOUT == VERTICAL
    img = QImage( 3 * header.width, 4 * header.height, QImage::Format_RGB32 );
#endif

    DDSType type = GetType( header );

    // Enable alpha buffer for transparent or DDS images.
    if( HasAlpha( header ) || type >= DDS_DXT1 ) {
        img = img.convertToFormat( QImage::Format_ARGB32 );
    }

    // Select texture loader.
    TextureLoader loader = GetTextureLoader( type );
    if( loader == NULL ) {
        return false;
    }

    // Clear background.
    img.fill( 0 );

    // Create face image.
    QImage face(header.width, header.height, QImage::Format_RGB32);

    int offset = s.device()->pos();
    int size = FaceOffset( header );

    for( int i = 0; i < 6; i++ ) {

        if( !(header.caps.caps2 & face_flags[i]) ) {
            // Skip face.
            continue;
        }

        // Seek device.
        s.device()->seek( offset );
        offset += size;

        // Load face from stream.
        if( !loader( s, header, face ) ) {
            return false;
        }

#if CUBE_LAYOUT == VERTICAL
        if( i == 5 ) {
            face = face.mirror(true, true);
        }
#endif

        // Compute face offsets.
        int offset_x = face_offset[i][0] * header.width;
        int offset_y = face_offset[i][1] * header.height;

        // Copy face on the image.
        for( uint y = 0; y < header.height; y++ ) {
            QRgb * src = (QRgb *) face.scanLine( y );
            QRgb * dst = (QRgb *) img.scanLine( y + offset_y ) + offset_x;
            memcpy( dst, src, sizeof(QRgb) * header.width );
        }
    }

    return true;
}

static bool canReadDDS(QIODevice *device)
{
    if (!device) {
        qWarning("DDSHandler::canRead() called with no device");
        return false;
    }

    qint64 oldPos = device->pos();

    char head[3];
    qint64 readBytes = device->read(head, sizeof(head));
    if (readBytes != sizeof(head)) {
        if (device->isSequential()) {
            while (readBytes > 0) {
                device->ungetChar(head[readBytes-- - 1]);
            }
        } else {
            device->seek(oldPos);
        }
        return false;
    }

    if (device->isSequential()) {
        while (readBytes > 0) {
            device->ungetChar(head[readBytes-- - 1]);
        }
    } else {
        device->seek(oldPos);
    }

    return qstrncmp(head, "DDS", 3) == 0;
}


bool loadDDS(const QString &filename, QImage *image)
{
    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    if(!canReadDDS(&file)) {
        return false;
    }

    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);

    // Validate header.
    uint fourcc;
    s >> fourcc;
    if( fourcc != FOURCC_DDS ) {
        qDebug("This is not a DDS file.");
        return false;
    }

    // Read image header.
    DDSHeader header;
    s >> header;

    // Check image file format.
    if( s.atEnd() || !IsValid( header ) ) {
        qDebug("This DDS file is not valid.");
        return false;
    }

    // Determine image type, by now, we only support 2d textures.
    if( !IsSupported( header ) ) {
        qDebug("This DDS file is not supported.");
        return false;
    }

    bool result;

    if( IsCubeMap( header ) ) {
        result = LoadCubeMap( s, header, *image );
    } else {
        result = LoadTexture( s, header, *image );
    }

    return result;
}
