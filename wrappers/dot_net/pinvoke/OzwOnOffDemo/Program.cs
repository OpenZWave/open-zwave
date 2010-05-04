/*
 * Created by SharpDevelop.
 * User: Doug Brown
 * Date: 4/15/2010
 * Time: 7:39 AM
 * 
 * Copyright 2010 Syanpsity-AZ
 */
using System;
using System.Windows.Forms;

namespace OzwOnOffDemo
{
	/// <summary>
	/// Class with program entry point.
	/// </summary>
	internal sealed class Program
	{
		/// <summary>
		/// Program entry point.
		/// </summary>
		[STAThread]
		private static void Main(string[] args)
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new MainForm());
		}
		
	}
}
