## Network Related Errors

These messages are related to managing the Z-Wave network as a whole. They are usually routing or healing related messages. Occasional errors in this category should not be do much of a concern, but if you get lots of them, you should investigate the issue more closely.

### Specific MessageID Results

#### MessageID 49

A Request for the network to update its topology (rediscover neighbors and so on) failed. This is usually the result of a busy network or a SUC/SIS device offline.

#### MessageID 50, 51, 57, 69

A Request for a node to update its routing table failed. This could be because the node is offline or not reachable. You should confirm connectivity to the node and try again.

#### MessageID 54, 54

Requests to the Controller to check if a Node has failed or not. These are usually application initiated messages, and unless you are experiencing issues, you can safely ignore them. You should also consult the [DeadNode](deadnode) page for further info.

#### MessageID 59

A request to a Node to update its routing table was aborted. This is usually because of some type of event or message sent to the node while it was in progress. Unless you are having communication issues, you can safely ignore this message. If you are having communication issues, please retry the command. 

#### MessageID 60, 61

A request to update the Network Topology failed as the SUC node is either busy or offline. Please ensure the SUC node is online and reachable and retry the command.

#### MessageID 62

A request to update the Network Topology failed as the SUC and Controller Nodes are not fully synchronized. You should initiate a Controller Replication command and try again.

