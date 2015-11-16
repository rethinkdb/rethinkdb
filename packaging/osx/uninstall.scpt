-- check that a package version has been installed
set packageIdentifier to ""
set installedPath to ""
set installedVersion to ""
repeat with candidateIdentifier in {"rethinkdb", "com.rethinkdb", "com.rethinkdb.server"}
	set installedPath to do shell script "/usr/sbin/pkgutil --pkg-info " & candidateIdentifier & " | awk '/^location: / {sub(/^location: \\/?/, \"/\"); print $0}'"
	set installedVersion to do shell script "/usr/sbin/pkgutil --pkg-info " & candidateIdentifier & " | awk '/^version: / {sub(/^version: /, \"\"); print $0}'"
	if installedPath is not "" then
		set packageIdentifier to candidateIdentifier
		exit repeat
	end if
end repeat

if installedPath does not end with "/" then
	set installedPath to installedPath & "/"
end if

on escapePath(target)
	set AppleScript's text item delimiters to "/"
	set the pathList to every text item of target
	set AppleScript's text item delimiters to "\\/"
	set target to the pathList as string
	set AppleScript's text item delimiters to ""
	return target
end escapePath

if packageIdentifier is "" then
	-- the package is not installed, check if the server is there
	try
		do shell script "PATH=:\"$PATH:/usr/local/bin\"; /usr/bin/which rethinkdb"
		display alert "Unable to uninstall" message "Your copy of RethinkDB was not installed using a Mac OS package: you may have installed RethinkDB using Homebrew or manually." & return & return & "This uninstaller will not be able to remove your installation."
	on error
		display alert "Unable to uninstall" message "The RethinkDB package is not installed on this system."
	end try
else
	-- verify with the user
	set displayPath to installedPath
	if installedPath is "/" then
		set displayPath to "/usr/local"
	end if
	set result to display alert "Uninstall RethinkDB?" message "This will uninstall the RethinkDB " & installedVersion & " binaries and support files from:" & return & displayPath & return & return & "Your data files will not be affected. You will be asked for your Administrator password." as warning buttons {"Uninstall", "Cancel"} default button "Cancel"
	
	-- run the uninstall
	if the button returned of result is "Uninstall" then
		do shell script "set -e; /usr/sbin/pkgutil --files '" & packageIdentifier & "' | sort -r | sed 's/^/" & escapePath(installedPath) & "/' | egrep -vx '/usr|/usr/local|/usr/local/[^/]*' | while read FILE; do rm -fd \"$FILE\"; done && pkgutil --forget '" & packageIdentifier & "'" with administrator privileges
		display alert "RethinkDB " & installedVersion & " sucessfully removed from:" & return & displayPath giving up after 10
	end if
end if