/*
 * Created by SharpDevelop.
 * User: Doug Brown
 * Date: 4/15/2010
 * Time: 7:39 AM
 * 
 * Copyright 2010 Syanpsity-AZ
 */
namespace OzwOnOffDemo
{
	partial class MainForm
	{
		/// <summary>
		/// Designer variable used to keep track of non-visual components.
		/// </summary>
		private System.ComponentModel.IContainer components = null;
		
		/// <summary>
		/// Disposes resources used by the form.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing) {
				if (components != null) {
					components.Dispose();
				}
			}
			base.Dispose(disposing);
		}
		
		/// <summary>
		/// This method is required for Windows Forms designer support.
		/// Do not change the method contents inside the source code editor. The Forms designer might
		/// not be able to load this method if it was changed manually.
		/// </summary>
		private void InitializeComponent()
		{
			this.nodeTextBox = new System.Windows.Forms.TextBox();
			this.statusButton = new System.Windows.Forms.Button();
			this.NodeCntLabel = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.tbHomeID = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.ExitButton = new System.Windows.Forms.Button();
			this.OnButton = new System.Windows.Forms.Button();
			this.OffButton = new System.Windows.Forms.Button();
			this.BeginRepButton = new System.Windows.Forms.Button();
			this.EndRepButton = new System.Windows.Forms.Button();
			this.ResetCtrlButton = new System.Windows.Forms.Button();
			this.SoftResetButton = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// nodeTextBox
			// 
			this.nodeTextBox.Location = new System.Drawing.Point(58, 54);
			this.nodeTextBox.Name = "nodeTextBox";
			this.nodeTextBox.Size = new System.Drawing.Size(39, 20);
			this.nodeTextBox.TabIndex = 3;
			// 
			// statusButton
			// 
			this.statusButton.Location = new System.Drawing.Point(61, 80);
			this.statusButton.Name = "statusButton";
			this.statusButton.Size = new System.Drawing.Size(75, 23);
			this.statusButton.TabIndex = 4;
			this.statusButton.Text = "Status";
			this.statusButton.UseVisualStyleBackColor = true;
			this.statusButton.Click += new System.EventHandler(this.StatusButtonClick);
			// 
			// NodeCntLabel
			// 
			this.NodeCntLabel.Location = new System.Drawing.Point(3, 3);
			this.NodeCntLabel.Name = "NodeCntLabel";
			this.NodeCntLabel.Size = new System.Drawing.Size(215, 19);
			this.NodeCntLabel.TabIndex = 6;
			this.NodeCntLabel.Text = "Finding Switch Nodes";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(3, 25);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(49, 23);
			this.label1.TabIndex = 7;
			this.label1.Text = "HomeID:";
			this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// tbHomeID
			// 
			this.tbHomeID.Location = new System.Drawing.Point(58, 25);
			this.tbHomeID.Name = "tbHomeID";
			this.tbHomeID.ReadOnly = true;
			this.tbHomeID.Size = new System.Drawing.Size(78, 20);
			this.tbHomeID.TabIndex = 8;
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(12, 51);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(37, 23);
			this.label2.TabIndex = 9;
			this.label2.Text = "Node:";
			this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// ExitButton
			// 
			this.ExitButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ExitButton.Location = new System.Drawing.Point(152, 189);
			this.ExitButton.Name = "ExitButton";
			this.ExitButton.Size = new System.Drawing.Size(75, 23);
			this.ExitButton.TabIndex = 10;
			this.ExitButton.Text = "Exit";
			this.ExitButton.UseVisualStyleBackColor = true;
			this.ExitButton.Click += new System.EventHandler(this.ExitButtonClick);
			// 
			// OnButton
			// 
			this.OnButton.Location = new System.Drawing.Point(103, 54);
			this.OnButton.Name = "OnButton";
			this.OnButton.Size = new System.Drawing.Size(33, 20);
			this.OnButton.TabIndex = 11;
			this.OnButton.Text = "On";
			this.OnButton.UseVisualStyleBackColor = true;
			this.OnButton.Click += new System.EventHandler(this.OnButtonClick);
			// 
			// OffButton
			// 
			this.OffButton.Enabled = false;
			this.OffButton.Location = new System.Drawing.Point(142, 54);
			this.OffButton.Name = "OffButton";
			this.OffButton.Size = new System.Drawing.Size(33, 20);
			this.OffButton.TabIndex = 12;
			this.OffButton.Text = "Off";
			this.OffButton.UseVisualStyleBackColor = true;
			this.OffButton.Click += new System.EventHandler(this.OffButtonClick);
			// 
			// BeginRepButton
			// 
			this.BeginRepButton.Location = new System.Drawing.Point(22, 122);
			this.BeginRepButton.Name = "BeginRepButton";
			this.BeginRepButton.Size = new System.Drawing.Size(75, 23);
			this.BeginRepButton.TabIndex = 13;
			this.BeginRepButton.Text = "Begin Rep";
			this.BeginRepButton.UseVisualStyleBackColor = true;
			this.BeginRepButton.Click += new System.EventHandler(this.BeginRepButtonClick);
			// 
			// EndRepButton
			// 
			this.EndRepButton.Location = new System.Drawing.Point(127, 122);
			this.EndRepButton.Name = "EndRepButton";
			this.EndRepButton.Size = new System.Drawing.Size(75, 23);
			this.EndRepButton.TabIndex = 14;
			this.EndRepButton.Text = "End Rep";
			this.EndRepButton.UseVisualStyleBackColor = true;
			this.EndRepButton.Click += new System.EventHandler(this.EndRepButtonClick);
			// 
			// ResetCtrlButton
			// 
			this.ResetCtrlButton.Location = new System.Drawing.Point(22, 151);
			this.ResetCtrlButton.Name = "ResetCtrlButton";
			this.ResetCtrlButton.Size = new System.Drawing.Size(75, 23);
			this.ResetCtrlButton.TabIndex = 15;
			this.ResetCtrlButton.Text = "Reset Ctrl";
			this.ResetCtrlButton.UseVisualStyleBackColor = true;
			this.ResetCtrlButton.Click += new System.EventHandler(this.ResetCtrlButtonClick);
			// 
			// SoftResetButton
			// 
			this.SoftResetButton.Location = new System.Drawing.Point(127, 151);
			this.SoftResetButton.Name = "SoftResetButton";
			this.SoftResetButton.Size = new System.Drawing.Size(75, 23);
			this.SoftResetButton.TabIndex = 16;
			this.SoftResetButton.Text = "Soft Reset";
			this.SoftResetButton.UseVisualStyleBackColor = true;
			this.SoftResetButton.Click += new System.EventHandler(this.SoftResetButtonClick);
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(230, 214);
			this.ControlBox = false;
			this.Controls.Add(this.SoftResetButton);
			this.Controls.Add(this.ResetCtrlButton);
			this.Controls.Add(this.EndRepButton);
			this.Controls.Add(this.BeginRepButton);
			this.Controls.Add(this.OffButton);
			this.Controls.Add(this.OnButton);
			this.Controls.Add(this.ExitButton);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.tbHomeID);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.NodeCntLabel);
			this.Controls.Add(this.statusButton);
			this.Controls.Add(this.nodeTextBox);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "MainForm";
			this.Text = "Open ZWave On/Off Demo";
			this.ResumeLayout(false);
			this.PerformLayout();
		}
		private System.Windows.Forms.Button SoftResetButton;
		private System.Windows.Forms.Button ResetCtrlButton;
		private System.Windows.Forms.Button EndRepButton;
		private System.Windows.Forms.Button BeginRepButton;
		private System.Windows.Forms.Button OffButton;
		private System.Windows.Forms.Button OnButton;
		private System.Windows.Forms.Button ExitButton;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.TextBox tbHomeID;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label NodeCntLabel;
		private System.Windows.Forms.Button statusButton;
		private System.Windows.Forms.TextBox nodeTextBox;
	}
}
