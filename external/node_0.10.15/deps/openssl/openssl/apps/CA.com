$! CA - wrapper around ca to make it easier to use ... basically ca requires
$!      some setup stuff to be done before you can use it and this makes
$!      things easier between now and when Eric is convinced to fix it :-)
$!
$! CA -newca ... will setup the right stuff
$! CA -newreq ... will generate a certificate request 
$! CA -sign ... will sign the generated request and output 
$!
$! At the end of that grab newreq.pem and newcert.pem (one has the key 
$! and the other the certificate) and cat them together and that is what
$! you want/need ... I'll make even this a little cleaner later.
$!
$!
$! 12-Jan-96 tjh    Added more things ... including CA -signcert which
$!                  converts a certificate to a request and then signs it.
$! 10-Jan-96 eay    Fixed a few more bugs and added the SSLEAY_CONFIG
$!                 environment variable so this can be driven from
$!                 a script.
$! 25-Jul-96 eay    Cleaned up filenames some more.
$! 11-Jun-96 eay    Fixed a few filename missmatches.
$! 03-May-96 eay    Modified to use 'openssl cmd' instead of 'cmd'.
$! 18-Apr-96 tjh    Original hacking
$!
$! Tim Hudson
$! tjh@cryptsoft.com
$!
$!
$! default ssleay.cnf file has setup as per the following
$! demoCA ... where everything is stored
$
$ IF F$TYPE(SSLEAY_CONFIG) .EQS. "" THEN SSLEAY_CONFIG := SSLLIB:SSLEAY.CNF
$
$ DAYS   = "-days 365"
$ REQ    = openssl + " req " + SSLEAY_CONFIG
$ CA     = openssl + " ca " + SSLEAY_CONFIG
$ VERIFY = openssl + " verify"
$ X509   = openssl + " x509"
$ PKCS12 = openssl + " pkcs12"
$ echo   = "write sys$Output"
$ RET = 1
$!
$! 2010-12-20 SMS.
$! Use a concealed logical name to reduce command line lengths, to
$! avoid DCL errors on VAX:
$!     %DCL-W-TKNOVF, command element is too long - shorten
$! (Path segments like "openssl-1_0_1-stable-SNAP-20101217" accumulate
$! quickly.)
$!
$ CATOP = F$PARSE( F$ENVIRONMENT( "DEFAULT"), "[]")- "].;"+ ".demoCA.]"
$ define /translation_attributes = concealed CATOP 'CATOP'
$!
$ on error then goto clean_up
$ on control_y then goto clean_up
$!
$ CAKEY  = "CATOP:[private]cakey.pem"
$ CACERT = "CATOP:[000000]cacert.pem"
$
$ __INPUT := SYS$COMMAND
$!
$ i = 1
$opt_loop:
$ if i .gt. 8 then goto opt_loop_end
$
$ prog_opt = F$EDIT(P'i',"lowercase")
$
$ IF (prog_opt .EQS. "?" .OR. prog_opt .EQS. "-h" .OR. prog_opt .EQS. "-help") 
$ THEN
$   echo "usage: CA -newcert|-newreq|-newca|-sign|-verify" 
$   goto clean_up
$ ENDIF
$!
$ IF (prog_opt .EQS. "-input")
$ THEN
$   ! Get input from somewhere other than SYS$COMMAND
$   i = i + 1
$   __INPUT = P'i'
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-newcert")
$ THEN
$   ! Create a certificate.
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   REQ -new -x509 -keyout newreq.pem -out newreq.pem 'DAYS'
$   RET=$STATUS
$   echo "Certificate (and private key) is in newreq.pem"
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-newreq")
$ THEN
$   ! Create a certificate request
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   REQ -new -keyout newreq.pem -out newreq.pem 'DAYS'
$   RET=$STATUS
$   echo "Request (and private key) is in newreq.pem"
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-newca")
$ THEN
$   ! If explicitly asked for or it doesn't exist then setup the directory
$   ! structure that Eric likes to manage things.
$   IF F$SEARCH( "CATOP:[000000]serial.") .EQS. ""
$   THEN
$     CREATE /DIRECTORY /PROTECTION=OWNER:RWED CATOP:[000000]
$     CREATE /DIRECTORY /PROTECTION=OWNER:RWED CATOP:[certs]
$     CREATE /DIRECTORY /PROTECTION=OWNER:RWED CATOP:[crl]
$     CREATE /DIRECTORY /PROTECTION=OWNER:RWED CATOP:[newcerts]
$     CREATE /DIRECTORY /PROTECTION=OWNER:RWED CATOP:[private]
$
$     OPEN /WRITE ser_file CATOP:[000000]serial. 
$     WRITE ser_file "01"
$     CLOSE ser_file
$     APPEND /NEW_VERSION NL: CATOP:[000000]index.txt
$
$     ! The following is to make sure access() doesn't get confused.  It
$     ! really needs one file in the directory to give correct answers...
$     COPY NLA0: CATOP:[certs].;
$     COPY NLA0: CATOP:[crl].;
$     COPY NLA0: CATOP:[newcerts].;
$     COPY NLA0: CATOP:[private].;
$   ENDIF
$!
$   IF F$SEARCH( CAKEY) .EQS. ""
$   THEN
$     READ '__INPUT' FILE -
       /PROMPT="CA certificate filename (or enter to create): "
