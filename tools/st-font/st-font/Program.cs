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
using System.IO;
using System.Windows.Forms;

namespace STFont
{
	using GamePlayer.Tools.FontMaker;

	static class Program
	{
		private static void BuildFont( string fontFile )
		{
			Console.WriteLine( "Building '{0}'...", fontFile );
			SpriteFontDesc desc = SpriteFontDesc.LoadFromFile( fontFile );

			SpriteFont[] fonts = desc.Build();

			foreach( SpriteFont font in fonts )
			{
				string path = Path.ChangeExtension( fontFile, null );
				path = string.Format( "{0}_{1}.fnt", path, font.Size );
				font.WriteData( path );
			}
		}

		private static void BuildDirectory( string directory )
		{
			foreach( string s in Directory.GetFiles( directory, "*.stfont" ) )
			{
				try
				{
					BuildFont( s );
				}
				catch( Exception ex )
				{
					PrintFailure( s, ex );
				}
			}
		}

		private static void PrintFailure( string file, Exception ex )
		{
			Console.WriteLine( "Failed to build font file '{0}'.", file );
			Console.WriteLine( "{0}: \"{1}\" at\n{2}", ex.GetType().Name, ex.Message, ex.StackTrace );
		}

		[System.Runtime.InteropServices.DllImport( "Kernel32.dll" )]
		private static extern Boolean AllocConsole();

		[STAThread]
		static void Main( string[] args )
		{
			if( args.Length == 0 )
			{
				Application.EnableVisualStyles();
				Application.SetCompatibleTextRenderingDefault( false );
				Application.Run( new MainWnd() );

				return;
			}

			AllocConsole();

			foreach( string s in args )
			{
				try
				{
					if( s == "." )
						BuildDirectory( Environment.CurrentDirectory );
					else if( Directory.Exists( s ) )
						BuildDirectory( s );
					else
						BuildFont( s );
				}
				catch( Exception ex )
				{
					PrintFailure( s, ex );
				}
			}
		}
	}
}