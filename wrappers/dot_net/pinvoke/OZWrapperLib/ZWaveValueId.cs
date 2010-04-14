#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveValueID.cs
//
//      Thin wrapper for the C++ OpenZWave ValueID class
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
using System.Runtime.InteropServices;

namespace OpenZWaveWrapper
{
	/// <summary>
	/// Thin wrapper for the C++ OpenZWave ValueID class
	/// </summary>
	public class ZWaveValueId
	{
		#region PInvokes

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern UInt32 OPENZWAVEDLL_GetHomeId(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetNodeId(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetCommandClassId(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetInstance(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetIndex(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern ValueGenre OPENZWAVEDLL_GetGenre(IntPtr ptr);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern ValueType OPENZWAVEDLL_GetType(IntPtr ptr);

		#endregion	PInvokes
		
		#region members
		private IntPtr m_pValueId;
		
		public enum ValueType : byte
		{
			ValueType_Bool = 0,
			ValueType_Byte,
			ValueType_Decimal,
			ValueType_Int,
			ValueType_List,
			ValueType_Short,
			ValueType_String,
			ValueType_Count			
		};
		
		public enum ValueGenre : byte
		{
			ValueGenre_All = 0,
			ValueGenre_User,			// Basic values an ordinary user would be interested in
			ValueGenre_Config,			// Device-specific configuration parameters
			ValueGenre_System,			// Values of significance only to users who understand the Z-Wave protocol 
			ValueGenre_Count
		};
		
		#endregion
		
		private ZWaveValueId() {} // not implemented
			
		public ZWaveValueId(IntPtr pValueId)
		{
			//TODO: throw exception or return return null if pNode is null
			if (IntPtr.Zero != pValueId)
			{
				m_pValueId = pValueId;
			}
		}
		
		public IntPtr ValueIdRef
		{
			get { return m_pValueId; }
		}
		
		#region Wrapper Methods
		
		public UInt32 HomeId
		{
			get { return OPENZWAVEDLL_GetHomeId(this.m_pValueId); }
		}
		
		public byte NodeId
		{
			get { return OPENZWAVEDLL_GetNodeId(this.m_pValueId); }
		}
		
		public ValueGenre Genre
		{
			get { return OPENZWAVEDLL_GetGenre(this.m_pValueId); }
		}
		
		public byte CommandClassId
		{
			get { return OPENZWAVEDLL_GetCommandClassId(this.m_pValueId); }
		}
		
		public byte Instance
		{
			get { return OPENZWAVEDLL_GetInstance(this.m_pValueId); }
		}
		
		public byte Index
		{
			get { return OPENZWAVEDLL_GetIndex(this.m_pValueId); }
		}
		
		public ValueType Type
		{
			get { return OPENZWAVEDLL_GetType(this.m_pValueId); }
		}
		
		#endregion Wrapper Methods
		
	}
}
