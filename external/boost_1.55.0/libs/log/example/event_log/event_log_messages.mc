; /* --------------------------------------------------------
; HEADER SECTION
; */
SeverityNames=(Debug=0x0:MY_SEVERITY_DEBUG
            Info=0x1:MY_SEVERITY_INFO
            Warning=0x2:MY_SEVERITY_WARNING
            Error=0x3:MY_SEVERITY_ERROR
            )

; /* --------------------------------------------------------
; MESSAGE DEFINITION SECTION
; */

MessageIdTypedef=WORD

MessageId=0x1
SymbolicName=MY_CATEGORY_1
Language=English
Category 1
.

MessageId=0x2
SymbolicName=MY_CATEGORY_2
Language=English
Category 2
.

MessageId=0x3
SymbolicName=MY_CATEGORY_3
Language=English
Category 3
.

MessageIdTypedef=DWORD

MessageId=0x100
Severity=Warning
Facility=Application
SymbolicName=LOW_DISK_SPACE_MSG
Language=English
The drive %1 has low free disk space. At least %2 Mb of free space is recommended.
.

MessageId=0x101
Severity=Error
Facility=Application
SymbolicName=DEVICE_INACCESSIBLE_MSG
Language=English
The drive %1 is not accessible.
.

MessageId=0x102
Severity=Info
Facility=Application
SymbolicName=SUCCEEDED_MSG
Language=English
Operation finished successfully in %1 seconds.
.
