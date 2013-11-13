using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using OpenZWaveDotNet;

namespace OZWForm
{
    public partial class MainForm : Form
    {
        static private ZWOptions m_options = null;
        static public ZWOptions Options
        {
            get { return m_options; }
        }

        static private ZWManager m_manager = null;
        static public ZWManager Manager
        {
            get { return m_manager; }
        }

        private UInt32 m_homeId = 0;
        private ZWNotification m_notification = null;
        private BindingList<Node> m_nodeList = new BindingList<Node>();
        private Byte m_rightClickNode = 0xff;
		private string m_driverPort = string.Empty;

        public MainForm()
        {
            // Initialize the form
            InitializeComponent();

            // Add the columns to the grid view
            // Data Grid
            NodeGridView.AutoGenerateColumns = false;
            NodeGridView.AllowUserToResizeColumns = true;
            NodeGridView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.DisplayedCells;

            DataGridViewTextBoxColumn column;
            //DataGridViewCheckBoxColumn check;

            // Id
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "ID";
            column.Name = "Node";
            column.ReadOnly = true;
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The Z-Wave node ID of the device.\nThis value is not editable.";
            NodeGridView.Columns.Add(column);

            // Location
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Location";
            column.Name = "Location";
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The user-defined location of the Z-Wave device.";
            NodeGridView.Columns.Add(column);

            // Name
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Name";
            column.Name = "Name";
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The user-defined name for the Z-Wave device.";
            NodeGridView.Columns.Add(column);

            // Device Type
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Label";
            column.Name = "Type";
            column.ReadOnly = true;
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The Z-Wave device type.\nThis value is not editable.";
            NodeGridView.Columns.Add(column);

            // Manufacturer
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Manufacturer";
            column.Name = "Manufacturer";
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The manufacturer of the Z-Wave device.";
            NodeGridView.Columns.Add(column);

            // Product
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Product";
            column.Name = "Product";
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            column.ToolTipText = "The product name of the Z-Wave device.";
            NodeGridView.Columns.Add(column);

/*            // Poll Interval
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "PollInterval";
            column.Name = "Poll Interval";
            column.ReadOnly = false;
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "Polling interval in seconds, or zero for no polling.\nNewer devices should not need to be polled for\nyour PC to know their current state.\nFor those that do requre polling, the interval should\nbe as long as possible to reduce network traffic.";
            NodeGridView.Columns.Add(column);
*/
/*            // Schema
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Schema";
            column.Name = "Schema";
            column.ReadOnly = true;
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "The xPL message schema family that will be used\nif the 'Use zwave.basic' option is not checked.\nThe schema is chosen automatically according to\nthe Z-Wave device type, and cannot be changed.";
            NodeGridView.Columns.Add(column);

            // ZWaveBasic
            //check = new DataGridViewCheckBoxColumn();
            //check.DataPropertyName = "ZWaveBasic";
            //check.Name = "Use zwave.basic";
            //check.Frozen = false;
            //check.Resizable = DataGridViewTriState.True;
            //check.SortMode = DataGridViewColumnSortMode.NotSortable;
            //check.ToolTipText = "If the box is checked, the device will send and respond to\nnative zwave.basic messages rather than those of the\ngeneric schema family listed under the Schema column.";
            //NodeGridView.Columns.Add(check);
*/
            // Level
            column = new DataGridViewTextBoxColumn();
            column.DataPropertyName = "Level";
            column.Name = "Level";
            column.Frozen = false;
            column.Resizable = DataGridViewTriState.True;
            column.SortMode = DataGridViewColumnSortMode.NotSortable;
            column.ToolTipText = "Current level of the device";
            NodeGridView.Columns.Add(column);

            // On-Off button
            DataGridViewButtonColumn buttonColumn = new DataGridViewButtonColumn();
            buttonColumn.DataPropertyName = "ButtonText";
            buttonColumn.Name = "Power";
            buttonColumn.Frozen = false;
            buttonColumn.Resizable = DataGridViewTriState.True;
            buttonColumn.SortMode = DataGridViewColumnSortMode.NotSortable;
            buttonColumn.ToolTipText = "Click a button to turn a light on or off";
            NodeGridView.Columns.Add(buttonColumn);

            BindingSource bs = new BindingSource();
            bs.DataSource = m_nodeList;
            NodeGridView.DataSource = bs;

            // Create the Options
            m_options = new ZWOptions();
            m_options.Create(@"..\..\..\..\..\..\..\config\", @"", @"");

            // Add any app specific options here...
			m_options.AddOptionInt("SaveLogLevel", (int)ZWLogLevel.Detail);			// ordinarily, just write "Detail" level messages to the log
			m_options.AddOptionInt("QueueLogLevel", (int)ZWLogLevel.Debug);			// save recent messages with "Debug" level messages to be dumped if an error occurs
			m_options.AddOptionInt("DumpTriggerLevel", (int)ZWLogLevel.Error);		// only "dump" Debug  to the log emessages when an error-level message is logged

            // Lock the options
            m_options.Lock();

            // Create the OpenZWave Manager
            m_manager = new ZWManager();
            m_manager.Create();
            m_manager.OnNotification += new ManagedNotificationsHandler(NotificationHandler);

            // Add a driver
			m_driverPort = @"\\.\COM4";
            m_manager.AddDriver(m_driverPort);
//			m_manager.AddDriver(@"HID Controller", ZWControllerInterface.Hid);
        }

        public void NotificationHandler(ZWNotification notification)
        {
            // Handle the notification on a thread that can safely
            // modify the form controls without throwing an exception.
            m_notification = notification;
            Invoke(new MethodInvoker(NotificationHandler));
            m_notification = null;
        }

        private void NotificationHandler()
        {
            switch (m_notification.GetType())
            {
                case ZWNotification.Type.ValueAdded:
                    {
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
                        if (node != null)
                        {
                            node.AddValue(m_notification.GetValueID());
                        }
                        break;
                    }

                case ZWNotification.Type.ValueRemoved:
                    {
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
                        if (node != null)
                        {
                            node.RemoveValue(m_notification.GetValueID());
                        }
                        break;
                    }

                case ZWNotification.Type.ValueChanged:
                    {
/*						Console.WriteLine("Value Changed");
						ZWValueID v = m_notification.GetValueID();
						Console.WriteLine("  Node : " + v.GetNodeId().ToString());
						Console.WriteLine("  CC   : " + v.GetCommandClassId().ToString());
						Console.WriteLine("  Type : " + v.GetType().ToString());
						Console.WriteLine("  Index: " + v.GetIndex().ToString());
						Console.WriteLine("  Inst : " + v.GetInstance().ToString());
						Console.WriteLine("  Value: " + GetValue(v).ToString());
						Console.WriteLine("  Label: " + m_manager.GetValueLabel(v));
						Console.WriteLine("  Help : " + m_manager.GetValueHelp(v));
						Console.WriteLine("  Units: " + m_manager.GetValueUnits(v));
*/						break;
                    }

                case ZWNotification.Type.Group:
                    {
                        break;
                    }

                case ZWNotification.Type.NodeAdded:
                    {
						// if this node was in zwcfg*.xml, this is the first node notification
						// if not, the NodeNew notification should already have been received
						if (GetNode(m_notification.GetHomeId(), m_notification.GetNodeId()) == null)
						{
							Node node = new Node();
	                        node.ID = m_notification.GetNodeId();
		                    node.HomeID = m_notification.GetHomeId();
			                m_nodeList.Add(node);
						}
                        break;
                    }

				case ZWNotification.Type.NodeNew:
					{
						// Add the new node to our list (and flag as uninitialized)
						Node node = new Node();
						node.ID = m_notification.GetNodeId();
						node.HomeID = m_notification.GetHomeId();
						m_nodeList.Add(node);
						break;
					}

                case ZWNotification.Type.NodeRemoved:
                    {
                        foreach (Node node in m_nodeList)
                        {
                            if (node.ID == m_notification.GetNodeId())
                            {
                                m_nodeList.Remove(node);
                                break;
                            }
                        }
                        break;
                    }

                case ZWNotification.Type.NodeProtocolInfo:
                    {
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
                        if (node != null)
                        {
                            node.Label = m_manager.GetNodeType(m_homeId, node.ID);
                        }
                        break;
                    }

                case ZWNotification.Type.NodeNaming:
                    {
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
                        if (node != null)
                        {
                            node.Manufacturer = m_manager.GetNodeManufacturerName(m_homeId, node.ID);
                            node.Product = m_manager.GetNodeProductName(m_homeId, node.ID);
                            node.Location = m_manager.GetNodeLocation(m_homeId, node.ID);
                            node.Name = m_manager.GetNodeName(m_homeId, node.ID);
                        }
                        break;
                    }

                case ZWNotification.Type.NodeEvent:
                    {
                        break;
                    }

                case ZWNotification.Type.PollingDisabled:
                    {
                        Console.WriteLine("Polling disabled notification");
                        break;
                    }

                case ZWNotification.Type.PollingEnabled:
                    {
                        Console.WriteLine("Polling enabled notification");
                        break;
                    }

                case ZWNotification.Type.DriverReady:
                    {
                        m_homeId = m_notification.GetHomeId();
						toolStripStatusLabel1.Text = "Initializing...driver with Home ID 0x" + m_homeId.ToString("X8") + " is ready.";
						break;
                    }
                case ZWNotification.Type.NodeQueriesComplete:
                    {
						// as an example, enable query of BASIC info (CommandClass = 0x20)
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
                        //if (node != null)
                        //{
                        //    foreach (ZWValueID vid in node.Values)
                        //    {
                        //        if (vid.GetCommandClassId() == 0x84)	// remove this "if" to poll all values
                        //            m_manager.EnablePoll(vid);
                        //    }
                        //}
						toolStripStatusLabel1.Text = "Initializing...node " + node.ID + " query complete.";
						break;
					}
				case ZWNotification.Type.EssentialNodeQueriesComplete:
					{
                        Node node = GetNode(m_notification.GetHomeId(), m_notification.GetNodeId());
						toolStripStatusLabel1.Text = "Initializing...node " + node.ID + " essential queries complete.";
						break;
                    }
                case ZWNotification.Type.AllNodesQueried:
                    {
						toolStripStatusLabel1.Text = "Ready:  All nodes queried.";
						m_manager.WriteConfig(m_notification.GetHomeId());
                        break;
                    }
                case ZWNotification.Type.AllNodesQueriedSomeDead:
                    {
						toolStripStatusLabel1.Text = "Ready:  All nodes queried but some are dead.";
						m_manager.WriteConfig(m_notification.GetHomeId());
                        break;
                    }
                case ZWNotification.Type.AwakeNodesQueried:
                    {
						toolStripStatusLabel1.Text = "Ready:  Awake nodes queried (but not some sleeping nodes).";
						m_manager.WriteConfig(m_notification.GetHomeId());
						break;
                    }
            }

            //NodeGridView.Refresh();
            NodeGridView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.None;
            NodeGridView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.DisplayedCells;
        }

        private Node GetNode(UInt32 homeId, Byte nodeId)
        {
            foreach (Node node in m_nodeList)
            {
                if ((node.ID == nodeId) && (node.HomeID == homeId))
                {
                    return node;
                }
            }

            return null;
        }

        private void SaveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            m_manager.WriteConfig(m_homeId);
        }

