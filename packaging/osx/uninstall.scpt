-- check that a package version has been installed
set packageIdentifier to ""
repeat with candidateIdentifier in {"rethinkdb", "com.rethinkdb", "com.rethinkdb.server"}
	try
		do shell script "/usr/sbin/pkgutil --files " & candidateIdentifier
		set packageIdentifier to candidateIdentifier
		exit repeat
	end try
end repeat

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
	set result to display alert "Uninstall RethinkDB?" message "This will uninstall the RethinkDB binaries and support files from '/usr/local/'." & return & return & "Your data files will not be affected. You will be asked for your Administrator password." as warning buttons {"Uninstall", "Cancel"} default button "Cancel"
	
	-- run the uninstall
	if the button returned of result is "Uninstall" then
		do shell script "set -e; pkgutil --files " & packageIdentifier & " --only-files | while read LINE; do rm -f $LINE; done && pkgutil --files " & packageIdentifier & " --only-dirs | while read LINE; do rmdir $LINE; done && pkgutil --forget " & packageIdentifier with administrator privileges
	end if
end if
