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
	public partial class ControllerCommandDlg : Form
	{
        static private ZWManager m_manager;
        static private ControllerCommandDlg m_dlg;
        static private UInt32 m_homeId;
        static private ManagedControllerStateChangedHandler m_controllerStateChangedHandler = new ManagedControllerStateChangedHandler(ControllerCommandDlg.MyControllerStateChangedHandler);
		static private ZWControllerCommand m_op;
        static private Byte m_nodeId;
		static private DialogResult result;

		private MainForm m_mainDlg;
		public MainForm MainDlg
		{
			get { return m_mainDlg; }
		}

        public ControllerCommandDlg(MainForm _mainDlg, ZWManager _manager, UInt32 homeId, ZWControllerCommand _op, Byte nodeId)
		{
            m_mainDlg = _mainDlg;
            m_manager = _manager;
            m_homeId = homeId;
            m_op = _op;
            m_nodeId = nodeId;
            m_dlg = this;

            InitializeComponent();

			switch (m_op)
			{
				case ZWControllerCommand.RequestNodeNeighborUpdate:
					{
						this.Text = "Node Neighbor Update";
						this.label1.Text = "Request that a node update its list of neighbors.";
						break;
					}

                case ZWControllerCommand.AddDevice:
				{
					this.Text = "Add Device";
					this.label1.Text = "Press the program button on the Z-Wave device to add it to the network.\nFor security reasons, the PC Z-Wave Controller must be close to the device being added.";
					break;
				}
                case ZWControllerCommand.CreateNewPrimary:
				{
					this.Text = "Create New Primary Controller";
					this.label1.Text = "Put the target controller into receive configuration mode.\nThe PC Z-Wave Controller must be within 2m of the controller that is being made the primary.";
					break;
				}
                case ZWControllerCommand.ReceiveConfiguration:
				{
					this.Text = "Receive Configuration";
					this.label1.Text = "Transfering the network configuration\nfrom another controller.\n\nPlease bring the other controller within 2m of the PC controller and set it to send its network configuration.";
					break;
				}
                case ZWControllerCommand.RemoveDevice:
				{
					this.Text = "Remove Device";
					this.label1.Text = "Press the program button on the Z-Wave device to remove it from the network.\nFor security reasons, the PC Z-Wave Controller must be close to the device being removed.";
					break;
				}
                case ZWControllerCommand.TransferPrimaryRole:
				{
					this.Text = "Transfer Primary Role";
					this.label1.Text = "Transfering the primary role\nto another controller.\n\nPlease bring the new controller within 2m of the PC controller and set it to receive the network configuration.";
					break;
				}
                case ZWControllerCommand.HasNodeFailed:
                {
                    this.ButtonCancel.Enabled = false;
                    this.Text = "Has Node Failed";
                    this.label1.Text = "Testing whether the node has failed.\nThis command cannot be cancelled.";
                    break;
                }
                case ZWControllerCommand.RemoveFailedNode:
                {
                    this.ButtonCancel.Enabled = false;
                    this.Text = "Remove Failed Node";
                    this.label1.Text = "Removing the failed node from the controller's list.\nThis command cannot be cancelled.";
					break;
                }
                case ZWControllerCommand.ReplaceFailedNode:
                {
                    this.ButtonCancel.Enabled = false;
                    this.Text = "Replacing Failed Node";
                    this.label1.Text = "Testing the failed node.\nThis command cannot be cancelled.";
                    break;
                }
            }

            m_manager.OnControllerStateChanged += m_controllerStateChangedHandler;
            if (!m_manager.BeginControllerCommand(m_homeId, m_op, false, m_nodeId))
            {
                m_manager.OnControllerStateChanged -= m_controllerStateChangedHandler;
            }
		}

        public static void MyControllerStateChangedHandler( ZWControllerState state )
	    {
		    // Handle the controller state notifications here.
            bool complete = false;
            String dlgText = "";
            bool buttonEnabled = true;

            switch (state)
		    {
		        case ZWControllerState.Waiting:
		        {
	                // Display a message to tell the user to press the include button on the controller
                    if (m_op == ZWControllerCommand.ReplaceFailedNode)
                    {
                        dlgText = "Press the program button on the replacement Z-Wave device to add it to the network.\nFor security reasons, the PC Z-Wave Controller must be close to the device being added.\nThis command cannot be cancelled.";
                    }
                    break;
		        }
		        case ZWControllerState.InProgress:
		        {
		            // Tell the user that the controller has been found and the adding process is in progress.
                    dlgText = "Please wait...";
                    buttonEnabled = false;
                    break;
		        }
		        case ZWControllerState.Completed:
		        {
		            // Tell the user that the controller has been successfully added.
		            // The command is now complete
                    dlgText = "Command Completed OK.";
                    complete = true;
					result = DialogResult.OK;
		            break;
		        }
		        case ZWControllerState.Failed:
		        {
		            // Tell the user that the controller addition process has failed.
		            // The command is now complete
                    dlgText = "Command Failed.";
                    complete = true;
					result = DialogResult.Abort;
					break;
		        }
                case ZWControllerState.NodeOK:
                {
                    dlgText = "Node has not failed.";
                    complete = true;
					result = DialogResult.No;
					break;
                }
                case ZWControllerState.NodeFailed:
                {
                    dlgText = "Node has failed.";
                    complete = true;
					result = DialogResult.Yes;
					break;
                }
		    }

            if (dlgText != "")
            {
                m_dlg.SetDialogText(dlgText);
            }

            m_dlg.SetButtonEnabled(buttonEnabled);

            if (complete)
            {
                m_dlg.SetButtonText( "OK" );

                // Remove the event handler
                m_manager.OnControllerStateChanged -= m_controllerStateChangedHandler;
            }
		}

        private void SetDialogText(String text)
        {
            if (m_dlg.InvokeRequired)
            {
                Invoke(new MethodInvoker(delegate() { SetDialogText(text); }));
            }
            else
            {
                m_dlg.label1.Text = text;
            }
        }

        private void SetButtonText(String text)
        {
            if (m_dlg.InvokeRequired)
            {
                Invoke(new MethodInvoker(delegate() { SetButtonText(text); }));
            }
            else
            {
                m_dlg.ButtonCancel.Text = text;
            }
        }

        private void SetButtonEnabled(bool enabled)
        {
            if (m_dlg.InvokeRequired)
            {
                Invoke(new MethodInvoker(delegate() { SetButtonEnabled(enabled); }));
            }
            else
            {
                m_dlg.ButtonCancel.Enabled = enabled;
            }
        }

		private void ButtonCancel_Click( object sender, EventArgs e )
		{
            if (ButtonCancel.Text != "OK")
            {
                // Remove the event handler
                m_manager.OnControllerStateChanged -= m_controllerStateChangedHandler;

                // Cancel the operation
                m_manager.CancelControllerCommand(m_homeId);
            }

			// Close the dialog
			Close();
			m_dlg.DialogResult = result;
		}
 	}
}
