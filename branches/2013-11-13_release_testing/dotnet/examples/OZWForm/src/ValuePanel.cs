using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using OpenZWaveDotNet;

namespace OZWForm
{
    public partial class ValuePanel : UserControl
    {
        private ZWValueID m_valueID;
        public ZWValueID ValueID
        {
            get { return m_valueID; }
        }

        private bool m_sendChanges = false;
        public bool SendChanges
        {
            get { return m_sendChanges; }
            set { m_sendChanges = value; }
        }

        private ValuePanel()
        {
        }

        public ValuePanel( ZWValueID valueID )
        {
            m_valueID = valueID;
            InitializeComponent();
        }
    }
}
