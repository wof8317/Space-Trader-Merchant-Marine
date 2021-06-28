/******************************************************************************
	st-font - Unicode enabled font-building tool for Space Trader.
	Copyright (C) 2007 HermitWorks Entertainment Corporation

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
	for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the
	
		Free Software Foundation, Inc.
		51 Franklin Street, Fifth Floor
		Boston, MA  02110-1301, USA.
******************************************************************************/

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Globalization;
using System.Text;

namespace GamePlayer.Tools.FontMaker
{
	public sealed class GlyphSheet
	{
		public GlyphSheet( Size imageSize )
		{
			sheetData = new byte[imageSize.Height][];
			for( int i = 0; i < sheetData.Length; i++ )
				sheetData[i] = new byte[imageSize.Width];
		}

		private byte[][] sheetData;
		public byte[][] SheetData
		{
			get { return sheetData; }
		}

		public Size SheetDataSize
		{
			get { return new Size( sheetData[0].Length, sheetData.Length ); }
		}

		private List<GlyphInfo> glyphs = new List<GlyphInfo>();
		public List<GlyphInfo> Glyphs
		{
			get { return glyphs; }
		}
	}

	internal sealed class GlyphPacker
	{
		private int padding = 1;
		private Size imageSize;
		public GlyphPacker( Size imageSize )
		{
			this.imageSize = imageSize;
		}

		private List<GlyphSheet> LayoutSheets( Queue<GlyphInfo> glyphs )
		{
			List<GlyphSheet> ret = new List<GlyphSheet>();

			//ToDo: sort?

			while( glyphs.Count != 0 )
			{
				GlyphSheet currSheet = new GlyphSheet( imageSize );

				int y;
				for( y = 0; glyphs.Count != 0 && y < imageSize.Height; )
				{
					int availY = imageSize.Height - y;
					int maxY = 0;

					for( int x = 0; glyphs.Count != 0 && x < imageSize.Width; )
					{
						GlyphInfo glyph = glyphs.Peek();

						//see if we have room to add anything height-wise
						if( glyph.GlyphDataSize.Height > availY )
							//no, break out
							break;

						//find out where we'll be
						int newX = x + glyph.GlyphDataSize.Width;

						if( newX > imageSize.Width )
							//no room, break on to the next room
							break;

						//don't let the next row stomp on this
						if( glyph.GlyphDataSize.Height > maxY )
							maxY = glyph.GlyphDataSize.Height;

						glyph.Sheet = currSheet;
						glyph.SheetRectangle = new Rectangle( x, y, glyph.GlyphDataSize.Width, glyph.GlyphDataSize.Height );
						currSheet.Glyphs.Add( glyph );

						//advance to the next character
						glyphs.Dequeue();
						x = newX + padding;
					}

					y += maxY + padding;
				}

				if( y == 0 )
					continue;

				//see if we can cut off the bottom of the image
				if( y < currSheet.SheetDataSize.Height / 2 )
				{
					int targetH = currSheet.SheetDataSize.Height / 2;
					while( y < targetH / 2 )
						targetH /= 2;

					GlyphSheet newSheet = new GlyphSheet( new Size( currSheet.SheetDataSize.Width, targetH ) );

					newSheet.Glyphs.AddRange( currSheet.Glyphs );
					foreach( GlyphInfo glyph in newSheet.Glyphs )
						glyph.Sheet = newSheet;

					currSheet = newSheet;
				}

				ret.Add( currSheet );
			}

			return ret;
		}

		private void CopyGlyphData( List<GlyphSheet> sheets )
		{
			foreach( GlyphSheet sheet in sheets )
			{
				foreach( GlyphInfo glyph in sheet.Glyphs )
				{
					for( int y = 0; y < glyph.SheetRectangle.Height; y++ )
					{
						for( int x = 0; x < glyph.SheetRectangle.Width; x++ )
						{
							sheet.SheetData[glyph.SheetRectangle.Y + y][glyph.SheetRectangle.X + x] =
								glyph.GlyphData[y][x];
						}
					}
				}
			}
		}

		public List<GlyphSheet> PackGlyphs( List<GlyphInfo> glyphs )
		{
			Queue<GlyphInfo> src = new Queue<GlyphInfo>( glyphs );

			List<GlyphSheet> ret = LayoutSheets( src );
			CopyGlyphData( ret );

			return ret;			
		}
	}
}