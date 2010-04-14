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
	public class ZWaveNotification : IDisposable
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
		
		private IntPtr m_pNotify;
		
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
				
		private ZWaveNotification() {} // Not Implemented
		
		public ZWaveNotification( IntPtr pNotify)
		{
			//TODO: throw exception or return return null if pNode is null
			if (IntPtr.Zero != pNotify)
			{
				m_pNotify = pNotify;
			}
		}
		public void Dispose()
		{
			Dispose(true);
		}
		
		protected virtual void Dispose(bool bDisposing)
		{
			if (this.m_pNotify != IntPtr.Zero)
			{
				this.m_pNotify = IntPtr.Zero;
			}
			if (bDisposing)
			{
				// No need to call the finalizer since we've now cleaned
				// up the unmanaged memory

				GC.SuppressFinalize(this);
			}
		}
		
		~ZWaveNotification()
		{
			Dispose(false);
		}
		
		#region Wrapper Methods
				
		public NotificationType Type
		{
			get { return OPENZWAVEDLL_GetNotifyType(this.m_pNotify); }
		}
					
		public UInt32 HomeId
		{
			get{ return OPENZWAVEDLL_GetHomeIdFromNotify(this.m_pNotify);	}
		}
					
		public byte NodeId
		{
			get { return OPENZWAVEDLL_GetNodeIdFromNotify(this.m_pNotify); }
		}
		
		public IntPtr ValueId
		{
			get { return OPENZWAVEDLL_GetValueID(this.m_pNotify); }
		}
				
		public byte GroupIdx
		{
			get { return OPENZWAVEDLL_GetGroupIdx(this.m_pNotify); }
		}
				
		public byte Status
		{
			get { return OPENZWAVEDLL_GetStatus(this.m_pNotify); }
		}
				
		#endregion Wrapper Methods
		
	}
}
