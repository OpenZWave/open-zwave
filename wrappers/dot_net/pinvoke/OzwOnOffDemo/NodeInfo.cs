/*
 * Created by SharpDevelop.
 * User: Doug Brown
 * Date: 4/15/2010
 * Time: 2:30 PM
 * 
 * Copyright 2010 Syanpsity-AZ
 */
using System;
using System.Collections.Generic;
using OpenZWaveWrapper;
using SynapsityAZ.Utils;

namespace OzwOnOffDemo
{
	/// <summary>
	/// Description of NodeInfo.
	/// </summary>
	public class NodeInfo
	{
		private UInt32 m_homeId;
		private byte m_nodeId;
		private List<ZWaveValueId> m_values = new List<ZWaveValueId>();
		
		private NodeInfo() {} // Not Implemented
		
		public NodeInfo(UInt32 homeId, byte nodeId)
		{
			m_homeId = homeId;
		    m_nodeId = nodeId;
		    isPolled = false;
		}
		    
		public UInt32 HomeId{
			get { return m_homeId; }
		}
		
		public byte NodeId {
			get { return m_nodeId; }
		}
		
		public List<ZWaveValueId> Values {
			get { return m_values; }
		}
		
		public bool isPolled { get; set; }
		
		public static bool operator ==(NodeInfo n1, NodeInfo n2)
		{
			if ((n1.HomeId == n2.HomeId) && (n1.NodeId == n2.NodeId)) 
				return true;
			else
				return false;
		}
		
		public static bool operator !=(NodeInfo n1, NodeInfo n2)
		{
			if ((n1.HomeId != n2.HomeId) || (n1.NodeId != n2.NodeId)) 
				return true;
			else
				return false;
		}
		
		public override bool Equals(object o2)
		{
			NodeInfo n2 = (NodeInfo) o2;
			return (this == n2);
		}
		
		public override int GetHashCode()
		{
			return m_nodeId.GetHashCode() ^ m_homeId.GetHashCode();
		}
		
		public void AddValue(ZWaveValueId id)
		{
			m_values.Add(id);
		}
		
		public bool RemoveValue(ZWaveValueId id)
		{
			return m_values.Remove(id);
		}
		
		// Look for the given type (_type) in the list of
		// valueIds that this node knows about.  If not found return
		// false.  If false is returned the returned ValueID will be the 
		// first one on the list.
		public bool GetType(ZWaveValueId.ValueType _type, out ZWaveValueId _zwId)
		{
			foreach(ZWaveValueId zwId in m_values)
			{
				if (zwId.Type == _type) 
				{
					_zwId = zwId;
					return true;
				}
			}
			_zwId = m_values[0];
			return false;
		}
	}
}
