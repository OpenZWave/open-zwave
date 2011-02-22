using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelInt: ValuePanel
    {
        private System.Windows.Forms.NumericUpDown ValueIntNumericUpDown;
        private System.Windows.Forms.Label ValueIntLabel;
        private System.Windows.Forms.Button ValueIntButtonSet;

        public ValuePanelInt( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueIntLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueIntNumericUpDown.Enabled = false;
                ValueIntButtonSet.Visible = false;
            }

            Int32 value;
            if (MainForm.Manager.GetValueAsInt(valueID, out value))
            {
                ValueIntNumericUpDown.Value = Convert.ToDecimal(value);
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueIntNumericUpDown = new System.Windows.Forms.NumericUpDown();
            this.ValueIntButtonSet = new System.Windows.Forms.Button();
            this.ValueIntLabel = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.ValueIntNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // ValueIntNumericUpDown
            // 
            this.ValueIntNumericUpDown.Location = new System.Drawing.Point(3, 6);
            this.ValueIntNumericUpDown.Maximum = new decimal(new int[] {
            2147483647,
            0,
            0,
            0});
            this.ValueIntNumericUpDown.Minimum = new decimal(new int[] {
            -2147483648,
            0,
            0,
            -2147483648});
            this.ValueIntNumericUpDown.Name = "ValueIntNumericUpDown";
            this.ValueIntNumericUpDown.Size = new System.Drawing.Size(59, 20);
            this.ValueIntNumericUpDown.TabIndex = 0;
            this.ValueIntNumericUpDown.ThousandsSeparator = true;
            // 
            // ValueIntButtonSet
            // 
            this.ValueIntButtonSet.Location = new System.Drawing.Point(68, 6);
            this.ValueIntButtonSet.Name = "ValueIntButtonSet";
            this.ValueIntButtonSet.Size = new System.Drawing.Size(59, 20);
            this.ValueIntButtonSet.TabIndex = 1;
            this.ValueIntButtonSet.Text = "Set";
            this.ValueIntButtonSet.UseVisualStyleBackColor = true;
            this.ValueIntButtonSet.Click += new System.EventHandler(this.ValueIntButtonSet_Click);
            // 
            // ValueIntLabel
            // 
            this.ValueIntLabel.AutoSize = true;
            this.ValueIntLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueIntLabel.Name = "ValueIntLabel";
            this.ValueIntLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueIntLabel.TabIndex = 2;
            this.ValueIntLabel.Text = "Label";
            // 
            // ValuePanelInt
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueIntLabel);
            this.Controls.Add(this.ValueIntButtonSet);
            this.Controls.Add(this.ValueIntNumericUpDown);
            this.Name = "ValuePanelInt";
            this.Size = new System.Drawing.Size(169, 29);
            ((System.ComponentModel.ISupportInitialize)(this.ValueIntNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void ValueIntButtonSet_Click(object sender, EventArgs e)
        {
            if (SendChanges)
            {
                Int32 value = Convert.ToInt32(ValueIntNumericUpDown.Value);
                MainForm.Manager.SetValue(ValueID, value);
            }
        }
    }
}
