## Optimizing Scan Results

In order to get the most complete coverage possible, you should ensure that follow the instructions below when creating a LogFile for scanning. 

 * Do not Filter/Grep or Edit the Log File
 * Ensure that you remove the zwcfg_*.xml file before running your application. 
 * Only stop your application after you have received a AllNodesQueried Notification. (if you stop before then, you will have incomplete results)
 * If you are trying to fix a issue, try to reproduce the issue as soon as you receive the AllNodesQueried notification.
 * If you are going to log a issue or bug, ensure that you mark your scan results as public, and include the link to the results in the bug report. 

### Specific MessageID Results

#### MessageID 10000

The logfile was created when the zwcfg_*.xml file was present. A lot of information will not be present, and we might not be able to fully diagnose your issue with the zwcfg_*.xml file present as a lot of the QueryStages will be skipped. 