        private void NodeGridView_CellMouseDown(object sender, DataGridViewCellMouseEventArgs e)
        {
            if ((e.RowIndex >= 0) && (e.Button == System.Windows.Forms.MouseButtons.Right))
            {
                // Highlight the clicked row
                NodeGridView.Rows[e.RowIndex].Selected = true;

                // Store the index of the selected node
                m_rightClickNode = Convert.ToByte(NodeGridView.Rows[e.RowIndex].Cells["Node"].Value);
            }
        }

        private void PowerOnToolStripMenuItem_Click(object sender, EventArgs e)
        {
            m_manager.SetNodeOn(m_homeId, m_rightClickNode);
        }

        private void PowerOffToolStripMenuItem_Click(object sender, EventArgs e)
        {
            m_manager.SetNodeOff(m_homeId, m_rightClickNode);
        }

        private void hasNodeFailedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.HasNodeFailed);
        }

        private void markNodeAsFailedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.RemoveFailedNode);
        }

        private void replaceFailedNodeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.ReplaceFailedNode);
        }

        private void createNewPrmaryControllerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand( ZWControllerCommand.CreateNewPrimary);
        }

        private void addDeviceToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.AddDevice);
        }

        private void removeDeviceToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.RemoveDevice);
        }

        private void transferPrimaryRoleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.TransferPrimaryRole);
        }

        private void receiveConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.ReceiveConfiguration);
        }

        private void requestNetworkUpdateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.RequestNetworkUpdate);
        }

        private void requestNodeNeighborUpdateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.RequestNodeNeighborUpdate);
        }

        private void assignReturnRouteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.AssignReturnRoute);
        }

        private void deleteReturnRouteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCommand(ZWControllerCommand.DeleteAllReturnRoutes);
        }

        private void DoCommand(ZWControllerCommand command)
        {
            ControllerCommandDlg dlg = new ControllerCommandDlg(this, m_manager, m_homeId, command, m_rightClickNode);
            DialogResult d = dlg.ShowDialog(this);
            dlg.Dispose();
        }

        private void propertiesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Node node = GetNode(m_homeId, m_rightClickNode);
            if (node != null)
            {
                // Wait for refreshed config param values
                ConfigurationWakeUpDlg configDlg = new ConfigurationWakeUpDlg( m_manager, node.HomeID, node.ID);
                if (DialogResult.OK == configDlg.ShowDialog(this))
                {
                    // Show the form
                    NodeForm dlg = new NodeForm(node);
                    dlg.ShowDialog(this);
                    dlg.Dispose();
                }
                configDlg.Dispose();
            }
        }

        private void NodeGridView_CellParsing(object sender, DataGridViewCellParsingEventArgs e)
        {
            if ((e.RowIndex < 0) || (e.ColumnIndex < 0))
            {
                // Invalid cell
                return;
            }

            if (e.ColumnIndex == 1)
            {
                // Location
                Byte nodeId = Convert.ToByte(NodeGridView.Rows[e.RowIndex].Cells["Node"].Value);
                Node node = GetNode(m_homeId, nodeId);
                if (node != null)
                {
                    String newLocation = e.Value.ToString();
                    if (newLocation != node.Location)
                    {
                        m_manager.SetNodeLocation(m_homeId, node.ID, newLocation);
                    }
                }
            }

            if (e.ColumnIndex == 2)
            {
                // Name
                Byte nodeId = Convert.ToByte(NodeGridView.Rows[e.RowIndex].Cells["Node"].Value);
                Node node = GetNode(m_homeId, nodeId);
                if (node != null)
                {
                    String newName = e.Value.ToString();
                    if (newName != node.Name)
                    {
                        m_manager.SetNodeName(m_homeId, node.ID, newName);
                    }
                }
            }
        }

		string GetValue(ZWValueID v)
		{
			switch (v.GetType())
			{
				case ZWValueID.ValueType.Bool:
					bool r1;
					m_manager.GetValueAsBool(v, out r1);
					return r1.ToString();
				case ZWValueID.ValueType.Byte:
					byte r2;
					m_manager.GetValueAsByte(v, out r2);
					return r2.ToString();
				case ZWValueID.ValueType.Decimal:
					decimal r3;
					m_manager.GetValueAsDecimal(v, out r3);
					return r3.ToString();
				case ZWValueID.ValueType.Int:
					Int32 r4;
					m_manager.GetValueAsInt(v, out r4);
					return r4.ToString();
				case ZWValueID.ValueType.List:
					string[] r5;
					m_manager.GetValueListItems(v, out r5);
					string r6 = "";
					foreach (string s in r5)
					{
						r6 += s;
						r6 += "/";
					}
					return r6;
				case ZWValueID.ValueType.Schedule:
					return "Schedule";
				case ZWValueID.ValueType.Short:
					short r7;
					m_manager.GetValueAsShort(v, out r7);
					return r7.ToString();
				case ZWValueID.ValueType.String:
					string r8;
					m_manager.GetValueAsString(v, out r8);
					return r8;
				default:
					return "";
			}
		}

		private void softToolStripMenuItem_Click(object sender, EventArgs e)
		{
			m_manager.SoftReset(m_homeId);
		}

		private void eraseAllToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (DialogResult.Yes == MessageBox.Show("Are you sure you want to fully reset the controller?  This will delete all network information and require re-including all nodes.", "Hard Reset", MessageBoxButtons.YesNo))
			{
				m_manager.ResetController(m_homeId);
				m_manager.RemoveDriver(m_driverPort);
				m_manager.AddDriver(m_driverPort);
			}
		}
    }
}
