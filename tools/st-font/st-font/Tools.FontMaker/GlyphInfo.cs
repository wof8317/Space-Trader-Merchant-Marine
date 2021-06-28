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
	public sealed class GlyphInfo
	{
		public GlyphInfo( char character, byte[][] glyphData,
			Size charSize, Point glyphOffset )
		{
			this.character = character;
			this.glyphData = glyphData;
			this.size = charSize;
			this.glyphOffset = glyphOffset;
		}

		private char character;
		public Char Character
		{
			get { return character; }
		}

		private byte[][] glyphData;
		public byte[][] GlyphData
		{
			get { return glyphData; }
		}

		public Size GlyphDataSize
		{
			get { return new Size( glyphData[0].Length, glyphData.Length ); }
		}

		private Size size;
		public Size Size
		{
			get { return size; }
			internal set { size = value; }
		}

		private Point glyphOffset;
		public Point GlyphOffset
		{
			get { return glyphOffset; }
			internal set { glyphOffset = value; }
		}

		private GlyphSheet sheet;
		public GlyphSheet Sheet
		{
			get { return sheet; }
			internal set { sheet = value; }
		}

		private Rectangle destRect;
		public Rectangle SheetRectangle
		{
			get { return destRect; }
			internal set { destRect = value; }
		}

		private List<KeyValuePair<char, int>> kernings = new List<KeyValuePair<char, int>>();
		public List<KeyValuePair<char, int>> Kernings
		{
			get { return kernings; }
		}

		private float abcKernLeading;
		public float ABCKernLeading
		{
			get { return abcKernLeading; }
			internal set { abcKernLeading = value; }
		}

		private float abcKernWidth;
		public float ABCKernWidth
		{
			get { return abcKernWidth; }
			internal set { abcKernWidth = value; }
		}

		private float abcKernTrailing;
		public float ABCKernTrailing
		{
			get { return abcKernTrailing; }
			internal set { abcKernTrailing = value; }
		}

		private GlyphInfo smallCap;
		public GlyphInfo SmallCap
		{
			get { return smallCap; }
			internal set { smallCap = value; }
		}
	}
}