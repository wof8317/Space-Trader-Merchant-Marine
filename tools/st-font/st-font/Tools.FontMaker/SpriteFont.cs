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
using System.Drawing.Text;
using System.Globalization;
using System.IO;
using System.Text;

namespace GamePlayer.Tools.FontMaker
{
	public sealed class SpriteFont
	{
		private string fontName;
		public string FontName
		{
			get { return fontName; }
		}

		private FontStyle style;
		public FontStyle Style
		{
			get { return style; }
		}

		private float size;
		public float Size
		{
			get { return size; }
		}

		private List<char> characters;
		public List<char> Characters
		{
			get { return characters; }
		}

		private SpriteFont( SpriteFontDesc desc, float size )
		{
			this.fontName = desc.FontName;
			this.style = desc.Style;
			this.size = size;
			this.maxImageSize = desc.MaxImageSize;
			this.characters = new List<char>( desc.Characters );
		}

		private Font CreateFont()
		{
			float emSize = size * 96.0F / 72.0F;
			return new Font( fontName, emSize, style, GraphicsUnit.Pixel );
		}

		private Size maxImageSize;

		private void Build()
		{
			characters.Sort();

			for( int i = 0; i < characters.Count; i++ )
			{
				for( int j = i + 1; j < characters.Count; j++ )
				{
					if( characters[j] == characters[i] )
						characters.RemoveAt( j-- );
				}
			}

			glyphs.Clear();
			sheets.Clear();

			using( Font font = CreateFont() )
			using( FontRasterizer rasterizer = new FontRasterizer() )
			using( KerningHelper kerner = new KerningHelper( font ) )
			{
				lineHeight = font.Height;
				foreach( char ch in characters )
				{
					GlyphInfo glyph = rasterizer.Rasterize( font, ch );

					kerner.GetGlyphKerningInfo( glyph );

					glyphs.Add( glyph );
				}
			}

			foreach( GlyphInfo glyph in glyphs )
			{
				char c = char.ToUpper( glyph.Character );
				if( c == glyph.Character )
					continue;

				GlyphInfo upper = glyphs.Find( delegate( GlyphInfo g ) { return g.Character == c; } );
				glyph.SmallCap = upper;
			}

			glyphs.Sort( delegate( GlyphInfo a, GlyphInfo b ) { return a.Character.CompareTo( b.Character ); } );

			sheets = new GlyphPacker( maxImageSize ).PackGlyphs( glyphs );
		}

		private List<GlyphInfo> glyphs = new List<GlyphInfo>();
		public List<GlyphInfo> Glyphs
		{
			get { return glyphs; }
		}

		private List<GlyphSheet> sheets = new List<GlyphSheet>();
		public List<GlyphSheet> Sheets
		{
			get { return sheets; }
		}

		private int lineHeight;
		public int LineHeight
		{
			get { return lineHeight; }
		}

		private const int FontIdent = unchecked( ('H' << 24) | ('F' << 16) | ('N' << 8) | ('T' << 0) );
		private const int FontVersion = 1;

		public void WriteData( Stream stream )
		{
			if( stream == null )
				throw new ArgumentNullException( "stream" );

			int[] kernMap = new int[glyphs.Count];
			List<KeyValuePair<char, int>> allKernings = new List<KeyValuePair<char, int>>();

			for( int i = 0; i < glyphs.Count; i++ )
			{
				if( glyphs[i].Kernings.Count == 0 )
				{
					kernMap[i] = -1;
					continue;
				}

				kernMap[i] = allKernings.Count;
				allKernings.AddRange( glyphs[i].Kernings );
			}

			BinaryWriter writer = new BinaryWriter( stream );

			writer.Write( FontIdent );
			writer.Write( FontVersion );
			writer.Write( fontName.Length );
			for( int i = 0; i < fontName.Length; i++ )
				writer.Write( (int)fontName[i] );
			writer.Write( (int)style );
			writer.Write( size );
			writer.Write( lineHeight );
			writer.Write( (int)glyphs.Count );
			writer.Write( (int)sheets.Count );
			writer.Write( (int)allKernings.Count );

			for( int i = 0; i < glyphs.Count; i++ )
			{
				GlyphInfo g = glyphs[i];
				writer.Write( (int)g.Character );
			}

			for( int i = 0; i < glyphs.Count; i++ )
			{
				GlyphInfo g = glyphs[i];
				writer.Write( sheets.IndexOf( g.Sheet ) );
				writer.Write( g.Size.Width );
				writer.Write( g.Size.Height );
				writer.Write( g.GlyphOffset.X );
				writer.Write( g.GlyphOffset.Y );
				writer.Write( g.SheetRectangle.X );
				writer.Write( g.SheetRectangle.Y );
				writer.Write( g.SheetRectangle.Width );
				writer.Write( g.SheetRectangle.Height );
				writer.Write( g.ABCKernLeading );
				writer.Write( g.ABCKernWidth );
				writer.Write( g.ABCKernTrailing );
				writer.Write( kernMap[i] );
				writer.Write( g.Kernings.Count );
				writer.Write( glyphs.IndexOf( g.SmallCap ) );
			}

			for( int i = 0; i < allKernings.Count; i++ )
			{
				writer.Write( (int)allKernings[i].Key );
				writer.Write( allKernings[i].Value );
			}

			for( int i = 0; i < sheets.Count; i++ )
			{
				writer.Write( sheets[i].SheetDataSize.Width );
				writer.Write( sheets[i].SheetDataSize.Height );
				for( int y = 0; y < sheets[i].SheetDataSize.Height; y++ )
					writer.Write( sheets[i].SheetData[y], 0, sheets[i].SheetDataSize.Width );
			}

			writer.Flush();
		}

		public void WriteData( string path )
		{
			using( Stream file = File.Create( path ) )
				WriteData( file );
		}

		public static SpriteFont[] BuildFonts( SpriteFontDesc desc )
		{
			SpriteFont[] ret = new SpriteFont[desc.Sizes.Count];

			for( int i = 0; i < ret.Length; i++ )
			{
				ret[i] = new SpriteFont( desc, desc.Sizes[i] );
				ret[i].Build();
			}

			return ret;
		}
	}
}