## Adding and Removing Nodes

Issues in this category are related to using OpenZWave to include and exclude devices from your network.

### Specific MessageID Results

#### MessageID 53, 56, 67, 68

A attempt to remove a node via the RemoveFailedNode command failed. This is usually caused by the Controller not considering the node as failed. You should instead try to remove the node using the standard removenode commands.

#### MessageID 63

When attempting to remove a Node, the Controller returned Node 0 which is a invalid Node. Please restart the application and try again, and if the problem persists, seek help on the [Google Groups list][1]

[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"
