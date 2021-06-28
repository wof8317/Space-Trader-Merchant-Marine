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
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Globalization;
using System.Text;

namespace GamePlayer.Tools.FontMaker
{
	internal sealed class FontRasterizer : IDisposable
	{
		private Bitmap bmp;
		private Graphics g;

		public FontRasterizer()
		{
			EnsureTemporaryBitmap( 32, 32 );
		}

		private void EnsureTemporaryBitmap( int width, int height )
		{
			if( bmp != null )
			{
				if( bmp.Width >= width && bmp.Height >= height )
				{
					g.Clear( Color.Black );
					return;
				}

				width = Math.Max( width, bmp.Width );
				height = Math.Max( height, bmp.Height );
			}

			Helpers.DisposeAndClear( ref g );
			Helpers.DisposeAndClear( ref bmp );

			bmp = new Bitmap( width, height, PixelFormat.Format32bppRgb );

			g = Graphics.FromImage( bmp );
			
			g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
			g.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
			g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
			
			g.Clear( Color.Black );
		}

		private unsafe byte GetPixelValue( BitmapData data, int x, int y )
		{
			byte* b = (byte*)data.Scan0 + y * data.Stride + x * 4;
			return b[1];
		}

		private bool ColumnIsEmpty( BitmapData data, int c )
		{
			for( int i = 0; i < data.Height; i++ )
				if( GetPixelValue( data, c, i ) != 0 )
					return false;

			return true;
		}

		private bool RowIsEmpty( BitmapData data, int c )
		{
			for( int i = 0; i < data.Width; i++ )
				if( GetPixelValue( data, i, c ) != 0 )
					return false;

			return true;
		}

		private byte[][] GetImageAlphaBytes( ref Point glyphOffset )
		{
			BitmapData bmpDat = null;
			try
			{
				Rectangle rc = new Rectangle( 0, 0, bmp.Width, bmp.Height );
				bmpDat = bmp.LockBits( rc, ImageLockMode.ReadOnly, PixelFormat.Format32bppRgb );

				//find the portion that actually contains data

				while( ColumnIsEmpty( bmpDat, rc.Right - 1 ) )
					rc.Width--;

				while( ColumnIsEmpty( bmpDat, rc.Left ) )
				{
					rc.X++;
					rc.Width--;
					glyphOffset.X++;
				}

				while( RowIsEmpty( bmpDat, rc.Bottom - 1 ) )
					rc.Height--;

				while( RowIsEmpty( bmpDat, rc.Top ) )
				{
					rc.Y++;
					rc.Height--;
					glyphOffset.Y++;
				}

				if( rc.Right <= rc.Left || rc.Bottom <= rc.Top )
				{
					byte[][] empty = new byte[1][];
					empty[0] = new byte[1];
					return empty;
				}

				//copy it out into a managed array

				byte[][] ret = new byte[rc.Height][];
				for( int y = 0; y < rc.Height; y++ )
				{
					ret[y] = new byte[rc.Width];
					for( int x = 0; x < rc.Width; x++ )
						ret[y][x] = GetPixelValue( bmpDat, rc.Left + x, rc.Top + y );
				}
				return ret;
			}
			finally
			{
				if( bmpDat != null )
					bmp.UnlockBits( bmpDat );
			}
		}

		public GlyphInfo Rasterize( Font font, char character )
		{
			string s = character.ToString();

			Size glyphSize = Size.Ceiling( g.MeasureString( s, font ) );

			EnsureTemporaryBitmap(
				glyphSize.Width + 32,
				glyphSize.Height + 32 );

			Point glyphOffset = new Point( 8, 8 );

			g.DrawString( s, font, Brushes.White, glyphOffset );
			g.Flush();

			glyphOffset = new Point( -glyphOffset.X, -glyphOffset.Y );
			byte[][] glyphData = GetImageAlphaBytes( ref glyphOffset );

			return new GlyphInfo( character, glyphData, glyphSize, glyphOffset );
		}

		public void Dispose()
		{
			Helpers.DisposeAndClear( ref g );
			Helpers.DisposeAndClear( ref bmp );
		}
	}
}