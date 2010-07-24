namespace OZWForm
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.NodeGridView = new System.Windows.Forms.DataGridView();
            this.NodeContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.PowerOnToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.PowerOffToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.requestNodeNeighborUpdateToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.MenuBar = new System.Windows.Forms.MenuStrip();
            this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.SaveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.controllerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createNewPrmaryControllerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addControllerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.addDeviceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.removeControllerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.removeDeviceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.transferPrimaryRoleToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.receiveConfigurationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.replaceFailedDeviceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.cancelControllerCommandToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            ((System.ComponentModel.ISupportInitialize)(this.NodeGridView)).BeginInit();
            this.NodeContextMenuStrip.SuspendLayout();
            this.MenuBar.SuspendLayout();
            this.SuspendLayout();
            // 
            // NodeGridView
            // 
            this.NodeGridView.AllowUserToAddRows = false;
            this.NodeGridView.AllowUserToDeleteRows = false;
            this.NodeGridView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.NodeGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.NodeGridView.ContextMenuStrip = this.NodeContextMenuStrip;
            this.NodeGridView.Location = new System.Drawing.Point(13, 37);
            this.NodeGridView.MultiSelect = false;
            this.NodeGridView.Name = "NodeGridView";
            this.NodeGridView.ReadOnly = true;
            this.NodeGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.NodeGridView.Size = new System.Drawing.Size(609, 343);
            this.NodeGridView.TabIndex = 0;
            this.NodeGridView.CellMouseDown += new System.Windows.Forms.DataGridViewCellMouseEventHandler(this.NodeGridView_CellMouseDown);
            // 
            // NodeContextMenuStrip
            // 
            this.NodeContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.PowerOnToolStripMenuItem,
            this.PowerOffToolStripMenuItem,
            this.toolStripSeparator4,
            this.requestNodeNeighborUpdateToolStripMenuItem});
            this.NodeContextMenuStrip.Name = "NodeContextMenuStrip";
            this.NodeContextMenuStrip.Size = new System.Drawing.Size(243, 76);
            // 
            // PowerOnToolStripMenuItem
            // 
            this.PowerOnToolStripMenuItem.Name = "PowerOnToolStripMenuItem";
            this.PowerOnToolStripMenuItem.Size = new System.Drawing.Size(242, 22);
            this.PowerOnToolStripMenuItem.Text = "Power On";
            this.PowerOnToolStripMenuItem.Click += new System.EventHandler(this.PowerOnToolStripMenuItem_Click);
            // 
            // PowerOffToolStripMenuItem
            // 
            this.PowerOffToolStripMenuItem.Name = "PowerOffToolStripMenuItem";
            this.PowerOffToolStripMenuItem.Size = new System.Drawing.Size(242, 22);
            this.PowerOffToolStripMenuItem.Text = "Power Off";
            this.PowerOffToolStripMenuItem.Click += new System.EventHandler(this.PowerOffToolStripMenuItem_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(239, 6);
            // 
            // requestNodeNeighborUpdateToolStripMenuItem
            // 
            this.requestNodeNeighborUpdateToolStripMenuItem.Name = "requestNodeNeighborUpdateToolStripMenuItem";
            this.requestNodeNeighborUpdateToolStripMenuItem.Size = new System.Drawing.Size(242, 22);
            this.requestNodeNeighborUpdateToolStripMenuItem.Text = "Request Node Neighbor Update";
            this.requestNodeNeighborUpdateToolStripMenuItem.Click += new System.EventHandler(this.RequestNodeNeighborUpdateToolStripMenuItem_Click);
            // 
            // MenuBar
            // 
            this.MenuBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.controllerToolStripMenuItem});
            this.MenuBar.Location = new System.Drawing.Point(0, 0);
            this.MenuBar.Name = "MenuBar";
            this.MenuBar.Size = new System.Drawing.Size(634, 24);
            this.MenuBar.TabIndex = 1;
            this.MenuBar.Text = "menuStrip1";
            // 
            // FileToolStripMenuItem
            // 
            this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.SaveToolStripMenuItem});
            this.FileToolStripMenuItem.Name = "FileToolStripMenuItem";
            this.FileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.FileToolStripMenuItem.Text = "&File";
            // 
            // SaveToolStripMenuItem
            // 
            this.SaveToolStripMenuItem.Name = "SaveToolStripMenuItem";
            this.SaveToolStripMenuItem.Size = new System.Drawing.Size(98, 22);
            this.SaveToolStripMenuItem.Text = "&Save";
            this.SaveToolStripMenuItem.Click += new System.EventHandler(this.SaveToolStripMenuItem_Click);
            // 
            // controllerToolStripMenuItem
            // 
            this.controllerToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.createNewPrmaryControllerToolStripMenuItem,
            this.addControllerToolStripMenuItem,
            this.addDeviceToolStripMenuItem,
            this.toolStripSeparator1,
            this.removeControllerToolStripMenuItem,
            this.removeDeviceToolStripMenuItem,
            this.toolStripSeparator2,
            this.transferPrimaryRoleToolStripMenuItem,
            this.receiveConfigurationToolStripMenuItem,
            this.toolStripSeparator3,
            this.replaceFailedDeviceToolStripMenuItem,
            this.toolStripSeparator5,
            this.cancelControllerCommandToolStripMenuItem});
            this.controllerToolStripMenuItem.Name = "controllerToolStripMenuItem";
            this.controllerToolStripMenuItem.Size = new System.Drawing.Size(72, 20);
            this.controllerToolStripMenuItem.Text = "Controller";
            // 
            // createNewPrmaryControllerToolStripMenuItem
            // 
            this.createNewPrmaryControllerToolStripMenuItem.Name = "createNewPrmaryControllerToolStripMenuItem";
            this.createNewPrmaryControllerToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.createNewPrmaryControllerToolStripMenuItem.Text = "Create New Prmary Controller";
            this.createNewPrmaryControllerToolStripMenuItem.Click += new System.EventHandler(this.createNewPrmaryControllerToolStripMenuItem_Click);
            // 
            // addControllerToolStripMenuItem
            // 
            this.addControllerToolStripMenuItem.Name = "addControllerToolStripMenuItem";
            this.addControllerToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.addControllerToolStripMenuItem.Text = "Add Controller";
            this.addControllerToolStripMenuItem.Click += new System.EventHandler(this.addControllerToolStripMenuItem_Click);
            // 
            // addDeviceToolStripMenuItem
            // 
            this.addDeviceToolStripMenuItem.Name = "addDeviceToolStripMenuItem";
            this.addDeviceToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.addDeviceToolStripMenuItem.Text = "Add Device";
            this.addDeviceToolStripMenuItem.Click += new System.EventHandler(this.addDeviceToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(229, 6);
            // 
            // removeControllerToolStripMenuItem
            // 
            this.removeControllerToolStripMenuItem.Name = "removeControllerToolStripMenuItem";
            this.removeControllerToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.removeControllerToolStripMenuItem.Text = "Remove Controller";
            this.removeControllerToolStripMenuItem.Click += new System.EventHandler(this.removeControllerToolStripMenuItem_Click);
            // 
            // removeDeviceToolStripMenuItem
            // 
            this.removeDeviceToolStripMenuItem.Name = "removeDeviceToolStripMenuItem";
            this.removeDeviceToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.removeDeviceToolStripMenuItem.Text = "Remove Device";
            this.removeDeviceToolStripMenuItem.Click += new System.EventHandler(this.removeDeviceToolStripMenuItem_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(229, 6);
            // 
            // transferPrimaryRoleToolStripMenuItem
            // 
            this.transferPrimaryRoleToolStripMenuItem.Name = "transferPrimaryRoleToolStripMenuItem";
            this.transferPrimaryRoleToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.transferPrimaryRoleToolStripMenuItem.Text = "Transfer Primary Role";
            this.transferPrimaryRoleToolStripMenuItem.Click += new System.EventHandler(this.transferPrimaryRoleToolStripMenuItem_Click);
            // 
            // receiveConfigurationToolStripMenuItem
            // 
            this.receiveConfigurationToolStripMenuItem.Name = "receiveConfigurationToolStripMenuItem";
            this.receiveConfigurationToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.receiveConfigurationToolStripMenuItem.Text = "Receive Configuration";
            this.receiveConfigurationToolStripMenuItem.Click += new System.EventHandler(this.receiveConfigurationToolStripMenuItem_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(229, 6);
            // 
            // replaceFailedDeviceToolStripMenuItem
            // 
            this.replaceFailedDeviceToolStripMenuItem.Name = "replaceFailedDeviceToolStripMenuItem";
            this.replaceFailedDeviceToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.replaceFailedDeviceToolStripMenuItem.Text = "Replace Failed Device";
            this.replaceFailedDeviceToolStripMenuItem.Click += new System.EventHandler(this.replaceFailedDeviceToolStripMenuItem_Click);
            // 
            // cancelControllerCommandToolStripMenuItem
            // 
            this.cancelControllerCommandToolStripMenuItem.Name = "cancelControllerCommandToolStripMenuItem";
            this.cancelControllerCommandToolStripMenuItem.Size = new System.Drawing.Size(232, 22);
            this.cancelControllerCommandToolStripMenuItem.Text = "Cancel Controller Command";
            this.cancelControllerCommandToolStripMenuItem.Click += new System.EventHandler(this.cancelControllerCommandToolStripMenuItem_Click);
            // 
            // toolStripSeparator5
            // 
            this.toolStripSeparator5.Name = "toolStripSeparator5";
            this.toolStripSeparator5.Size = new System.Drawing.Size(229, 6);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(634, 392);
            this.Controls.Add(this.NodeGridView);
            this.Controls.Add(this.MenuBar);
            this.MainMenuStrip = this.MenuBar;
            this.Name = "MainForm";
            this.Text = "OpenZWave Test";
            ((System.ComponentModel.ISupportInitialize)(this.NodeGridView)).EndInit();
            this.NodeContextMenuStrip.ResumeLayout(false);
            this.MenuBar.ResumeLayout(false);
            this.MenuBar.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView NodeGridView;
        private System.Windows.Forms.MenuStrip MenuBar;
        private System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem SaveToolStripMenuItem;
        private System.Windows.Forms.ContextMenuStrip NodeContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem requestNodeNeighborUpdateToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem controllerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createNewPrmaryControllerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addControllerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem addDeviceToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem removeControllerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem removeDeviceToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem transferPrimaryRoleToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem receiveConfigurationToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem replaceFailedDeviceToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem PowerOnToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem PowerOffToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
        private System.Windows.Forms.ToolStripMenuItem cancelControllerCommandToolStripMenuItem;
    }
}

