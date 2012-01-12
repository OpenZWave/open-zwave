using System;
using System.Collections.Generic;
using System.Text;
using OpenZWaveDotNet;

namespace OZWForm
{
    public class ValuePanelList: ValuePanel
    {
        private System.Windows.Forms.ComboBox ValueListComboBox;
        private System.Windows.Forms.Label ValueListLabel;

        public ValuePanelList( ZWValueID valueID ): base( valueID )
        {
            InitializeComponent();

            ValueListLabel.Text = MainForm.Manager.GetValueLabel(valueID);

            if (MainForm.Manager.IsValueReadOnly(valueID))
            {
                ValueListComboBox.Enabled = false;
            }

            String[] items;
            if (MainForm.Manager.GetValueListItems(valueID, out items))
            {
                ValueListComboBox.Items.Clear();
                foreach (String item in items)
                {
                    ValueListComboBox.Items.Add(item);
                }
            }

            String value;
            if (MainForm.Manager.GetValueListSelection(valueID, out value))
            {
                ValueListComboBox.Text = value;
            }

            SendChanges = true;
        }

        private void InitializeComponent()
        {
            this.ValueListLabel = new System.Windows.Forms.Label();
            this.ValueListComboBox = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // ValueListLabel
            // 
            this.ValueListLabel.AutoSize = true;
            this.ValueListLabel.Location = new System.Drawing.Point(133, 10);
            this.ValueListLabel.Name = "ValueListLabel";
            this.ValueListLabel.Size = new System.Drawing.Size(33, 13);
            this.ValueListLabel.TabIndex = 2;
            this.ValueListLabel.Text = "Label";
            // 
            // ValueListComboBox
            // 
            this.ValueListComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ValueListComboBox.FormattingEnabled = true;
            this.ValueListComboBox.Location = new System.Drawing.Point(4, 4);
            this.ValueListComboBox.MaxDropDownItems = 100;
            this.ValueListComboBox.Name = "ValueListComboBox";
            this.ValueListComboBox.Size = new System.Drawing.Size(121, 21);
            this.ValueListComboBox.TabIndex = 3;
            this.ValueListComboBox.SelectedIndexChanged += new System.EventHandler(this.ValueListComboBox_SelectedIndexChanged);
            // 
            // ValuePanelList
            // 
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.ValueListComboBox);
            this.Controls.Add(this.ValueListLabel);
            this.Name = "ValuePanelList";
            this.Size = new System.Drawing.Size(169, 28);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void ValueListComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (SendChanges)
            {
                MainForm.Manager.SetValueListSelection(ValueID, ValueListComboBox.Text);
            }
        }
    }
}
