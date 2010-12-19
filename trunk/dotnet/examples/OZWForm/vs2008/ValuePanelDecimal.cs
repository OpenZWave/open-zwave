using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelDecimal: ValuePanel
    {
        private System.Windows.Forms.TextBox ValueDecimalTextBox;
        private System.Windows.Forms.Label ValueDecimalLabel;

        public ValuePanelDecimal( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueDecimalLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueDecimalTextBox.Enabled = false;
            }

            Decimal value;
            if (MainForm.Manager.GetValueAsDecimal(valueID, out value))
            {
                ValueDecimalTextBox.Text = value.ToString();
            }
        }

        private void InitializeComponent()
        {
            this.ValueDecimalLabel = new System.Windows.Forms.Label();
            this.ValueDecimalTextBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // ValueDecimalLabel
            // 
            this.ValueDecimalLabel.AutoSize = true;
            this.ValueDecimalLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueDecimalLabel.Name = "ValueDecimalLabel";
            this.ValueDecimalLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueDecimalLabel.TabIndex = 2;
            this.ValueDecimalLabel.Text = "Label";
            // 
            // ValueDecimalTextBox
            // 
            this.ValueDecimalTextBox.Location = new System.Drawing.Point(4, 4);
            this.ValueDecimalTextBox.MaxLength = 255;
            this.ValueDecimalTextBox.Name = "ValueDecimalTextBox";
            this.ValueDecimalTextBox.Size = new System.Drawing.Size(123, 20);
            this.ValueDecimalTextBox.TabIndex = 3;
            // 
            // ValuePanelDecimal
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueDecimalTextBox);
            this.Controls.Add(this.ValueDecimalLabel);
            this.Name = "ValuePanelDecimal";
            this.Size = new System.Drawing.Size(169, 27);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
    }
}
