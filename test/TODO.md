# TODO - Test Cases

## Session Save/Open
 * Verify each save session triggers the file dialog at the last path used
 * OSSN Session file format tests (TODO: expand)
 * Verify no file is saved if user clicks 'Cancel' on the progress dialog while saving session file

 * On open session, verify user is prompted if there are existing portgroups and not if there are no port groups
 * Verify each open session triggers the file dialog at the last path used
 * OSSN Session file format tests (TODO: expand)
 * Verify no change in existing port groups if user clicks 'Cancel' on the options dialog
 * Verify no change in existing port groups if user clicks 'Cancel' on the progress dialog while opening session file
 * Verify all old portgroups are removed before new portgroups from the session file are added
 * Verify config of portGroups loaded from the session file are correct
 * Verify config of a portGroup is NOT restored to the config saved in session file after a open session - disconnect - connect
