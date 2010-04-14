#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveValueDecimal.cs
//
//      Thin wrapper for the C++ OpenZWave ValueDecimal class
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
using System.Text;
using System.Runtime.InteropServices;

namespace OpenZWaveWrapper
{
	/// <summary>
	/// Thin wrapper for the C++ OpenZWave ValueDecimal class
	/// </summary>
	public class ZWaveValueDecimal : IDisposable
	{
		#region PInvokes
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_SetDecimal(IntPtr ptr, string setVal);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_OnValueChangedDecimal(IntPtr ptr, string setVal);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_GetAsStringDecimal(IntPtr ptr, StringBuilder sb);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_GetValueDecimal(IntPtr ptr, StringBuilder sb);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_GetPendingDecimal(IntPtr ptr, StringBuilder sb);

		#endregion	PInvokes
		
		#region Members
		
		private IntPtr m_pValue;
		
		#endregion Members
		
		
		private ZWaveValueDecimal() {} // Not Implemented
		
		public ZWaveValueDecimal( IntPtr pValue)
		{
			//TODO: throw exception or return return null if pNode is null
			if (IntPtr.Zero != pValue)
			{
				m_pValue = pValue;
			}
		}
		public void Dispose()
		{
			Dispose(true);
		}
		
		protected virtual void Dispose(bool bDisposing)
		{
			if (this.m_pValue != IntPtr.Zero)
			{
				this.m_pValue = IntPtr.Zero;
			}
			if (bDisposing)
			{
				// No need to call the finalizer since we've now cleaned
				// up the unmanaged memory

				GC.SuppressFinalize(this);
			}
		}
		
		~ZWaveValueDecimal()
		{
			Dispose(false);
		}
		
		#region Wrapper Methods
				
		public bool Set(string setVal)
		{
			return OPENZWAVEDLL_SetDecimal(this.m_pValue, setVal);
		}
		
		public void OnValueChanged(string setVal)
		{
			OPENZWAVEDLL_OnValueChangedDecimal(this.m_pValue, setVal);
		}
		
		public string GetAsString()
		{
			StringBuilder strB = new StringBuilder(80);
			OPENZWAVEDLL_GetAsStringDecimal(this.m_pValue, strB);
			return strB.ToString();			
		}
		
		public string GetValue()
		{
			StringBuilder strB = new StringBuilder(80);
			OPENZWAVEDLL_GetValueDecimal(this.m_pValue, strB);
			return strB.ToString();			
		}
		
		public string GetPending()
		{
			StringBuilder strB = new StringBuilder(80);
			OPENZWAVEDLL_GetPendingDecimal(this.m_pValue, strB);
			return strB.ToString();			
		}
		
		#endregion Wrapper Methods
		
	}
}
