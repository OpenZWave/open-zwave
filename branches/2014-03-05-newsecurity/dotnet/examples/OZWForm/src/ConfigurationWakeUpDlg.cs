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
	public partial class ConfigurationWakeUpDlg : Form
	{
        static private ZWManager m_manager;
        static private UInt32 m_homeId;
        private ZWNotification m_notification = null;
        static private Byte m_nodeId;

        public ConfigurationWakeUpDlg( ZWManager _manager, UInt32 homeId, Byte nodeId)
		{
            m_manager = _manager;
            m_homeId = homeId;
            m_nodeId = nodeId;

            InitializeComponent();

            // Set the text according to whether the device is listening
            if( m_manager.IsNodeListeningDevice( homeId, nodeId ) )
            {
                label1.Text = "Waiting for configurable parameter info from device...";
            }
            else
            {
                label1.Text = "Waiting for configurable parameter info from device.\r\nPlease ensure device is awake...";
            }
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
            // Check whether all the queries on this node have completed
            if( m_notification.GetType() == ZWNotification.Type.NodeQueriesComplete )
            {
                if ((m_notification.GetHomeId() == m_homeId) && (m_notification.GetNodeId() == m_nodeId))
                {
                    // Done!
					m_manager.OnNotification -= new ManagedNotificationsHandler(NotificationHandler);
					DialogResult = DialogResult.OK;
				}
            }
        }

        private void ConfigurationWakeUpDlg_FormClosing(object sender, FormClosingEventArgs e)
        {
        }

		private void ConfigurationWakeUpDlg_Shown(object sender, EventArgs e)
		{
			// Add a handler so that we receive notification of when the node queries are complete.
			m_manager.OnNotification += new ManagedNotificationsHandler(NotificationHandler);

			// Request refreshed config param values.
			m_manager.RequestAllConfigParams(m_homeId, m_nodeId);
		}
 	}
}