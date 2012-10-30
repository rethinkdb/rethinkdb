# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python

import time, smtplib, socket, email.mime.text

def try_repeatedly(function, exception, num_tries=10, try_interval=10):
    while True:
        try:
            return function()
        except exception:
            if num_tries > 0:
                num_tries -= 1
                time.sleep(try_interval)
            else:
                raise

def send_gmail(subject, body, sender, receiver):
    """send_gmail(subject, body, (fromaddr, password), toaddr) -> None

    Sends email from `fromaddr` to `toaddr`. The email subject will be `subject`
    and the body will be `body`. The email will be plain-text and there is no
    way to attach anything to the email. `fromaddr` must be an address that is
    maintained by Google (that is, a GMail address or address of a company that
    hosts its mail using Google). `password` must be the password for that mail
    account."""

    (fromaddr, password) = sender

    smtp_server, smtp_port = "smtp.gmail.com:587".split(":")

    message = email.mime.text.MIMEText(body)
    message.add_header("Subject", subject)

    s = try_repeatedly(lambda: smtplib.SMTP(smtp_server, smtp_port), socket.gaierror)
    s.starttls()
    try_repeatedly(lambda: s.login(fromaddr, password), smtplib.SMTPAuthenticationError)
    s.sendmail(fromaddr, [receiver], message.as_string())
    s.quit()

if __name__ == "__main__":

    import sys
    argv = sys.argv[:]
    prog_name = argv.pop(0)

    def bad(msg):
        print >>sys.stderr, prog_name+":", msg
        sys.exit(1)

    def parse_arg(expected):
        if not argv:
            bad("Expected a %s; found end of arguments." % expected)
        return argv.pop(0)

    subject = None
    fromaddr = None
    toaddr = None
    password = None

    while argv:
        flag = argv.pop(0)
        if "=" in flag:
            parts = flag.split("=")
            argv.insert("=".join(parts[1:]))
            flag = parts[0]
        if flag in ["--subject", "-s"]:
            subject = parse_arg("subject for the email")
        elif flag in ["--from", "-f"]:
            fromaddr = parse_arg("email address")
        elif flag in ["--to", "-t"]:
            toaddr = parse_arg("email address")
        elif flag in ["--password", "-p"]:
            password = parse_arg("password")
        else:
            bad("Unrecognized flag: %r" % flag)

    if not subject:
        bad("The '--subject' flag is mandatory.")
    if not fromaddr:
        bad("The '--from' flag is mandatory.")
    if not toaddr:
        bad("The '--to' flag is mandatory.")
    if not password:
        bad("The '--password' flag is mandatory.")

    body = sys.stdin.read()
    if not body:
        bad("Expected email body on stdin")

    print "Sending email to <%s>..." % toaddr
    send_gmail(subject, body, (fromaddr, password), toaddr)
    print "Email sent successfully."
