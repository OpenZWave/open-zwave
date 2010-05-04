#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveNotification.cs
//
//      Thin wrapper for the C++ OpenZWave Notification class
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
	/// Thin wrapper for the C++ OpenZWave Notification class
	/// </summary>
	public class ZWaveNotification
	{
		#region PInvokes

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern NotificationType OPENZWAVEDLL_GetNotifyType(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern UInt32 OPENZWAVEDLL_GetHomeIdFromNotify(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetNodeIdFromNotify(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern IntPtr OPENZWAVEDLL_GetValueID(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetGroupIdx(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern byte OPENZWAVEDLL_GetStatus(IntPtr ptr);

		#endregion	PInvokes
		
		#region Members
		
		public enum NotificationType : byte
		{
			Type_ValueAdded = 0,	// Value Added
			Type_ValueRemoved,		// Value Removed
			Type_ValueChanged,		// Value Changed
			Type_Group,				// Group (associations) changed
			Type_NodeAdded,			// Node has been added
			Type_NodeRemoved,		// Node has been removed
			Type_NodeStatus,		// Node status has changed (usually triggered by receiving a basic_set command from the node)
			Type_PollingDisabled,	// Polling of this node has been turned off
			Type_PollingEnabled,	// Polling of this node has been turned on
			Type_DriverReady		// Driver has been added and is ready to use
		};
		
		#endregion Members
		
		private NotificationType m_type;
		private UInt32 m_homeId;
		private byte m_nodeId;
		private byte m_status;
		private byte m_groupIdx;
		private ZWaveValueId m_valueId;
				
		private ZWaveNotification() {} // Not Implemented
		
		public ZWaveNotification( IntPtr pNotify)
		{
			if (IntPtr.Zero != pNotify)
			{
				m_type = OPENZWAVEDLL_GetNotifyType(pNotify);
				m_homeId = OPENZWAVEDLL_GetHomeIdFromNotify(pNotify);
				m_nodeId = OPENZWAVEDLL_GetNodeIdFromNotify(pNotify);
				if (NotificationType.Type_NodeStatus == m_type)
					m_status = OPENZWAVEDLL_GetStatus(pNotify);
				else
					m_status = 0;
				if (NotificationType.Type_Group == m_type)
					m_groupIdx = OPENZWAVEDLL_GetGroupIdx(pNotify);
				else
					m_groupIdx = 0;
				m_valueId = new ZWaveValueId(OPENZWAVEDLL_GetValueID(pNotify));
			}
			else
			{
				m_type = NotificationType.Type_ValueAdded;
				m_homeId = 0;
				m_nodeId = 0;
				m_status = 0;
				m_groupIdx = 0;
				m_valueId = new ZWaveValueId(OPENZWAVEDLL_GetValueID(IntPtr.Zero));				
			}
		}
		
		#region Wrapper Methods
				
		public NotificationType Type
		{
			get { return m_type; }
		}
					
		public UInt32 HomeId
		{
			get{ return m_homeId; }
		}
					
		public byte NodeId
		{
			get { return m_nodeId; }
		}
		
		public ZWaveValueId ValueId
		{
			get { return m_valueId; }
		}
				
		public byte GroupIdx
		{
			get { return m_groupIdx; }
		}
				
		public byte Status
		{
			get { return m_status; }
		}
				
		#endregion Wrapper Methods
		
	}
}
