﻿using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelButton: ValuePanel
    {
        private System.Windows.Forms.Button ValueButtonButton;

        public ValuePanelButton( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueButtonButton.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueButtonButton.Enabled = false;
            }
        }

        private void InitializeComponent()
        {
            this.ValueButtonButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // ValueButtonButton
            // 
            this.ValueButtonButton.AutoSize = true;
            this.ValueButtonButton.Location = new System.Drawing.Point(3, 6);
            this.ValueButtonButton.Name = "ValueButtonButton";
            this.ValueButtonButton.Size = new System.Drawing.Size(59, 23);
            this.ValueButtonButton.TabIndex = 1;
            this.ValueButtonButton.Text = "Label";
            this.ValueButtonButton.UseVisualStyleBackColor = true;
            // 
            // ValuePanelButton
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueButtonButton);
            this.Name = "ValuePanelButton";
            this.Size = new System.Drawing.Size(65, 32);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
    }
}
