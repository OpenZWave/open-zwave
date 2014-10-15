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

			// load all possible configuration parameters
			for (byte i = 0; i <= 10; i++)
			{
				MainForm.Manager.RequestConfigParam(node.HomeID, node.ID, i);
			}

			if (node.Manufacturer != "")
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
                switch (valueID.GetType())
                {
                    case ZWValueID.ValueType.Bool:
                    {
                        control = new ValuePanelBool(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.Button:
                    {
                        control = new ValuePanelButton(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.Byte:
                    {
                        control = new ValuePanelByte(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.Decimal:
                    {
                        control = new ValuePanelDecimal(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.Int:
                    {
                        control = new ValuePanelInt(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.List:
                    {
                        control = new ValuePanelList(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.Short:
                    {
                        control = new ValuePanelShort(valueID);
                        break;
                    }
                    case ZWValueID.ValueType.String:
                    {
                        control = new ValuePanelString(valueID);
                        break;
                    }
                }

                if (control != null)
                {
                    NodeLayoutPanel.Controls.Add(control);
                }
            }

        }
	}
}