$     IF (FILE .NES. "") .AND. (F$SEARCH(FILE) .NES. "")
$     THEN
$       COPY 'FILE' 'CAKEY'
$       RET=$STATUS
$     ELSE
$       echo "Making CA certificate ..."
$       DEFINE /USER_MODE SYS$INPUT '__INPUT'
$       REQ -new -x509 -keyout 'CAKEY' -out 'CACERT' 'DAYS'
$       RET=$STATUS
$     ENDIF
$   ENDIF
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-pkcs12")
$ THEN
$   i = i + 1
$   cname = P'i'
$   IF cname .EQS. "" THEN cname = "My certificate"
$   PKCS12 -in newcert.pem -inkey newreq.pem -certfile 'CACERT' -
     -out newcert.p12 -export -name "''cname'"
$   RET=$STATUS
$   goto clean_up
$ ENDIF
$!
$ IF (prog_opt .EQS. "-xsign")
$ THEN
$!
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   CA -policy policy_anything -infiles newreq.pem
$   RET=$STATUS
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF ((prog_opt .EQS. "-sign") .OR. (prog_opt .EQS. "-signreq"))
$ THEN
$!   
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   CA -policy policy_anything -out newcert.pem -infiles newreq.pem
$   RET=$STATUS
$   type newcert.pem
$   echo "Signed certificate is in newcert.pem"
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-signcert")
$  THEN
$!   
$   echo "Cert passphrase will be requested twice - bug?"
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   X509 -x509toreq -in newreq.pem -signkey newreq.pem -out tmp.pem
$   DEFINE /USER_MODE SYS$INPUT '__INPUT'
$   CA -policy policy_anything -out newcert.pem -infiles tmp.pem
y
y
$   type newcert.pem
$   echo "Signed certificate is in newcert.pem"
$   GOTO opt_loop_continue
$ ENDIF
$!
$ IF (prog_opt .EQS. "-verify")
$ THEN
$!   
$   i = i + 1
$   IF (p'i' .EQS. "")
$   THEN
$     DEFINE /USER_MODE SYS$INPUT '__INPUT'
$     VERIFY "-CAfile" 'CACERT' newcert.pem
$   ELSE
$     j = i
$    verify_opt_loop:
$     IF j .GT. 8 THEN GOTO verify_opt_loop_end
$     IF p'j' .NES. ""
$     THEN 
$       DEFINE /USER_MODE SYS$INPUT '__INPUT'
$       __tmp = p'j'
$       VERIFY "-CAfile" 'CACERT' '__tmp'
$       tmp=$STATUS
$       IF tmp .NE. 0 THEN RET=tmp
$     ENDIF
$     j = j + 1
$     GOTO verify_opt_loop
$    verify_opt_loop_end:
$   ENDIF
$   
$   GOTO opt_loop_end
$ ENDIF
$!
$ IF (prog_opt .NES. "")
$ THEN
$!   
$   echo "Unknown argument ''prog_opt'"
$   RET = 3
$   goto clean_up
$ ENDIF
$
$opt_loop_continue:
$ i = i + 1
$ GOTO opt_loop
$
$opt_loop_end:
$!
$clean_up:
$!
$ if f$trnlnm( "CATOP", "LNM$PROCESS") .nes. "" then -
   deassign /process CATOP
$!
$ EXIT 'RET'
