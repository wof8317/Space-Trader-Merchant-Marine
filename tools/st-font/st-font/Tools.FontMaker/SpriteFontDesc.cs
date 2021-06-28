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
	public sealed class SpriteFontDesc
	{
		private string fontName = string.Empty;
		public string FontName
		{
			get { return fontName; }
			set { fontName = value; }
		}

		private FontStyle style = FontStyle.Regular;
		public FontStyle Style
		{
			get { return style; }
			set { style = value; }
		}

		private List<float> sizes = new List<float>();
		public List<float> Sizes
		{
			get { return sizes; }
		}

		private Size maxImageSize = new Size( 256, 256 );
		public Size MaxImageSize
		{
			get { return maxImageSize; }
			set
			{
				if( !Helpers.IsPowerOf2( value.Width ) ||
					!Helpers.IsPowerOf2( value.Height ) )
					throw new ArgumentException( "Image dimensions must be powers of 2." );

				maxImageSize = value;
			}
		}

		private List<char> characters = new List<char>();
		public List<char> Characters
		{
			get { return characters; }
		}

		private static readonly char[] DefaultCharacters;
		static SpriteFontDesc()
		{
			List<char> chars = new List<char>();

			chars.AddRange( " \"'\\/~!@#$%^&*()_+-=|[]{};:,.<>?`".ToCharArray() );
			for( char c = 'a'; c <= 'z'; c++ )
				chars.Add( c );
			for( char c = 'A'; c <= 'Z'; c++ )
				chars.Add( c );
			for( char c = '0'; c <= '9'; c++ )
				chars.Add( c );

			DefaultCharacters = chars.ToArray();
		}

		public void AddDefaultCharacters()
		{
			Characters.AddRange( DefaultCharacters );
		}

		private bool HasDefaultCharacters()
		{
			foreach( char ch in DefaultCharacters )
				if( !characters.Contains( ch ) )
					return false;

			return true;
		}

		public void Save( Stream stream )
		{
			TextWriter writer = new StreamWriter( stream );

			writer.WriteLine( fontName );
			writer.WriteLine( style );

			for( int i = 0; i < sizes.Count; i++ )
				writer.Write( i != 0 ? ", {0}" : "{0}", sizes[i] );
			writer.WriteLine();

			writer.WriteLine();

			char[] chars;
			if( HasDefaultCharacters() )
			{
				writer.WriteLine( "Default" );

				List<char> charList = new List<char>( characters );
				foreach( char ch in DefaultCharacters )
					charList.Remove( ch );

				chars = charList.ToArray();
			}
			else
				chars = characters.ToArray();
				
			Array.Sort( chars );

			for( int e, s = 0; s < chars.Length; s++ )
			{
				for( e = s + 1; e < chars.Length; e++ )
				{
					if( (int)chars[e] - (int)chars[s] > 1 )
						break;
				}

				if( e - s == 1 )
					writer.WriteLine( "0x{0:X04}", (int)chars[s] );
				else
					writer.WriteLine( "0x{0:X04} - 0x{1:X04}", (int)chars[s], (int)chars[e - 1] );
			}

			writer.Flush();
		}

		private int ParseNum( string s )
		{
			s = s.ToLower();

			if( s.StartsWith( "0x" ) )
			{
				return int.Parse( s.Substring( 2 ), NumberStyles.AllowHexSpecifier );
			}

			return int.Parse( s );
		}

		private string StripComments( string s )
		{
			int idx = s.IndexOf( '#' );
			if( idx != -1 )
				s = s.Substring( 0, idx );

			return s.Trim();
		}

		private void Load( Stream stream )
		{
			TextReader reader = new StreamReader( stream );

			this.sizes.Clear();
			characters.Clear();

			fontName = StripComments( reader.ReadLine() );
			style = (FontStyle)Enum.Parse( typeof( FontStyle ), StripComments( reader.ReadLine() ) );

			string sizeStr = StripComments( reader.ReadLine() );
			string[] sizes = sizeStr.Split( new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries );
			foreach( string s in sizes )
			{
				this.sizes.Add( float.Parse( s.Trim() ) );
			}			
			
			for( ; ; )
			{
				string s = reader.ReadLine();

				if( s == null )
					break;

				s = StripComments( s );

				if( s == string.Empty )
					continue;

				if( string.Compare( s, "Default", StringComparison.InvariantCultureIgnoreCase ) == 0 )
				{
					AddDefaultCharacters();
					continue;
				}

				int idx = s.IndexOf( '-' );
				if( idx == -1 )
				{
					int ch = ParseNum( s );
					characters.Add( (char)ch );
				}
				else
				{
					int cs = ParseNum( s.Substring( 0, idx ) );
					int ce = ParseNum( s.Substring( idx + 1 ) );

					for( int ch = cs; ch <= ce; ch++ )
						characters.Add( (char)ch );
				}
			}

			characters.Sort();
		}

		public static SpriteFontDesc LoadFromFile( Stream stream )
		{
			SpriteFontDesc ret = new SpriteFontDesc();
			ret.Load( stream );
			return ret;
		}

		public static SpriteFontDesc LoadFromFile( string path )
		{
			using( Stream descFile = File.OpenRead( path ) )
				return LoadFromFile( descFile );
		}

		public SpriteFont[] Build()
		{
			return SpriteFont.BuildFonts( this );
		}
	}
}