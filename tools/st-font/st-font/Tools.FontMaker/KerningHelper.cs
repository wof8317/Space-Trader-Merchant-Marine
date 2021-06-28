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
using System.Runtime.InteropServices;

namespace GamePlayer.Tools.FontMaker
{

	internal sealed class KerningHelper : IDisposable
	{
		private IntPtr dc, hFont, oldFont;
		public KerningHelper( Font font )
		{
			dc = CreateCompatibleDC( IntPtr.Zero );
			if( dc == IntPtr.Zero )
				throw new System.ComponentModel.Win32Exception();

			hFont = font.ToHfont();			
			oldFont = SelectObject( dc, hFont );
		}

		private bool FontContainsChar( char ch )
		{
			ushort idx;
			if( GetGlyphIndicesW( dc, ref ch, 1, out idx, GetGlyphIndicesFlags.MarkNonexistingGlyphs ) == uint.MaxValue )
				throw new System.ComponentModel.Win32Exception();

			return idx != 0xFFFF;
		}

		public void GetGlyphKerningInfo( GlyphInfo glyph )
		{
			if( !FontContainsChar( glyph.Character ) )
			{
				glyph.ABCKernLeading = 0;
				glyph.ABCKernWidth = glyph.Size.Width;
				glyph.ABCKernTrailing = 0;

				return;
			}

			ABCFloat abc;
			GetCharABCWidthsFloatW( dc, (uint)glyph.Character, (uint)glyph.Character, out abc );

			glyph.ABCKernLeading = abc.A;
			glyph.ABCKernWidth = abc.B;
			glyph.ABCKernTrailing = abc.C;
		}

		private void DeleteObjects()
		{
			if( dc != IntPtr.Zero )
			{
				SelectObject( dc, oldFont );
				DeleteObject( hFont );
				DeleteDC( dc );
			}

			dc = IntPtr.Zero;
		}

		public void Dispose()
		{
			DeleteObjects();
			GC.SuppressFinalize( this );
		}

		~KerningHelper()
		{
			DeleteObjects();
		}

		private const string GdiLib = "Gdi32.dll";

		[DllImport( GdiLib, SetLastError = true )]
		private static extern IntPtr CreateCompatibleDC( IntPtr dc );

		[DllImport( GdiLib, SetLastError = true )]
		private static extern IntPtr SelectObject( IntPtr dc, IntPtr hObj );

		[DllImport( GdiLib, SetLastError = true )]
		private static extern int DeleteObject( IntPtr obj );

		[DllImport( GdiLib, SetLastError = true )]
		private static extern int DeleteDC( IntPtr dc );

		[StructLayout( LayoutKind.Sequential )]
		private struct ABCFloat
		{
			public float A, B, C;
		}

		[DllImport( GdiLib, SetLastError = true )]
		private static extern int GetCharABCWidthsFloatW( IntPtr dc, uint iFirstChar, uint iLastChar, out ABCFloat abcFloat );

		[Flags]
		private enum GetGlyphIndicesFlags : uint
		{
			MarkNonexistingGlyphs = 0x00000001,
		}

		[DllImport( GdiLib, SetLastError = true )]
		private static extern uint GetGlyphIndicesW( IntPtr dc, ref char ch, int numChars, out ushort indices, GetGlyphIndicesFlags flags );
	}

}