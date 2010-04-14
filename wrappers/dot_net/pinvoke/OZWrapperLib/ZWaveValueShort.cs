#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveValueShort.cs
//
//      Thin wrapper for the C++ OpenZWave ValueShort class
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
	/// Thin wrapper for the C++ OpenZWave ValueShort class
	/// </summary>
	public class ZWaveValueShort : IDisposable
	{
		#region PInvokes
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_SetShort(IntPtr ptr, UInt16 setVal);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_OnValueChangedShort(IntPtr ptr, UInt16 setVal);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_GetAsStringByte(IntPtr ptr, StringBuilder sb);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern UInt16 OPENZWAVEDLL_GetValueShort(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern UInt16 OPENZWAVEDLL_GetPendingShort(IntPtr ptr);

		#endregion	PInvokes
		
		#region Members
		
		private IntPtr m_pValue;
		
		#endregion Members
		
		
		private ZWaveValueShort() {} // Not Implemented
		
		public ZWaveValueShort( IntPtr pValue)
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
		
		~ZWaveValueShort()
		{
			Dispose(false);
		}
		
		#region Wrapper Methods
				
		public bool Set(UInt16 setVal)
		{
			return OPENZWAVEDLL_SetShort(this.m_pValue, setVal);
		}
		
		public void OnValueChanged(UInt16 setVal)
		{
			OPENZWAVEDLL_OnValueChangedShort(this.m_pValue, setVal);
		}
		
		public string GetAsString()
		{
			StringBuilder strB = new StringBuilder(80);
			OPENZWAVEDLL_GetAsStringByte(this.m_pValue, strB);
			return strB.ToString();			
		}
		
		public UInt16 GetValue()
		{
			return OPENZWAVEDLL_GetValueShort(this.m_pValue);
		}
		
		public UInt16 GetPending()
		{
			return OPENZWAVEDLL_GetPendingShort(this.m_pValue);
		}
		
		#endregion Wrapper Methods
		
	}
}
