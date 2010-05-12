Imports OpenZWaveDotNET



Public Class Form1

    Dim zwManager As New ZWManager
    Dim zwValuesData As New DataSet1.ValuesDataTable
    Dim homeIds As New List(Of UInt32)
    Dim critical As New Object



    Private Sub zwavenotification(ByVal notification As ZWNotification)
        Try
            Threading.Monitor.TryEnter(critical)
            Try
                Select Case notification.GetType()


                    Case Is = ZWNotification.Type_DriverReady
                        homeids.Add(notification.GetValueID.GetHomeId())
                        logText("Driver Ready " & notification.GetValueID.GetHomeId())
                    Case Is = ZWNotification.Type_NodeAdded
                        logText("Node Added " & notification.GetValueID.GetNodeId())
                    Case Is = ZWNotification.Type_NodeRemoved
                        logText("Node Removed " & notification.GetValueID.GetNodeId())
                    Case Is = ZWNotification.Type_NodeStatus
                        logText("Node Status " & notification.GetValueID.GetNodeId() & " status is " & notification.GetStatus())
                    Case Is = ZWNotification.Type_ValueAdded

                        Dim vid As ZWValueID = notification.GetValueID
                        Dim value As ZWValue = zwManager.GetValue(vid)
                        dovalueadded(vid.GetHomeId, vid.GetUniqueId, vid.GetNodeId, vid.GetGenre, vid.GetType, value.GetLabel, value.GetAsString, value.IsReadOnly)

                        logText("Value Added " & notification.GetValueID.GetNodeId() & " Value Label is " & zwManager.GetValue(notification.GetValueID()).GetLabel())

                    Case Is = ZWNotification.Type_ValueRemoved

                        Dim vid As ZWValueID = notification.GetValueID
                        dovalueremove(vid.GetHomeId, vid.GetUniqueId)
                        logText("Value Removed " & notification.GetValueID.GetNodeId() & " ValueID Label is " & zwManager.GetValue(notification.GetValueID()).GetLabel())

                    Case Is = ZWNotification.Type_ValueChanged

                        Dim vid As ZWValueID = notification.GetValueID
                        Dim value As ZWValue = zwManager.GetValue(vid)
                        dovaluechange(vid.GetHomeId, vid.GetUniqueId, vid.GetNodeId, vid.GetGenre, vid.GetType, value.GetLabel, value.GetAsString, value.IsReadOnly)

                        logText("Value Changed " & notification.GetValueID.GetNodeId() & " ValueID Label is " & zwManager.GetValue(notification.GetValueID()).GetLabel())
                End Select
            Catch ex As Exception
            Finally
                Threading.Monitor.Exit(critical)
            End Try
        Catch ex As Exception

        End Try
        
        
        ' dotext("Noty:" & notification.GetType() & " - " & notification.)

    End Sub

    Private Delegate Sub logTextDel(ByVal t As String)

    Private Sub logText(ByVal t As String)
        If TextBox1.InvokeRequired Then
            Invoke(New logTextDel(AddressOf logText), New Object() {t})
        Else
            TextBox1.Text = t & " " & Now & vbCrLf & TextBox1.Text
        End If
    End Sub

    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        DataGridView1.AutoGenerateColumns = False
        DataGridView1.DataSource = zwValuesData

    End Sub

    Private Delegate Sub dovalueaddeddel(ByVal homeid As UInt32, ByVal valueid As UInt32, ByVal nodeid As Byte, ByVal genre As Byte, ByVal type As Byte, ByVal label As String, ByVal value As String, ByVal reado As String)

    Private Sub dovalueadded(ByVal homeid As UInt32, ByVal valueid As UInt32, ByVal nodeid As Byte, ByVal genre As Byte, ByVal type As Byte, ByVal label As String, ByVal value As String, ByVal reado As String)
        If DataGridView1.InvokeRequired Then
            Invoke(New dovalueaddeddel(AddressOf dovalueadded), New Object() {homeid, valueid, nodeid, genre, type, label, value, reado})
        Else
            Dim r As DataSet1.ValuesRow
            r = zwValuesData.NewRow
            r.valueId = valueid
            r.homeId = homeid
            r.datatype = type
            r.genre = genre
            r.label = label
            r.nodeid = nodeid
            r.isreadonly = reado
            r.value = value
            zwValuesData.Rows.Add(r)
        End If


    End Sub

    Private Delegate Sub dovaluechangeddel(ByVal homeid As UInt32, ByVal valueid As UInt32, ByVal nodeid As Byte, ByVal genre As Byte, ByVal type As Byte, ByVal label As String, ByVal value As String, ByVal reado As String)

    Private Sub dovaluechange(ByVal homeid As UInt32, ByVal valueid As UInt32, ByVal nodeid As Byte, ByVal genre As Byte, ByVal type As Byte, ByVal label As String, ByVal value As String, ByVal reado As String)
        If DataGridView1.InvokeRequired Then
            Invoke(New dovaluechangeddel(AddressOf dovaluechange), New Object() {homeid, valueid, nodeid, genre, type, label, value, reado})
        Else
            Dim r As DataSet1.ValuesRow = zwValuesData.FindByhomeIdvalueId(homeid, valueid)
            r.valueId = valueid
            r.homeId = homeid
            r.datatype = type
            r.genre = genre
            r.label = label
            r.nodeid = nodeid
            r.isreadonly = reado
            r.value = value
        End If


    End Sub

    Private Delegate Sub dovalueremovedel(ByVal homeid As UInt32, ByVal valueid As UInt32)

    Private Sub dovalueremove(ByVal homeid As UInt32, ByVal valueid As UInt32)
        If DataGridView1.InvokeRequired Then
            Invoke(New dovalueremovedel(AddressOf dovalueremove), New Object() {homeid, valueid})
        Else
            zwValuesData.FindByhomeIdvalueId(homeid, valueid).Delete()
        End If


    End Sub


    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        zwManager.Create("", "")
        AddHandler zwManager.OnZWNotification, AddressOf zwavenotification

    End Sub

    Private Sub Button2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button2.Click
        RemoveHandler zwManager.OnZWNotification, AddressOf zwavenotification
        zwManager.Destroy()

    End Sub

    Private Sub Button3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button3.Click
        zwManager.AddDriver("\\.\COM4")
    End Sub

    Private Sub Button4_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button4.Click
        For Each homeid As UInt32 In homeids
            zwManager.WriteConfig(homeid)
        Next
    End Sub

   
   

    Private Sub DataGridView1_CellEndEdit(ByVal sender As Object, ByVal e As System.Windows.Forms.DataGridViewCellEventArgs) Handles DataGridView1.CellEndEdit
        'i dont know if this might throw an error if a row is added or deleted during the procedure
        Try
            Dim valueid As UInt32 = DataGridView1.Rows(e.RowIndex).Cells(0).Value
            Dim homeid As UInt32 = DataGridView1.Rows(e.RowIndex).Cells(1).Value
            zwManager.SetValue(New ZWValueID(homeid, valueid), DataGridView1.Rows(e.RowIndex).Cells(4).Value)
        Catch ex As Exception

        End Try

    End Sub

   
End Class
