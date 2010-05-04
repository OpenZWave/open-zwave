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

        public struct VALUEIDSTRUCT
        {
            public UInt32 homeId;
            public byte nodeId;
            public ValueGenre genre;
            public byte commandClassId;
            public byte instance;
            public byte valueIndex;
            public ValueType type;
        };

        private VALUEIDSTRUCT m_valueId;

		#endregion
		
		private ZWaveValueId() {} // not implemented
			
		public ZWaveValueId(IntPtr pValueId)
		{
			//TODO: throw exception or return return null if pNode is null
            if (IntPtr.Zero != pValueId)
            {
                // If this is too slow might have to extract the fields
                // here instead of calling the P/Invoke.
                m_valueId.homeId = OPENZWAVEDLL_GetHomeId(pValueId);
                m_valueId.nodeId = OPENZWAVEDLL_GetNodeId(pValueId);
                m_valueId.genre = OPENZWAVEDLL_GetGenre(pValueId);
                m_valueId.commandClassId = OPENZWAVEDLL_GetCommandClassId(pValueId);
                m_valueId.instance = OPENZWAVEDLL_GetInstance(pValueId);
                m_valueId.valueIndex = OPENZWAVEDLL_GetIndex(pValueId);
                m_valueId.type = OPENZWAVEDLL_GetType(pValueId);
            }
            else
            {
                m_valueId.homeId = 0;
                m_valueId.nodeId = 0;
                m_valueId.genre = ValueGenre.ValueGenre_All;
                m_valueId.commandClassId = 0;
                m_valueId.instance = 0;
                m_valueId.valueIndex = 0;
                m_valueId.type = ValueType.ValueType_Bool;
            }
		}
		
        public VALUEIDSTRUCT ValueID
        {
            get { return m_valueId; }
        }
		
		#region Wrapper Methods
		
		public UInt32 HomeId
		{
			get { return m_valueId.homeId; }
		}
		
		public byte NodeId
		{
			get { return m_valueId.nodeId; }
		}
		
		public ValueGenre Genre
		{
			get { return m_valueId.genre; }
		}
		
		public byte CommandClassId
		{
			get { return m_valueId.commandClassId; }
		}
		
		public byte Instance
		{
			get { return m_valueId.instance; }
		}
		
		public byte Index
		{
			get { return m_valueId.valueIndex; }
		}
		
		public ValueType Type
		{
			get { return m_valueId.type; }
		}
		
		public static bool operator ==(ZWaveValueId id1, ZWaveValueId id2)
		{
			if ((id1.HomeId == id2.HomeId) &&
			    (id1.NodeId == id2.NodeId) &&
			    (id1.Genre  == id2.Genre)  &&
			    (id1.CommandClassId == id2.CommandClassId) &&
			    (id1.Instance == id2.Instance) &&
			    (id1.Index == id2.Index) &&
			    (id1.Instance == id2.Instance)) return true;
			else
				return false;
		}
		
		public static bool operator !=(ZWaveValueId id1, ZWaveValueId id2)
		{
			if ((id1.HomeId != id2.HomeId) ||
			    (id1.NodeId != id2.NodeId) ||
			    (id1.Genre  != id2.Genre)  ||
			    (id1.CommandClassId != id2.CommandClassId) ||
			    (id1.Instance != id2.Instance) ||
			    (id1.Index != id2.Index) ||
			    (id1.Instance != id2.Instance)) return true;
			else
				return false;
		}
		
		public override bool Equals(object o2)
		{
			ZWaveValueId id2 = (ZWaveValueId) o2;
			return (this == id2);
		}
		
		public override int GetHashCode()
		{
			return m_valueId.GetHashCode();
		}
		
		#endregion Wrapper Methods
		
	}
}
