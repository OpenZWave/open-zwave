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
    public partial class NodeForm : Form
    {
        private Node m_node;

        public NodeForm( Node node )
        {
            m_node = node;
            InitializeComponent();

            if( node.Manufacturer != "" )
            {
                this.Text = "Node " + node.ID.ToString() + ": " + node.Manufacturer + " " + node.Product;
            }
            else
            {
                this.Text = "Node " + node.ID.ToString() + ": " + node.Label;
            }

            foreach (ZWValueID valueID in node.Values)
            {
                Control control = null;
                if (valueID.GetType() == ZWValueID.ValueType.Bool)
                {
                    control = new ValuePanelBool(valueID);
                }
                else if (valueID.GetType() == ZWValueID.ValueType.Byte)
                {
                    control = new ValuePanelByte(valueID);
                }
                if (control != null)
                {
                    NodeLayoutPanel.Controls.Add(control);
                }
            }
        }
    }
}
