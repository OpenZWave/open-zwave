#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveValueList.cs
//
//      Thin wrapper for the C++ OpenZWave ValueList class
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
	/// Thin wrapper for the C++ OpenZWave ValueList class
	/// </summary>
	public class ZWaveValueList : IDisposable
	{
		#region PInvokes
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_SetListByLabel(IntPtr ptr, string setVal);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_SetListByValue(IntPtr ptr, Int32 setVal);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_OnValueChangedList(IntPtr ptr, Int32 valueIdx);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern void OPENZWAVEDLL_GetAsStringList(IntPtr ptr, StringBuilder sb);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern ITEM OPENZWAVEDLL_GetValueListItem(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern ITEM OPENZWAVEDLL_GetPendingListItem(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern Int32 OPENZWAVEDLL_GetItemIdxByLabel(IntPtr ptr, string strLabel);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern Int32 OPENZWAVEDLL_GetItemIdxByValue(IntPtr ptr, Int32 val);

		#endregion	PInvokes
		
		#region Members
		
		private IntPtr m_pValue;
		
		[StructLayout(LayoutKind.Sequential)]
		public struct ITEM
		{
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
			public string m_label;
			[MarshalAs(UnmanagedType.I4)]
			public Int32 m_value;
		};
		
		#endregion Members
		
		
		private ZWaveValueList() {} // Not Implemented
		
		public ZWaveValueList( IntPtr pValue)
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
		
		~ZWaveValueList()
		{
			Dispose(false);
		}
		
		#region Wrapper Methods
				
		public bool SetByLabel(string setVal)
		{
			return OPENZWAVEDLL_SetListByLabel(this.m_pValue, setVal);
		}
				
		public bool SetByValue(Int32 setVal)
		{
			return OPENZWAVEDLL_SetListByValue(this.m_pValue, setVal);
		}
		
		public void OnValueChanged(Int32 valueIdx)
		{
			OPENZWAVEDLL_OnValueChangedList(this.m_pValue, valueIdx);
		}
		
		public string GetAsString()
		{
			StringBuilder strB = new StringBuilder(80);
			OPENZWAVEDLL_GetAsStringList(this.m_pValue, strB);
			return strB.ToString();			
		}
		
		public ITEM GetItem()
		{
			return OPENZWAVEDLL_GetValueListItem(this.m_pValue);
		}
		
		public ITEM GetPending()
		{
			return OPENZWAVEDLL_GetPendingListItem(this.m_pValue);
		}
		
		public Int32 GetItemIdxByLabel(string strLabel)
		{
			return OPENZWAVEDLL_GetItemIdxByLabel(this.m_pValue, strLabel);
		}
				
		public Int32 GetItemIdxByValue(Int32 val)
		{
			return OPENZWAVEDLL_GetItemIdxByValue(this.m_pValue, val);
		}
		
		#endregion Wrapper Methods
		
	}
}
