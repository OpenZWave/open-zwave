#region Header
//-----------------------------------------------------------------------------
//
//      Log.cs
//
// 		This class provides logging capabilites to a file and optionally
// 		to the console.  The amount of logging information can be controlled.
//
//      Copyright (c) 2010 Doug Brown <djbrown2001@gmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------
#endregion Header
using System;
using System.IO;

namespace SynapsityAZ.Utils
{
	/// <summary>
	/// This class provides logging capabilites to a file and optionally
	/// to the console.  The amount of logging information can be controlled
	/// by the Level which is one of the Log.Lvl enumerated types which starts
	/// withe Errors at the least verbose level up to Debug which is the most
	/// verbose level.  The methods to create and write the log are static so
	/// instantiation of the class is not necessary.
	/// </summary>
	public class Log
	{
		public enum Lvl
		{
			Errors = 5,				// Least verbose, errors only
			Warnings = 10,
			Minimal = 15,
			Normal = 20,
			Verbose = 30,			// Everything is logged.
			Debug = 40				// Use for debugging
		}
		private static readonly Log instance = new Log();
		private static TextWriter m_tw = null;
		private static Lvl m_logLvl = Lvl.Minimal;						// Contains the current log level
		private static bool m_console = false;

		// Explicit static constructor to tell C# compiler
		// not to mark type as beforefieldinit
		static Log() {}
		
		public static Lvl Level
		{
			set
			{
				m_logLvl = value;
			}
		}
		
		public static bool OutputToConsole
		{
			set
			{
			  	m_console = value;
			}
		}
		
		public static Log Create(String fileName) 
		{
			return Create(fileName, Lvl.Minimal);
		}
		
		public static Log Create(String fileName, Lvl logLvl)
		{
			if (m_tw == null)
			{
				try
				{
					Open(fileName);
					m_logLvl = logLvl;
				}
				catch (Exception e) {
					System.Console.WriteLine("Error opening log file: " + e.ToString());
				}				
			}
			return instance;
		}
		
		private Log()
		{
		}
		
		private static void Open(String fileName)
		{
			m_tw = new StreamWriter(fileName);
		}
		
		public static void Write(String str)
		{
			Write(Lvl.Verbose, str);
		}
		
		public static void Write(Lvl logLvl, String str)
		{
			if (logLvl <= m_logLvl)
			{
				String outStr = DateTime.Now.ToString("MM/dd/yy-HH:mm:ss.fff") + ": " + str;
				m_tw.WriteLine(outStr);
				m_tw.Flush();
				if (m_console)
				{
					System.Console.WriteLine(outStr);
				}
			}
		}
	}
}
