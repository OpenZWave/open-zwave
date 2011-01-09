using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelDecimal: ValuePanel
    {
        private System.Windows.Forms.TextBox ValueDecimalTextBox;
        private System.Windows.Forms.Button ValueDecimalButtonSet;
        private System.Windows.Forms.Label ValueDecimalLabel;

        public ValuePanelDecimal( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueDecimalLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueDecimalTextBox.Enabled = false;
				ValueDecimalButtonSet.Enabled = false;
            }

            Decimal value;
            if (MainForm.Manager.GetValueAsDecimal(valueID, out value))
            {
                ValueDecimalTextBox.Text = value.ToString();
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueDecimalLabel = new System.Windows.Forms.Label();
            this.ValueDecimalTextBox = new System.Windows.Forms.TextBox();
            this.ValueDecimalButtonSet = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // ValueDecimalLabel
            // 
            this.ValueDecimalLabel.AutoSize = true;
            this.ValueDecimalLabel.Location = new System.Drawing.Point(152, 7);
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
            this.ValueDecimalTextBox.Size = new System.Drawing.Size(77, 20);
            this.ValueDecimalTextBox.TabIndex = 3;
            // 
            // ValueDecimalButtonSet
            // 
            this.ValueDecimalButtonSet.Location = new System.Drawing.Point(87, 3);
            this.ValueDecimalButtonSet.Name = "ValueDecimalButtonSet";
            this.ValueDecimalButtonSet.Size = new System.Drawing.Size(59, 20);
            this.ValueDecimalButtonSet.TabIndex = 4;
            this.ValueDecimalButtonSet.Text = "Set";
            this.ValueDecimalButtonSet.UseVisualStyleBackColor = true;
            this.ValueDecimalButtonSet.Click += new System.EventHandler(this.ValueDecimalButtonSet_Click);
            // 
            // ValuePanelDecimal
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueDecimalButtonSet);
            this.Controls.Add(this.ValueDecimalTextBox);
            this.Controls.Add(this.ValueDecimalLabel);
            this.Name = "ValuePanelDecimal";
            this.Size = new System.Drawing.Size(188, 27);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void ValueDecimalButtonSet_Click(object sender, EventArgs e)
        {
            if (SendChanges)
            {
                float value = Convert.ToSingle(ValueDecimalTextBox.Text);
                MainForm.Manager.SetValue(ValueID, value);
            }
        }
    }
}
