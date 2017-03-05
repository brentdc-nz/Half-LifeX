/*
img_jpg.c - jpg format load & save
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "imagelib.h"
#include "..\mathlib.h"

jpg_t jpg_file;		// jpeg read struct

int jpeg_read_byte( void )
{
	// read byte
	jpg_file.curbyte = *jpg_file.buffer++;
	jpg_file.curbit = 0;
	return jpg_file.curbyte;
}

int jpeg_read_word( void )
{
	// read word
	word i = (jpg_file.buffer[1]<<8)|jpg_file.buffer[0];
	i = ((i << 8) & 0xFF00) + ((i >> 8) & 0x00FF);
	jpg_file.buffer += 2;
	
	return i;
}

int jpeg_read_bit( void )
{
	// read bit
	register int i;
	if( jpg_file.curbit == 0 )
	{
		jpeg_read_byte();
		if( jpg_file.curbyte == 0xFF )
		{
			while( jpg_file.curbyte == 0xFF ) jpeg_read_byte();
			if( jpg_file.curbyte >= 0xD0 && jpg_file.curbyte <= 0xD7 )
				Q_memset( jpg_file.dc, 0, sizeof(int) * 3 );
			if( jpg_file.curbyte == 0 ) jpg_file.curbyte = 0xFF;
			else jpeg_read_byte();
		}
	}

	i = (jpg_file.curbyte >> (7 - jpg_file.curbit++)) & 0x01;
	if( jpg_file.curbit == 8 ) jpg_file.curbit = 0;

	return i;
}

int jpeg_read_bits( int num )
{
	// read num bit
	register int i, j;

	for(i = 0, j = 0; i < num; i++)
	{
		j <<= 1;
		j |= jpeg_read_bit();
	}
	return j;
}

int jpeg_bit2int( int bit, int i )
{
	// convert bit code to int
	if((i & (1 << (bit - 1))) > 0) return i;
	return -(i ^ ((1 << bit) - 1));
}

int jpeg_huffmancode( huffman_table_t *table )
{
	// get Huffman code
	register int i,size,code;
	for(size = 1, code = 0, i = 0; size < 17; size++)
	{
		code <<= 1;
		code |= jpeg_read_bit();
		while(table->size[i] <= size)
		{
			if( table->code[i] == code )
				return table->hval[i];
			i++;
		}
	}
	return code;
}

void jpeg_idct( float *data )
{
	// aa&n algorithm inverse DCT
	float   t0, t1, t2, t3, t4, t5, t6, t7;
	float   t10, t11, t12, t13;
	float   z5, z10, z11, z12, z13;
	float   *dataptr;
	int i;

	dataptr = data;
    
	for(i = 0; i < 8; i++)
	{
		t0 = dataptr[8 * 0];
		t1 = dataptr[8 * 2];
		t2 = dataptr[8 * 4];
		t3 = dataptr[8 * 6];

		t10 = t0 + t2;
		t11 = t0 - t2;
		t13 = t1 + t3;
		t12 = - t13 + (t1 - t3) * 1.414213562;//??
        
		t0 = t10 + t13;
		t3 = t10 - t13;
		t1 = t11 + t12;
		t2 = t11 - t12;
		t4 = dataptr[8 * 1];
		t5 = dataptr[8 * 3];
		t6 = dataptr[8 * 5];
		t7 = dataptr[8 * 7];
        
		z13 = t6 + t5;
		z10 = t6 - t5;
		z11 = t4 + t7;
		z12 = t4 - t7;
        
		t7 = z11 + z13;
		t11 = (z11 - z13) * 1.414213562;
		z5 = (z10 + z12) * 1.847759065;
		t10 = - z5 + z12 * 1.082392200;
		t12 = z5 - z10 * 2.613125930;
		t6 = t12 - t7;
		t5 = t11 - t6;
		t4 = t10 + t5;

		dataptr[8 * 0] = t0 + t7;
		dataptr[8 * 7] = t0 - t7;
		dataptr[8 * 1] = t1 + t6;
		dataptr[8 * 6] = t1 - t6;
		dataptr[8 * 2] = t2 + t5;
		dataptr[8 * 5] = t2 - t5;
		dataptr[8 * 4] = t3 + t4;
		dataptr[8 * 3] = t3 - t4;
		dataptr++;
	}

	dataptr = data;

	for(i = 0; i < 8; i++)
	{
		t10 = dataptr[0] + dataptr[4];
		t11 = dataptr[0] - dataptr[4];
		t13 = dataptr[2] + dataptr[6];
		t12 = - t13 + (dataptr[2] - dataptr[6]) * 1.414213562;//??

		t0 = t10 + t13;
		t3 = t10 - t13;
		t1 = t11 + t12;
		t2 = t11 - t12;
        
		z13 = dataptr[5] + dataptr[3];
		z10 = dataptr[5] - dataptr[3];
		z11 = dataptr[1] + dataptr[7];
		z12 = dataptr[1] - dataptr[7];

		t7 = z11 + z13;
		t11 = (z11 - z13) * 1.414213562;
		z5 = (z10 + z12) * 1.847759065;
		t10 = - z5 + z12 * 1.082392200;
		t12 = z5 - z10 * 2.613125930;
        
		t6 = t12 - t7;
		t5 = t11 - t6;
		t4 = t10 + t5;

		dataptr[0] = t0 + t7;
		dataptr[7] = t0 - t7;
		dataptr[1] = t1 + t6;
		dataptr[6] = t1 - t6;
		dataptr[2] = t2 + t5;
		dataptr[5] = t2 - t5;
		dataptr[4] = t3 + t4;
		dataptr[3] = t3 - t4;
		dataptr += 8;//move ptr
	}
}

int jpeg_readmarkers( void )
{
	// read jpeg markers
	int marker, length, i, j, k, l, m;
	huffman_table_t *hptr;

	while( 1 )
	{
		marker = jpeg_read_byte();
		if( marker != 0xFF ) return 0;

		marker = jpeg_read_byte();
		if( marker != 0xD8 )
		{
			length = jpeg_read_word();
			length -= 2;

			switch( marker )
			{
			case 0xC0:  // baseline
				jpg_file.data_precision = jpeg_read_byte();
				jpg_file.height = jpeg_read_word();
				jpg_file.width = jpeg_read_word();
				jpg_file.num_components = jpeg_read_byte();
				if(length - 6 != jpg_file.num_components * 3) return 0;
				
				for(i = 0; i < jpg_file.num_components; i++)
				{
					jpg_file.component_info[i].id = jpeg_read_byte();
					j = jpeg_read_byte();
					jpg_file.component_info[i].h = (j >> 4) & 0x0F;
					jpg_file.component_info[i].v = j & 0x0F;
					jpg_file.component_info[i].t = jpeg_read_byte();
				}
				break;
			case 0xC1:  // extended sequetial, Huffman
			case 0xC2:  // progressive, Huffman            
			case 0xC3:  // lossless, Huffman
			case 0xC5:  // differential sequential, Huffman
			case 0xC6:  // differential progressive, Huffman
			case 0xC7:  // differential lossless, Huffman
			case 0xC8:  // reserved for JPEG extensions
			case 0xC9:  // extended sequential, arithmetic
			case 0xCA:  // progressive, arithmetic
			case 0xCB:  // lossless, arithmetic
			case 0xCD:  // differential sequential, arithmetic
			case 0xCE:  // differential progressive, arithmetic
			case 0xCF:  // differential lossless, arithmetic
				return 0;	// not supported yet
			case 0xC4:  // huffman table
				while( length > 0 )
				{
					k = jpeg_read_byte();
					if(k & 0x10) hptr = &jpg_file.hac[k & 0x0F];
					else hptr = &jpg_file.hdc[k & 0x0F];
					for(i = 0, j = 0; i < 16; i++)
					{
						hptr->bits[i] = jpeg_read_byte();
						j += hptr->bits[i];
					}
					length -= 17;
					for(i = 0; i < j; i++) hptr->hval[i] = jpeg_read_byte();
					length -= j;

					for(i = 0, k = 0, l = 0; i < 16; i++)
					{
						for(j = 0; j < hptr->bits[i]; j++, k++)
						{
							hptr->size[k] = i + 1;
							hptr->code[k] = l++;
						}
						l <<= 1;
					}
				}
				break;
			case 0xDB:  // quantization table
				while( length > 0 )
				{
					j = jpeg_read_byte();
					k = (j >> 4) & 0x0F;
					for(i = 0; i < 64; i++)
					{
						if( k )jpg_file.qtable[j][i] = jpeg_read_word();
						else jpg_file.qtable[j][i] = jpeg_read_byte();
					}
					length -= 65;
					if( k )length -= 64;
				}
				break;
			case 0xD9:  // end of image (EOI)
				return 0;
			case 0xDA:  // start of scan (SOS)
				j = jpeg_read_byte();
				for(i = 0; i < j; i++)
				{
					k = jpeg_read_byte();
					m = jpeg_read_byte();
					for( l = 0; l < jpg_file.num_components; l++ )
					{
						if( jpg_file.component_info[l].id == k )
						{
							jpg_file.component_info[l].td = (m >> 4) & 0x0F;
							jpg_file.component_info[l].ta = m & 0x0F;
						}
					}
				}
				jpg_file.scan.ss = jpeg_read_byte();
				jpg_file.scan.se = jpeg_read_byte();
				k = jpeg_read_byte();
				jpg_file.scan.ah = (k >> 4) & 0x0F;
				jpg_file.scan.al = k & 0x0F;
				return 1;
			case 0xDD:  // restart interval
				jpg_file.restart_interval = jpeg_read_word();
				break;
			default:
				jpg_file.buffer += length; // move ptr
				break;
			}
		}
	}
}

void jpeg_decompress( void )
{
	// decompress jpeg file (baseline algorithm)
	register int x, y, i, j, k, l, c;
	int X, Y, H, V, plane, scaleh[3], scalev[3];
    	static float vector[64], dct[64];
    
	static const int jpeg_zigzag[64] = 
	{
	0,  1,  5,  6,  14, 15, 27, 28,
	2,  4,  7,  13, 16, 26, 29, 42,
	3,  8,  12, 17, 25, 30, 41, 43,
	9,  11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
	};

	// 1.0, k = 0; cos(k * PI / 16) * sqrt(2), k = 1...7
	static const float aanscale[8] =
	{
	1.0, 1.387039845, 1.306562965, 1.175875602,
	1.0, 0.785694958, 0.541196100, 0.275899379
	};

	scaleh[0] = 1;
	scalev[0] = 1;
    
	if( jpg_file.num_components == 3 )
	{
		scaleh[1] = jpg_file.component_info[0].h / jpg_file.component_info[1].h;
		scalev[1] = jpg_file.component_info[0].v / jpg_file.component_info[1].v;
		scaleh[2] = jpg_file.component_info[0].h / jpg_file.component_info[2].h;
		scalev[2] = jpg_file.component_info[0].v / jpg_file.component_info[2].v;
	}

	Q_memset( jpg_file.dc, 0, sizeof(int) * 3 );

	for( Y = 0; Y < jpg_file.height; Y += jpg_file.component_info[0].v << 3 )
	{
		if( jpg_file.restart_interval > 0 ) jpg_file.curbit = 0;
		for( X = 0; X < jpg_file.width; X += jpg_file.component_info[0].h << 3 )
		{
			for(plane = 0; plane < jpg_file.num_components; plane++)
			{
                			for(V = 0; V < jpg_file.component_info[plane].v; V++)
                			{
					for(H = 0; H < jpg_file.component_info[plane].h; H++)
					{
						i = jpeg_huffmancode(&jpg_file.hdc[jpg_file.component_info[plane].td]);
						i &= 0x0F;
						vector[0] = jpg_file.dc[plane] + jpeg_bit2int(i,jpeg_read_bits(i));
						jpg_file.dc[plane] = vector[0];
						i = 1;

						while(i < 64)
						{
							j = jpeg_huffmancode(&jpg_file.hac[jpg_file.component_info[plane].ta]);
							if(j == 0) while(i < 64) vector[i++] = 0;
							else
							{
								k = i + ((j >> 4) & 0x0F);
								while(i < k) vector[i++] = 0;
								j &= 0x0F;
								vector[i++] = jpeg_bit2int(j,jpeg_read_bits(j));
							}
						}

						k = jpg_file.component_info[plane].t;
						for(y = 0, i = 0; y < 8; y++)
						{
							for(x = 0; x < 8; x++, i++)
							{
								j = jpeg_zigzag[i];
								dct[i] = vector[j] * jpg_file.qtable[k][j] * aanscale[x] * aanscale[y];
							}
						}

						jpeg_idct(dct);
						for(y = 0; y < 8; y++)
						{
							for(x = 0; x < 8; x++)
							{
								c = ((int)dct[(y << 3) + x] >> 3) + 128;
								if(c < 0) c = 0;
								else if(c > 255) c = 255;

								if(scaleh[plane] == 1 && scalev[plane] == 1)
								{
									i = X + x + (H << 3);
									j = Y + y + (V << 3);
									if(i < jpg_file.width && j < jpg_file.height)
										jpg_file.data[((j * jpg_file.width + i) << 2) + plane] = c;
								}
								else for(l = 0; l < scalev[plane]; l++)//else for, heh...
								{
									for(k = 0; k < scaleh[plane]; k++)
									{
										i = X + (x + (H << 3)) * scaleh[plane] + k;
										j = Y + (y + (V << 3)) * scalev[plane] + l;
										if(i < jpg_file.width && j < jpg_file.height)
											jpg_file.data[((j * jpg_file.width + i) << 2) + plane] = c;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void jpeg_ycbcr2rgba( void )
{
	int i, Y, Cb, Cr, R, G, B;

	// convert YCbCr image to RGBA
	for( i = 0; i < jpg_file.width * jpg_file.height << 2; i += 4 )
	{
		Y  = jpg_file.data[i+0];
		Cb = jpg_file.data[i+1] - 128;
		Cr = jpg_file.data[i+2] - 128;

		R = Y + 1.40200 * Cr;
		G = Y - 0.34414 * Cb - 0.71414 * Cr;
		B = Y + 1.77200 * Cb;

		// bound colors
		R = bound( 0, R, 255 );
		G = bound( 0, G, 255 );
		B = bound( 0, B, 255 );

		jpg_file.data[i+0] = R;
		jpg_file.data[i+1] = G;
		jpg_file.data[i+2] = B;
		jpg_file.data[i+3] = 0xff;	// no alpha channel
	}
	image.flags |= IMAGE_HAS_COLOR;
}

void jpeg_gray2rgba( void )
{
	int i, j;

	// grayscale image to RGBA
	for(i = 0; i < jpg_file.width * jpg_file.height << 2; i += 4)
	{
		j = jpg_file.data[i];
		jpg_file.data[i+0] = j;
		jpg_file.data[i+1] = j;
		jpg_file.data[i+2] = j;
		jpg_file.data[i+3] = 0xff;
	}
}


/*
=============
Image_LoadJPG
=============
*/
qboolean Image_LoadJPG( const char *name, const byte *buffer, size_t filesize )
{
	Q_memset( &jpg_file, 0, sizeof( jpg_file ));
	jpg_file.buffer = (byte *)buffer;

	if(!jpeg_readmarkers( )) 
		return false; // it's not a jpg file, just skip it

	image.width = jpg_file.width;
	image.height = jpg_file.height;
	if(!Image_ValidSize( name )) return false;

	image.size = jpg_file.width * jpg_file.height * 4;
	jpg_file.data = Mem_Alloc( host.imagepool, image.size );

	jpeg_decompress();
	if( jpg_file.num_components == 1 ) jpeg_gray2rgba();        
	if( jpg_file.num_components == 3 ) jpeg_ycbcr2rgba();

	image.rgba = jpg_file.data;
	image.type = PF_RGBA_32;

	return true;
}