using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelShort: ValuePanel
    {
        private System.Windows.Forms.NumericUpDown ValueShortNumericUpDown;
        private System.Windows.Forms.Label ValueShortLabel;
        private System.Windows.Forms.Button ValueShortButtonSet;

        public ValuePanelShort( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueShortLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueShortNumericUpDown.Enabled = false;
                ValueShortButtonSet.Visible = false;
            }

            Int16 value;
            if (MainForm.Manager.GetValueAsShort(valueID, out value))
            {
                ValueShortNumericUpDown.Value = Convert.ToDecimal(value);
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueShortNumericUpDown = new System.Windows.Forms.NumericUpDown();
            this.ValueShortButtonSet = new System.Windows.Forms.Button();
            this.ValueShortLabel = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.ValueShortNumericUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // ValueShortNumericUpDown
            // 
            this.ValueShortNumericUpDown.Location = new System.Drawing.Point(3, 6);
            this.ValueShortNumericUpDown.Maximum = new decimal(new int[] {
            32767,
            0,
            0,
            0});
            this.ValueShortNumericUpDown.Minimum = new decimal(new int[] {
            32768,
            0,
            0,
            -2147483648});
            this.ValueShortNumericUpDown.Name = "ValueShortNumericUpDown";
            this.ValueShortNumericUpDown.Size = new System.Drawing.Size(59, 20);
            this.ValueShortNumericUpDown.TabIndex = 0;
            // 
            // ValueShortButtonSet
            // 
            this.ValueShortButtonSet.Location = new System.Drawing.Point(68, 6);
            this.ValueShortButtonSet.Name = "ValueShortButtonSet";
            this.ValueShortButtonSet.Size = new System.Drawing.Size(59, 20);
            this.ValueShortButtonSet.TabIndex = 1;
            this.ValueShortButtonSet.Text = "Set";
            this.ValueShortButtonSet.UseVisualStyleBackColor = true;
            this.ValueShortButtonSet.Click += new System.EventHandler(this.ValueShortButtonSet_Click);
            // 
            // ValueShortLabel
            // 
            this.ValueShortLabel.AutoSize = true;
            this.ValueShortLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueShortLabel.Name = "ValueShortLabel";
            this.ValueShortLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueShortLabel.TabIndex = 2;
            this.ValueShortLabel.Text = "Label";
            // 
            // ValuePanelShort
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueShortLabel);
            this.Controls.Add(this.ValueShortButtonSet);
            this.Controls.Add(this.ValueShortNumericUpDown);
            this.Name = "ValuePanelShort";
            this.Size = new System.Drawing.Size(169, 29);
            ((System.ComponentModel.ISupportInitialize)(this.ValueShortNumericUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void ValueShortButtonSet_Click(object sender, EventArgs e)
        {
            if (SendChanges)
            {
                Int16 value = Convert.ToInt16(ValueShortNumericUpDown.Value);
                MainForm.Manager.SetValue(ValueID, value);
            }
        }
    }
}
