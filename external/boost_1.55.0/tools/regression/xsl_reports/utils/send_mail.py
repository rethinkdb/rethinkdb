
import smtplib

def send_mail( mail, subject, msg = '' ):
    smtp_server = smtplib.SMTP( 'mail.%s' % mail.split( '@' )[-1] )
    smtp_server.sendmail( 
          mail
        , [ mail ]
        , 'Subject: %s\n' % subject
            + 'To: %s\n' % mail
            + '\n'
            + msg 
        )
