using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelByte: ValuePanel
    {
        private System.Windows.Forms.NumericUpDown ValueByteNumericUpDown;
        private System.Windows.Forms.Label ValueByteLabel;
        private System.Windows.Forms.Button ValueByteButtonSet;

        public ValuePanelByte( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueByteLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueByteNumericUpDown.Enabled = false;
                ValueByteButtonSet.Visible = false;
            }

            Byte value;
            if (MainForm.Manager.GetValueAsByte(valueID, out value))
            {
                ValueByteNumericUpDown.Value = Convert.ToDecimal(value);
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueByteNumericUpDown = new System.Windows.Forms.NumericUpDown();
            this.ValueByteButtonSet = new System.Windows.Forms.Button();
            this.ValueByteLabel = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.ValueByteNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // ValueByteNumericUpDown
            // 
            this.ValueByteNumericUpDown.Location = new System.Drawing.Point(3, 6);
            this.ValueByteNumericUpDown.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.ValueByteNumericUpDown.Name = "ValueByteNumericUpDown";
            this.ValueByteNumericUpDown.Size = new System.Drawing.Size(59, 20);
            this.ValueByteNumericUpDown.TabIndex = 0;
            // 
            // ValueByteButtonSet
            // 
            this.ValueByteButtonSet.Location = new System.Drawing.Point(68, 6);
            this.ValueByteButtonSet.Name = "ValueByteButtonSet";
            this.ValueByteButtonSet.Size = new System.Drawing.Size(59, 20);
            this.ValueByteButtonSet.TabIndex = 1;
            this.ValueByteButtonSet.Text = "Set";
            this.ValueByteButtonSet.UseVisualStyleBackColor = true;
            this.ValueByteButtonSet.Click += new System.EventHandler(this.ValueByteButtonSet_Click);
            // 
            // ValueByteLabel
            // 
            this.ValueByteLabel.AutoSize = true;
            this.ValueByteLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueByteLabel.Name = "ValueByteLabel";
            this.ValueByteLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueByteLabel.TabIndex = 2;
            this.ValueByteLabel.Text = "Label";
            // 
            // ValuePanelByte
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueByteLabel);
            this.Controls.Add(this.ValueByteButtonSet);
            this.Controls.Add(this.ValueByteNumericUpDown);
            this.Name = "ValuePanelByte";
            this.Size = new System.Drawing.Size(169, 29);
            ((System.ComponentModel.ISupportInitialize)(this.ValueByteNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void ValueByteButtonSet_Click(object sender, EventArgs e)
        {
            if (SendChanges)
            {
                Byte value = Convert.ToByte(ValueByteNumericUpDown.Value);
                MainForm.Manager.SetValue(ValueID, value);
            }
        }
    }
}
