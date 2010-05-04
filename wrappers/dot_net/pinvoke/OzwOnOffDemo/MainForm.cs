/*
 * Created by SharpDevelop.
 * User: Doug Brown
 * Date: 4/15/2010
 * Time: 7:39 AM
 * 
 * Copyright 2010 Syanpsity-AZ
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;
using System.Reflection;
using OpenZWaveWrapper;
using SynapsityAZ.Utils;

namespace OzwOnOffDemo
{
	/// <summary>
	/// Description of MainForm.
	/// </summary>
	public partial class MainForm : Form
	{
		private byte m_switchNode = 0;
		private UInt32 m_homeId = 0;
		readonly object DialogLock = new object();
		public byte SwitchNode
		{
			get { lock(DialogLock) { return m_switchNode; } }
			set { lock(DialogLock) { m_switchNode = value; } }
		}
		public UInt32 HomeId
		{
			get { lock(DialogLock) { return m_homeId; } }
			set { lock(DialogLock) { m_homeId = value; } }
		}
		
		private static ZWaveManager manager;
		private static Queue<ZWaveNotification> sharedQueue = new Queue<ZWaveNotification>();

		private ArrayList nodeArray = new ArrayList();
		private Thread queMonThread;
		
//		public struct NOTIFYSTRUCT
//		{
//			public ZWaveNotification.NotificationType m_type;
//			public UInt32 m_homeId;
//			public byte m_nodeId;
//			public ZWaveValueId m_value;
//		};

		public MainForm()
		{
			//
			// The InitializeComponent() call is required for Windows Forms designer support.
			//
			InitializeComponent();
			
 			queMonThread = new Thread(QueMonitor);
 			queMonThread.Start();
			
			Log.Create(@"UnmanagedTestLog.txt", Log.Lvl.Debug);
			ZWaveManager.NotificationDelegate NotifyCb = new ZWaveManager.NotificationDelegate(OnNotify);
			manager = new ZWaveManager(@"../../../../config/", "");

			manager.AddWatcher(NotifyCb, IntPtr.Zero);
			manager.AddDriver("COM3");

		}

		private void QueMonitor()
		{
			bool bFirstSwitchFound = false;
			while (true)
			{
				ZWaveNotification notification;
				lock (sharedQueue)
				{
					while (sharedQueue.Count == 0)
						Monitor.Wait(sharedQueue);
					notification = sharedQueue.Dequeue();
				}
				
//				Log.Write(Log.Lvl.Debug, "RCV: Node: " + notification.NodeId.ToString() +
//				          " type: " + notification.Type.ToString() +
//				          " value type: " + notification.ValueId.Type.ToString() +
//				          " CC: " + notification.ValueId.CommandClassId.ToString() +
//				          " Genre: " + notification.ValueId.Genre.ToString());
//				          " GroupdIdx: " + notification.GroupIdx.ToString());
//					      " Status: " + notification.Status.ToString());
				
				lock (nodeArray)
				{
					NodeInfo nI = new NodeInfo(notification.HomeId, notification.NodeId);
					int nIdx = nodeArray.IndexOf(nI);
					switch (notification.Type)
					{
						case ZWaveNotification.NotificationType.Type_ValueAdded:
							// If the homeid/nodeid is in the node array, add
							// this new valueId
							if (nIdx != -1)
							{
								nI = (NodeInfo) nodeArray[nIdx];
								nI.AddValue(notification.ValueId);
								if (notification.ValueId.Type == ZWaveValueId.ValueType.ValueType_Bool)
								{
									// Found a new switch, update the form
									if (!bFirstSwitchFound)
									{
										SetControlPropertyThreadSafe(this.NodeCntLabel, "Text", 
										                             "Switches Found: " + notification.NodeId.ToString());
										bFirstSwitchFound = true;
										                            
									}
									else
									{
										String strTmp = (String) GetControlPropertyThreadSafe(this.NodeCntLabel, "Text");
										SetControlPropertyThreadSafe(this.NodeCntLabel, "Text", 
										                             strTmp + ", " + notification.NodeId.ToString());										
									}
										
								}
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_ValueRemoved:
							// If the homeid/nodeid is in the dictionary, remove
							// this valueId if it's on the list
							if (nIdx != -1)
							{
								nI = (NodeInfo) nodeArray[nIdx];
								nI.RemoveValue(notification.ValueId);
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_ValueChanged:
							
							// If there's a node number in the text box, see if this value is for it.
							String strNode = (String) GetControlPropertyThreadSafe(this.nodeTextBox, "Text");
							UInt32 editBoxNode = 256;
							if (strNode.Length > 0) 
							{
								editBoxNode = Convert.ToUInt32(strNode);
							}
							if ( editBoxNode == (UInt32) notification.NodeId )
							{
								// See if the homeid/nodeid is in the dictionary, get the new value
								if (nIdx != -1)
								{
									if (notification.ValueId.Type == ZWaveValueId.ValueType.ValueType_Bool)
									{
										IntPtr bValPtr = manager.GetValueBool(notification.ValueId.ValueID);
										if (bValPtr != IntPtr.Zero)
										{
											ZWaveValueBool bValObj = new ZWaveValueBool(bValPtr);
											bool bVal = bValObj.GetValue();
											// Set the radio button depending on value returned from switch
											//
											if (bVal)
											{
												SetControlPropertyThreadSafe(this.OnButton, "Enabled", false);
												SetControlPropertyThreadSafe(this.OffButton, "Enabled", true);
											}
											else
											{
												SetControlPropertyThreadSafe(this.OnButton, "Enabled", true);
												SetControlPropertyThreadSafe(this.OffButton, "Enabled", false);
											}
										}
									}
								}
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_Group:
							// If the homeid/nodeid is in the dictionary, TBD
							if (nIdx != -1)
							{
								// TBD
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_NodeAdded:
							// If the homeid/nodeid is not in the dictionary then add it
							if (nIdx == -1)
							{
								nodeArray.Add(nI);
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_NodeRemoved:
							// If the homeid/nodeid is in the dictionary then remove it
							if (nIdx != -1)
							{
								nodeArray.Remove(nI);
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_NodeStatus:
							// If the homeid/nodeid is in the dictionary then TBD
							if (nIdx != -1)
							{
								// We have received an event from the node, caused by a
								// basic_set or hail message.
								// TBD...
								Log.Write(Log.Lvl.Debug, "Received Status notification from Node " + notification.NodeId.ToString());
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_PollingEnabled:
							// If the homeid/nodeid is in the dictionary then TBD
							Log.Write(Log.Lvl.Debug, "Received PollEnable notification");
							if (nIdx != -1)
							{
								nI = (NodeInfo) nodeArray[nIdx];
								nI.isPolled = true;
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_PollingDisabled:
							// If the homeid/nodeid is in the dictionary then TBD
							Log.Write(Log.Lvl.Debug, "Received PollDisable notification ");
							if (nIdx != -1)
							{
								nI = (NodeInfo) nodeArray[nIdx];
								nI.isPolled = false;
							}
							break;
							
						case ZWaveNotification.NotificationType.Type_DriverReady:
							HomeId = notification.HomeId;
							SetControlPropertyThreadSafe(this.tbHomeID, "Text",
							                             HomeId.ToString("X8"));
							break;

						default:
							break;
					} // Switch
				} // Lock
			}
		}
		
		public static void OnNotify(IntPtr pNotification, IntPtr pContext)
		{
			ZWaveNotification notification = new ZWaveNotification(pNotification);
			lock (sharedQueue)
			{
				sharedQueue.Enqueue(notification);
				Monitor.Pulse(sharedQueue);
			}
		}
		
		private delegate void SetControlPropertyThreadSafeDelegate(Control control,
		                                                           string propertyName, object propertyValue);

		public static void SetControlPropertyThreadSafe(Control control,
		                                                string propertyName, object propertyValue)
		{
			if (control.InvokeRequired)
			{
				control.BeginInvoke(new SetControlPropertyThreadSafeDelegate(SetControlPropertyThreadSafe),
				                    new object[] { control, propertyName, propertyValue });
			}
			else
			{
				control.GetType().InvokeMember(propertyName, BindingFlags.SetProperty, null, control,
				                               new object[] { propertyValue });
			}
		}
		
		private delegate Object GetControlPropertyThreadSafeDelegate(Control control,
		                                                           string propertyName);

		public static Object GetControlPropertyThreadSafe(Control control,
		                                                string propertyName)
		{
			if (control.InvokeRequired)
			{
				return control.Invoke(new GetControlPropertyThreadSafeDelegate(GetControlPropertyThreadSafe),
				                    new object[] { control, propertyName });
			}
			else
			{
				return control.GetType().InvokeMember(propertyName, BindingFlags.GetProperty, null, control, null );
			}
		}
		
		private bool GetValueID(ZWaveValueId.ValueType _type, out ZWaveValueId _zwVID)
		{
			// Get Node from form
			_zwVID = new ZWaveValueId(IntPtr.Zero);
			bool bRet = false;
			byte nodeId = 0;
			{
				try {
					nodeId = Convert.ToByte(this.nodeTextBox.Text);
				}
				catch (Exception) {
					MessageBox.Show ("Invalid Node", "Invalid Entry",
					                 MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
					return false;
				}
				
				lock (nodeArray)
				{
					int nIdx = nodeArray.IndexOf(new NodeInfo(HomeId, nodeId));
					if (nIdx != 1)
					{
						// Found the right node, see if it contains the correct type
						NodeInfo nI = (NodeInfo) nodeArray[nIdx];
						bRet = nI.GetType(_type, out _zwVID);
					}
					else
						Log.Write(Log.Lvl.Debug,"Could not find NodeInfo for Node: " + nodeId.ToString() + " HomeID: " + HomeId.ToString("X8"));
				} // lock
				return bRet;				
			}
		}
				
		void StatusButtonClick(object sender, EventArgs e)
		{
			String strNode = (String) GetControlPropertyThreadSafe(this.nodeTextBox, "Text");
			manager.RequestState(HomeId, Convert.ToByte(strNode));
		}
		
		void ExitButtonClick(object sender, EventArgs e)
		{
			manager.Dispose();
			queMonThread.Abort();
			Application.Exit();
		}
		
		
		void OnButtonClick(object sender, EventArgs e)
		{
			// Silently return if nothing in node text box.
			if (this.nodeTextBox.Text.Length == 0) return;
			ZWaveValueId zwVID;
			// See if this node has a boolean type
			if (GetValueID(ZWaveValueId.ValueType.ValueType_Bool, out zwVID))
			{
				// Get ValueBool
				IntPtr bValPtr = manager.GetValueBool(zwVID.ValueID);
				if (bValPtr != IntPtr.Zero)
				{
					ZWaveValueBool bValObj = new ZWaveValueBool(bValPtr);
					bValObj.Set(true);
					this.OnButton.Enabled = false;
					this.OffButton.Enabled = true;
				}
				else
				{
					Log.Write(Log.Lvl.Debug, "Bool Value Pointer is null");
				}
			}
			else
			{
				MessageBox.Show ("Node is not a switch", "Invalid Entry",
				                 MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
			}
		}
		
		void OffButtonClick(object sender, EventArgs e)
		{
			// Silently return if nothing in node text box.
			if (this.nodeTextBox.Text.Length == 0) return;
			ZWaveValueId zwVID;
			// See if this node has a boolean type
			if (GetValueID(ZWaveValueId.ValueType.ValueType_Bool, out zwVID))
			{
				// Get ValueBool
				IntPtr bValPtr = manager.GetValueBool(zwVID.ValueID);
				if (bValPtr != IntPtr.Zero)
				{
					ZWaveValueBool bValObj = new ZWaveValueBool(bValPtr);
					bValObj.Set(false);
					this.OnButton.Enabled = true;
					this.OffButton.Enabled = false;
				}
				else
				{
					Log.Write(Log.Lvl.Debug, "Bool Value Pointer is null");
				}
			}
			else
			{
				MessageBox.Show ("Node is not a switch", "Invalid Entry",
				                 MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
			}
		}
		
		void BeginRepButtonClick(object sender, EventArgs e)
		{
			manager.BeginReplicateController(HomeId);
		}
		
		void EndRepButtonClick(object sender, EventArgs e)
		{
			manager.EndReplicateController(HomeId);
		}
		
		void ResetCtrlButtonClick(object sender, EventArgs e)
		{
			manager.ResetController(HomeId);
		}
		
		void SoftResetButtonClick(object sender, EventArgs e)
		{
			manager.SoftReset(HomeId);
			
		}
	}
}
