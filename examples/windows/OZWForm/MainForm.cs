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
        private ZWManager m_manager = null;
        private UInt32 m_homeId = 0;
        private ZWNotification m_notification = null;
        private BindingList<Node> m_nodeList = new BindingList<Node>();

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

            //// Poll Interval
            //column = new DataGridViewTextBoxColumn();
            //column.DataPropertyName = "PollInterval";
            //column.Name = "Poll Interval";
            //column.ReadOnly = false;
            //column.Frozen = false;
            //column.Resizable = DataGridViewTriState.True;
            //column.SortMode = DataGridViewColumnSortMode.NotSortable;
            //column.ToolTipText = "Polling interval in seconds, or zero for no polling.\nNewer devices should not need to be polled for\nyour PC to know their current state.\nFor those that do requre polling, the interval should\nbe as long as possible to reduce network traffic.";
            //dataGridView1.Columns.Add(column);

            //// Schema
            //column = new DataGridViewTextBoxColumn();
            //column.DataPropertyName = "Schema";
            //column.Name = "Schema";
            //column.ReadOnly = true;
            //column.Frozen = false;
            //column.Resizable = DataGridViewTriState.True;
            //column.SortMode = DataGridViewColumnSortMode.NotSortable;
            //column.ToolTipText = "The xPL message schema family that will be used\nif the 'Use zwave.basic' option is not checked.\nThe schema is chosen automatically according to\nthe Z-Wave device type, and cannot be changed.";
            //dataGridView1.Columns.Add(column);

            //// ZWaveBasic
            //check = new DataGridViewCheckBoxColumn();
            //check.DataPropertyName = "ZWaveBasic";
            //check.Name = "Use zwave.basic";
            //check.Frozen = false;
            //check.Resizable = DataGridViewTriState.True;
            //check.SortMode = DataGridViewColumnSortMode.NotSortable;
            //check.ToolTipText = "If the box is checked, the device will send and respond to\nnative zwave.basic messages rather than those of the\ngeneric schema family listed under the Schema column.";
            //dataGridView1.Columns.Add(check);

            //// Level
            //column = new DataGridViewTextBoxColumn();
            //column.DataPropertyName = "Level";
            //column.Name = "Level";
            //column.ReadOnly = false;
            //column.Frozen = false;
            //column.Resizable = DataGridViewTriState.True;
            //column.SortMode = DataGridViewColumnSortMode.NotSortable;
            //column.ToolTipText = "Current level of the device";
            //dataGridView1.Columns.Add(column);

            //// On-Off button
            //DataGridViewButtonColumn buttonColumn = new DataGridViewButtonColumn();
            //buttonColumn.DataPropertyName = "ButtonText";
            //buttonColumn.Name = "Power";
            //buttonColumn.ReadOnly = false;
            //buttonColumn.Frozen = false;
            //buttonColumn.Resizable = DataGridViewTriState.True;
            //buttonColumn.SortMode = DataGridViewColumnSortMode.NotSortable;
            //buttonColumn.ToolTipText = "Click a button to turn a light on or off";
            //dataGridView1.Columns.Add(buttonColumn);

            BindingSource bs = new BindingSource();
            bs.DataSource = m_nodeList;
            NodeGridView.DataSource = bs;


            // Create the OpenZWave Manager
            m_manager = new ZWManager();
            m_manager.Create(@"F:\Projects\OpenZWave\config\", @"");
            m_manager.m_event += new ManagedWatchersHandler(NotificationHandler);
            
            // Add a driver
            m_manager.AddDriver(@"\\.\COM3");
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
			        break;
		        }

		        case ZWNotification.Type.ValueRemoved:
		        {
			        break;
		        }

		        case ZWNotification.Type.ValueChanged:
		        {
			        break;
		        }

		        case ZWNotification.Type.Group:
		        {
			        break;
		        }

		        case ZWNotification.Type.NodeAdded:
		        {
			        // Add the new node to our list
                    Node node = new Node();
                    node.ID = m_notification.GetNodeId();
                    node.HomeID = m_notification.GetHomeId();
                    m_nodeList.Add( node );
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
                    }
                    break;
                }

                case ZWNotification.Type.NodeStatus:
		        {
			        break;
		        }

		        case ZWNotification.Type.PollingDisabled:
		        {
			        break;
		        }

		        case ZWNotification.Type.PollingEnabled:
		        {
			        break;
		        }

		        case ZWNotification.Type.DriverReady:
		        {
                    m_homeId = m_notification.GetHomeId();
			        break;
		        }
	        }

            //NodeGridView.Refresh();
            NodeGridView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.None;
            NodeGridView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.DisplayedCells;
        }

        private Node GetNode( UInt32 homeId, Byte nodeId )
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
    }
}
