#region Header
//-----------------------------------------------------------------------------
//
//      ZWaveManager.cs
//
//      Thin wrapper for the C++ OpenZWave Manager class
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
	/// Thin wrapper for the C++ OpenZWave Manager class
	/// </summary>
	public class ZWaveManager : IDisposable
	{
		#region PInvokes
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		private static extern IntPtr OPENZWAVEDLL_Create(
			string strConfigPath,
			string strUserPath);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_DisposeManager(IntPtr ptr);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_AddDriver(IntPtr ptr, string strPort);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_RemoveDriver(IntPtr ptr, string strPort);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_IsSlave(IntPtr ptr, UInt32 homeId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_HasTimerSupport(IntPtr ptr, UInt32 homeId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_IsPrimaryController(IntPtr ptr, UInt32 homeId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_IsStaticUpdateController(IntPtr ptr, UInt32 homeId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_SetPollInterval(IntPtr ptr, Int32 seconds);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_EnablePoll(IntPtr ptr, UInt32 homeId, byte nodeId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_DisablePoll(IntPtr ptr, UInt32 homeId, byte nodeId);

		// These unmanaged GetValue methods return pointers to Value Objects, these will have to be turned
		// into value objects on the managed side, so we'll need managed classes for each of the return types
		//		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueBoolPtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueBytePtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueDecimalPtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueIntPtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueListPtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueShortPtr(IntPtr ptr, IntPtr valueId);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern IntPtr OPENZWAVEDLL_GetValueStringPtr(IntPtr ptr, IntPtr valueId);		
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_AddWatcher(IntPtr ptr,
		                    NotificationDelegate watcherCB, IntPtr context);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		[return : MarshalAs(UnmanagedType.Bool)] private static extern bool OPENZWAVEDLL_RemoveWatcher(IntPtr ptr,
		                    NotificationDelegate watcherCB, IntPtr context);
		
		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_NotifyWatchers(IntPtr ptr, IntPtr pNotification);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_ResetController(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_SoftReset(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_RequestNodeNeighborUpdate(IntPtr ptr, UInt32 homeId, byte nodeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_AssignReturnRoute(IntPtr ptr, UInt32 homeId, byte srcNodeId, byte dstNodeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_BeginAddNode(IntPtr ptr, UInt32 homeId, [MarshalAs(UnmanagedType.Bool)]bool highPower);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_BeginAddController(IntPtr ptr, UInt32 homeId, [MarshalAs(UnmanagedType.Bool)]bool highPower);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_EndAddNode(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_BeginRemoveNode(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_EndRemoveNode(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_BeginReplicateController(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_EndReplicateController(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_RequestNetworkUpdate(IntPtr ptr, UInt32 homeId);

		[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
		static private extern void OPENZWAVEDLL_ControllerChange(IntPtr ptr, UInt32 homeId);

        //[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
        //static private extern void OPENZWAVEDLL_ReadMemory(IntPtr ptr, UInt32 homeId, UInt16 offset);

        //[DllImport("OpenZwaveDLL.dll", SetLastError=true)]
        //static private extern void OPENZWAVEDLL_SetConfiguration(IntPtr ptr, UInt32 homeId, byte nodeId, byte parameter, UInt32 value);
		
		#endregion PInvokes
		
		#region Members
		private IntPtr m_pManager;
		
		[UnmanagedFunctionPointer (CallingConvention.Cdecl)]
		public delegate void NotificationDelegate(IntPtr pNotification, IntPtr pContext);
		
		#endregion Members
		
		private ZWaveManager() {}  // not implemented
				
		public ZWaveManager(string configPath, string userPath)
		{
			this.m_pManager = OPENZWAVEDLL_Create(configPath, userPath);
		}
		
		public void Dispose()
		{
			Dispose(true);
		}
		
		protected virtual void Dispose(bool bDisposing)
		{
			if (this.m_pManager != IntPtr.Zero)
			{
				OPENZWAVEDLL_DisposeManager(this.m_pManager);
				this.m_pManager = IntPtr.Zero;
			}
			if (bDisposing)
			{
				// No need to call the finalizer since we've now cleaned up
				// unmanaged memory
				GC.SuppressFinalize(this);
			}
		}
		
		// This finalizer is called when Garbage collection occurs, but only if
		// the IDisposable.Dispose method wasn't already called.
		~ZWaveManager()
		{
			Dispose(false);
		}
		
		#region Wrapper Methods
		
		public bool AddDriver(string strPort)
		{
			return OPENZWAVEDLL_AddDriver(this.m_pManager, strPort);
		}
		
		public bool RemoveDriver(string strPort)
		{
			return OPENZWAVEDLL_AddDriver(this.m_pManager, strPort);
		}
		
		public bool IsSlave(UInt32 homeId)
		{
			return OPENZWAVEDLL_IsSlave(this.m_pManager, homeId);
		}
		
		public bool HasTimerSupport(UInt32 homeId)
		{
			return OPENZWAVEDLL_HasTimerSupport(this.m_pManager, homeId);
		}
		
		public bool IsPrimaryController(UInt32 homeId)
		{
			return OPENZWAVEDLL_IsPrimaryController(this.m_pManager, homeId);
		}
		
		public bool IsStaticUpdateController(UInt32 homeId)
		{
			return OPENZWAVEDLL_IsStaticUpdateController(this.m_pManager, homeId);
		}

		//TODO: For all of these "GetValue*" methods return an object of the correct
		//      type, not a generic pointer
		public IntPtr GetValueBool(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueBoolPtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueByte(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueBytePtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueDecimal(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueDecimalPtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueInt(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueIntPtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueList(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueListPtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueShort(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueShortPtr(this.m_pManager, valueId);
		}

		public IntPtr GetValueString(IntPtr valueId)
		{
			return OPENZWAVEDLL_GetValueStringPtr(this.m_pManager, valueId);
		}
		
		public void ResetController(UInt32 homeId)
		{
			OPENZWAVEDLL_ResetController(this.m_pManager, homeId);
		}

		public void SoftReset(UInt32 homeId)
		{
			OPENZWAVEDLL_SoftReset(this.m_pManager, homeId);
		}

		public void RequestNodeNeighborUpdate(UInt32 homeId, byte nodeId)
		{
			OPENZWAVEDLL_RequestNodeNeighborUpdate(this.m_pManager, homeId, nodeId);
		}

		public void AssignReturnRoute(UInt32 homeId, byte srcNodeId, byte dstNodeId)
		{
			OPENZWAVEDLL_AssignReturnRoute(this.m_pManager, homeId, srcNodeId, dstNodeId);
		}
		
		public bool AddWatcher(NotificationDelegate onNotify, IntPtr context)
		{
			return OPENZWAVEDLL_AddWatcher(this.m_pManager, onNotify, context);
		}
		
		public bool RemoveWatcher(NotificationDelegate onNotify, IntPtr context)
		{
			return OPENZWAVEDLL_RemoveWatcher(this.m_pManager, onNotify, context);
		}
		
		public void NotifyWatchers(IntPtr pNotification)
		{
			OPENZWAVEDLL_NotifyWatchers(this.m_pManager, pNotification);
		}

		public void BeginAddNode(UInt32 homeId)
		{
			OPENZWAVEDLL_BeginAddNode(this.m_pManager, homeId, false);
		}

		public void BeginAddNode(UInt32 homeId, bool bHighpower)
		{
			OPENZWAVEDLL_BeginAddNode(this.m_pManager, homeId, bHighpower);
		}

		public void BeginAddController(UInt32 homeId)
		{
			OPENZWAVEDLL_BeginAddController(this.m_pManager, homeId, false);
		}

		public void BeginAddController(UInt32 homeId, bool bHighpower)
		{
			OPENZWAVEDLL_BeginAddController(this.m_pManager, homeId, bHighpower);
		}

		public void EndAddNode(UInt32 homeId)
		{
			OPENZWAVEDLL_EndAddNode(this.m_pManager, homeId);
		}

		public void BeginRemoveNode(UInt32 homeId)
		{
			OPENZWAVEDLL_BeginRemoveNode(this.m_pManager, homeId);
		}

		public void EndRemoveNode(UInt32 homeId)
		{
			OPENZWAVEDLL_EndRemoveNode(this.m_pManager, homeId);
		}

		public void BeginReplicateController(UInt32 homeId)
		{
			OPENZWAVEDLL_BeginReplicateController(this.m_pManager, homeId);
		}

		public void EndReplicateController(UInt32 homeId)
		{
			OPENZWAVEDLL_EndReplicateController(this.m_pManager, homeId);
		}

		public void RequestNetworkUpdate(UInt32 homeId)
		{
			OPENZWAVEDLL_RequestNetworkUpdate(this.m_pManager, homeId);
		}

		public void ControllerChange(UInt32 homeId)
		{
			OPENZWAVEDLL_ControllerChange(this.m_pManager, homeId);
		}

        //public void ReadMemory(UInt32 homeId, UInt16 offset)
        //{
        //    OPENZWAVEDLL_ReadMemory(this.m_pManager, homeId, offset);
        //}

        //public void SetConfiguration(UInt32 homeId, byte nodeId, byte _parameter, UInt32 _value)
        //{
        //    OPENZWAVEDLL_SetConfiguration(this.m_pManager, homeId, nodeId, _parameter, _value);
        //}		
		
		#endregion Wrapper Methods
	}
}
