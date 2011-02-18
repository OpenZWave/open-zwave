using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelString: ValuePanel
    {
        private System.Windows.Forms.TextBox ValueStringTextBox;
        private System.Windows.Forms.Label ValueStringLabel;

        public ValuePanelString( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueStringLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueStringTextBox.Enabled = false;
            }

            String value;
            if (MainForm.Manager.GetValueAsString(valueID, out value))
            {
                ValueStringTextBox.Text = value;
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueStringLabel = new System.Windows.Forms.Label();
            this.ValueStringTextBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // ValueStringLabel
            // 
            this.ValueStringLabel.AutoSize = true;
            this.ValueStringLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueStringLabel.Name = "ValueStringLabel";
            this.ValueStringLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueStringLabel.TabIndex = 2;
            this.ValueStringLabel.Text = "Label";
            // 
            // ValueStringTextBox
            // 
            this.ValueStringTextBox.Location = new System.Drawing.Point(4, 4);
            this.ValueStringTextBox.MaxLength = 255;
            this.ValueStringTextBox.Name = "ValueStringTextBox";
            this.ValueStringTextBox.Size = new System.Drawing.Size(123, 20);
            this.ValueStringTextBox.TabIndex = 3;
            // 
            // ValuePanelString
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueStringTextBox);
            this.Controls.Add(this.ValueStringLabel);
            this.Name = "ValuePanelString";
            this.Size = new System.Drawing.Size(169, 27);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
    }
}
