# TODO - Test Cases

## Session Save/Open
 * Verify save session prompts before opening Save Dialog about only reserved ports being saved if there are some reserved ports and not prompted at all if no ports are reserved
 * Verify each save session triggers the file dialog at the last path used
 * Verify saved session file is correct
   * All portgroups are saved
   * All suitable (wrt reservation) ports are saved
   * All port configuration is saved
   * For each port -
     * All streams are saved with correct contents
     * All deviceGroups are saved with correct contents
 * OSSN Session file format tests (TODO: expand)
 * Verify no file is saved if user clicks 'Cancel' on the progress dialog while saving session file

 * On open session, verify user is prompted before opening the file dialog if there are existing portgroups and not prompted at all if there are no port groups
 * Verify each open session triggers the file dialog at the last path used
 * Verify open file dialog file filter has `(*.ssn *.*)`
 * Verify opening a unsupported format file triggers an error and existing session is not changed
 * Verify all existing portgroups are removed before new ones from the file are created and configured
 * Verify that only ports in the opened session file are overwritten and other ports are not changed
 * Verify that if port in the opened session file was reserved at save time, and the same port is now reserved by someone else, it is not changed and user is informed; if current port reservation is by self, port is overwritten with contents from the session file; all reservations made by open session are with self username, not the username who had reserved the port during save time (in other words, allow session files to be exchanged between users)
 * Verify no unnecessary RPCs during open session
   * if port has no existing streams, deleteStreams() is not invoked
   * if port has no existing deviceGroups, deleteDeviceGroups() is not invoked
   * if port config has no change, modifyPort() is not invoked
   * if opened port has no streams, addStreams()/modifyStreams() is not invoked
   * if opened port has no deviceGroups, addDeviceGroups()/modifyDeviceGroups() is not invoked
 * Verify open session is successful
   * All streams are restored with correct contents
   * All deviceGroups are restored with correct contents
   * Port config (TxMode, ExclusiveMode, UserName) is changed, if required
 * OSSN Session file format tests (TODO: expand)
 * Verify no change in existing port groups if user clicks 'Cancel' on the options dialog
 * Verify no change in existing port groups if user clicks 'Cancel' on the progress dialog while opening session file
 * Verify all old portgroups are removed before new portgroups from the session file are added
 * Verify config of portGroups loaded from the session file are correct
 * Verify config of a portGroup is NOT restored to the config saved in session file after a open session - disconnect - connect

## Streams Save/Open
 * Verify save file dialog file filter has all supported types but no `*.*`
 * Stream file format tests (TODO: expand)

 * Verify open file dialog file filter has all supported types except `*.py` and `*.*`
 * Verify opening a unsupported format file triggers an error and existing streams are not changed
 * Stream file format tests (TODO: expand)
